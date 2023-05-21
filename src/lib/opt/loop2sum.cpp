#include "loop2sum.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <queue>
#include <string>
#include <unordered_set>

using namespace llvm;
using namespace std;

namespace sc::opt::loop2sum {
PreservedAnalyses Loop2SumPass::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  // Get the loop header and latch
  BasicBlock *Header = L.getHeader();
  BasicBlock *Latch = L.getLoopLatch();
  Module *M = Header->getModule();
  Function *F = Header->getParent();
  ValueSymbolTable &Symbol = M->getValueSymbolTable();

  // There is a myriad of loop structures, since there are branches
  // Hence, first simplify the 'simple types' by SimplifyCFG.
  //
  // The common loop structure is:
  // 1-block structure: <head> -> br %cond <head>, <end>
  // Induction variable controlled by PHI, Accumulator variable in memory
  //
  // 2-block structure: <head> -> br %cond <body>, <end> | <body> -> br <head>
  // Induction/Accumulator variable controlled by PHI in head

  // For now, suppose loop structure is 2-block structure.
  // Check if the loop block structure is Cond -> Body / End
  Instruction *Terminator = Header->getTerminator();
  if (BranchInst *Branch = dyn_cast<BranchInst>(Terminator)) {
    if (Branch->isConditional() and Branch->getSuccessor(0) != Latch and
        Branch->getSuccessor(1) != Latch) {
      return PreservedAnalyses::all();
    }
  }

  // Get the induction/accumulator variable
  PHINode *IndVar = nullptr;
  PHINode *AccVar = nullptr;
  Value *AccTmp = nullptr;

  // Induction variable satisfies:
  // 1. Used to calculate the conditional variable
  // 2. Controlled by PHI node inside the Header block

  // Get cond variable
  Value *Cond = nullptr;
  for (Instruction &Inst : *Header) {
    if (BranchInst *Branch = dyn_cast<BranchInst>(&Inst)) {
      if (Branch->isConditional()) {
        Cond = Branch->getCondition();
      }
    }
  }
  // Find IndVar by Iteratively BFS starting from cond, for each PHI node
  for (Instruction &Inst : *Header) {
    if (PHINode *Phi = dyn_cast<PHINode>(&Inst)) {
      Value *PhiValue = Phi;

      queue<Value *> Q;
      unordered_set<Value *> Visited;
      bool isIndVar = false;

      Q.push(Cond);
      while (!Q.empty()) {
        Value *Cur = Q.front();
        Q.pop();
        if (Visited.count(Cur)) {
          continue;
        }
        Visited.insert(Cur);
        if (Cur == PhiValue) {
          isIndVar = true;
          break;
        }
        if (Instruction *I = dyn_cast<Instruction>(Cur)) {
          for (Value *operand : I->operands()) {
            Q.push(operand);
          }
        }
      }

      if (isIndVar) {
        IndVar = Phi;
        break;
      }
    }
  }
  // If IndVar not found, invalid
  if (!IndVar) {
    return PreservedAnalyses::all();
  }

  // Accumulator variable satisfies:
  // 1. Accumulated by add instructions in the body block
  // 2. Paired with a dual variable (%sum = phi [%add] <-> %add = add %sum, %0)
  // 3. Controlled by PHI node inside the header block
  // The dual variable is stored as AccTmp
  int IndexOfAdd; // The index of add variable in the AccVar->getIncomingvalue
  unordered_set<Instruction *> VisitedInstructions;
  for (Instruction &Inst : *Header) {
    if (PHINode *Phi = dyn_cast<PHINode>(&Inst)) {
      if (Phi == IndVar)
        continue;

      bool Phifound = false;
      for (int i = 0; i < Phi->getNumIncomingValues(); ++i) {
        Value *IncomingValue = Phi->getIncomingValue(i);
        Value *PhiValue = Phi;

        if (Instruction *IncomingDef = dyn_cast<Instruction>(IncomingValue)) {
          if (IncomingDef->getParent() == Latch) {
            if (IncomingDef->getOpcode() == Instruction::Add) {
              if (IncomingDef->getNumOperands() == 2) {
                Value *op1 = IncomingDef->getOperand(0);
                Value *op2 = IncomingDef->getOperand(1);
                if (op1 == Phi or op2 == Phi) {
                  Phifound = true;
                  AccVar = Phi;
                  AccTmp = dyn_cast<Value>(IncomingDef);
                  IndexOfAdd = i;
                  break;
                }
              }
            }
          }
        }
      }
      if (Phifound)
        break;
    }
  }
  // If AccVar not found, invalid
  if (!AccVar) {
    return PreservedAnalyses::all();
  }
  // If AccVar not Integer, invalid
  Value *AccVal = AccVar;
  Type *AccTy = AccVal->getType();
  if (!AccTy->isIntegerTy()) {
    return PreservedAnalyses::all();
  }

  // Check other loop conditions
  // If more PHI nodes other than Ind/Acc occur, invalid
  bool foundInd = false;
  bool foundAcc = false;
  for (Instruction &Inst : *Header) {
    if (PHINode *p = dyn_cast<PHINode>(&Inst)) {
      if (p == IndVar)
        foundInd = true;
      else if (p == AccVar)
        foundAcc = true;
      else {
        return PreservedAnalyses::all();
      }
    }
  }
  // Check if increments are simple, i.e. of the form i=0; i<n; ++i
  if (IndVar->getNumIncomingValues() != 2) {
    return PreservedAnalyses::all();
  }
  CmpInst *Cmp = dyn_cast<CmpInst>(Cond);
  auto Operator = Cmp->getPredicate();
  if (Operator != CmpInst::ICMP_SLT and Operator != CmpInst::ICMP_ULT) {
    return PreservedAnalyses::all();
  }
  if (Cmp->getOperand(0) != IndVar and Cmp->getOperand(1) != IndVar) {
    return PreservedAnalyses::all();
  }
  bool isIncrementByOne = false;
  for (Instruction &I : *Latch) {
    bool check = false;
    if (auto *AddInst = dyn_cast<BinaryOperator>(&I)) {
      if (AddInst->getOpcode() == Instruction::Add) {
        Value *Op0 = AddInst->getOperand(0);
        Value *Op1 = AddInst->getOperand(1);

        if (Op0 == IndVar and isa<ConstantInt>(Op1) and
            cast<ConstantInt>(Op1)->equalsInt(1)) {
          for (int i = 0; i < IndVar->getNumIncomingValues(); ++i) {
            if (IndVar->getIncomingValue(i) == AddInst) {
              isIncrementByOne = true;
              check = true;
              break;
            }
          }
        }
        if (Op1 == IndVar and isa<ConstantInt>(Op0) and
            cast<ConstantInt>(Op0)->equalsInt(1)) {
          for (int i = 0; i < IndVar->getNumIncomingValues(); ++i) {
            if (IndVar->getIncomingValue(i) == AddInst) {
              isIncrementByOne = true;
              check = true;
              break;
            }
          }
        }
      }
      if (check)
        break;
    }
  }
  if (!isIncrementByOne) {
    return PreservedAnalyses::all();
  }
  // If the Latch block contains harmful instructions that may cause
  // dependencies, invalid.
  for (Instruction &I : *Latch) {
    if (I.isTerminator())
      continue;
    if (I.getOpcode() == Instruction::Store or
        I.getOpcode() == Instruction::Call or isa<PHINode>(&I)) {
      return PreservedAnalyses::all();
    }
    // %0 or empty name is toxic name
    if (I.getName() == "0" or I.getName() == "") {
      I.setName("QYXV"+I.getName());
    }
  }

  
  // Check if the Accumulator Variable is used in only one instruction per loop.
  int AccVarCount = 0;
  int AccTmpCount = 0;
  for (Instruction &I : *Latch) {
    for (Use &OP : I.operands()) {
	  if (OP.get() == AccVar) {
	    AccVarCount++;
	  }
	  if (OP.get() == AccTmp) {
	    AccTmpCount++;
	  }
	}
  }
  if(AccVarCount > 1 or AccTmpCount > 0) {
    return PreservedAnalyses::all();
  }

  // Make a new vectorized block that connects 7 loops.
  // Each cloned variable has a ".vec(i)" suffix in its name.
  BasicBlock *Vectorized = BasicBlock::Create(
      Latch->getContext(), Latch->getName() + ".vec", Latch->getParent());
 
  for (int i = 0; i < 7; ++i) {
    for (Instruction &Inst : *Latch) {
      if (BranchInst *Branch = dyn_cast<BranchInst>(&Inst))
        break;
      Instruction *ClonedInst = Inst.clone();
      ClonedInst->setName(Inst.getName() + ".vec" + to_string(i));
	  //Symbol.reinsertValue(ClonedInst, ClonedInst->getName(), Inst);
      Vectorized->getInstList().push_back(ClonedInst);
	  // Symbol.createValueName(ClonedInst->getName(), ClonedInst);
    }
  }
  if (Latch->getTerminator()) {
    Instruction *Terminator = Latch->getTerminator()->clone();
    Vectorized->getInstList().push_back(Terminator);
  }

  // Update Induction variable dependencies inside Vectorized block.
  Value *CurIndVar = IndVar;
  Value *OrigIndVar = IndVar;
  for (Instruction &Inst : *Vectorized) {
    // Check if increment instruction
    bool check = true;
    if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(&Inst)) {
      if (binOp->getOpcode() == Instruction::Add) {
        Value *Op1 = binOp->getOperand(0);
        Value *Op2 = binOp->getOperand(1);

        if (Op1 == OrigIndVar) {
          if (ConstantInt *CI = dyn_cast<ConstantInt>(Op2)) {
            if (CI->equalsInt(1)) {
              for (Use &OP : Inst.operands()) {
                if (OP.get() == OrigIndVar)
                  OP.set(CurIndVar);
              }
              CurIndVar = dyn_cast<Value>(&Inst);
              check = false;
            }
          }
        } else if (Op2 == OrigIndVar) {
          if (ConstantInt *CI = dyn_cast<ConstantInt>(Op1)) {
            if (CI->equalsInt(1)) {
              for (Use &OP : Inst.operands()) {
                if (OP.get() == OrigIndVar)
                  OP.set(CurIndVar);
              }
              CurIndVar = dyn_cast<Value>(&Inst);
              check = false;
            }
          }
        }
      }
    }
    if (check) {
      for (Use &OP : Inst.operands()) {
        if (OP.get() == OrigIndVar) {
          OP.set(CurIndVar);
        }
      }
    }
  }

  // Update other variable dependencies inside Vectorized block.
  // For checking dependencies, compare names.
  for (BasicBlock::iterator it = Vectorized->begin(); it != Vectorized->end();
       ++it) {
    Instruction *CurInst = &(*it);
    string CurName = CurInst->getName().str();
    for (BasicBlock::iterator it2 = next(it); it2 != Vectorized->end(); ++it2) {
      Instruction *NextInst = &(*it2);
      string NextName = NextInst->getName().str();
      if (CurName.substr(0, CurName.size() - 1) ==
          NextName.substr(0, NextName.size() - 1))
        break;

      for (Use &OP : NextInst->operands()) {
        if (OP->hasName()) {
          if (OP->getName() == CurName.substr(0, CurName.size() - 5)) {
            Value *CurVar = CurInst;
            OP.set(CurVar);
          }
        }
      }
    }
  }
 
  // Change adds to sum.
  // Collect all summands, remove add instructions using Accumulator Variable.
  vector<Value *> Summands;
  Summands.push_back(AccVal);
  for (auto it = Vectorized->begin(); it != Vectorized->end(); ++it) {
    Instruction &Inst = *it;
    if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(&Inst)) {
      if (binOp->getOpcode() == Instruction::Add) {
        Value *Op1 = binOp->getOperand(0);
        Value *Op2 = binOp->getOperand(1);

        if (Op1 == AccVal) {
          Summands.push_back(Op2);
          it = Inst.eraseFromParent();
        } else if (Op2 == AccVal) {
          Summands.push_back(Op1);
          it = Inst.eraseFromParent();
        }
      }
    }
  }
  // Add sum function and its parameters.
  IntegerType *AccIntTy = dyn_cast<IntegerType>(AccTy);
  unsigned int BitWidth = AccIntTy->getBitWidth();
  vector<Type *> argTypes(8, Summands[0]->getType());
  FunctionType *funcType = nullptr;
  //Module *M = Vectorized->getModule();
  Function *func = nullptr;
  if (BitWidth == 1) {
    funcType = FunctionType::get(Type::getInt1Ty(Vectorized->getContext()),
                                 argTypes, false);
    func = M->getFunction("int_sum_i1");
    if (!func) {
      func = Function::Create(funcType, Function::ExternalLinkage, "int_sum_i1",
                              M);
    }
  } else if (BitWidth == 8) {
    funcType = FunctionType::get(Type::getInt8Ty(Vectorized->getContext()),
                                 argTypes, false);
    func = M->getFunction("int_sum_i8");
    if (!func) {
      func = Function::Create(funcType, Function::ExternalLinkage, "int_sum_i8",
                              M);
    }
  } else if (BitWidth == 16) {
    funcType = FunctionType::get(Type::getInt16Ty(Vectorized->getContext()),
                                 argTypes, false);
    func = M->getFunction("int_sum_i16");
    if (!func) {
      func = Function::Create(funcType, Function::ExternalLinkage,
                              "int_sum_i16", M);
    }
  } else if (BitWidth == 32) {
    funcType = FunctionType::get(Type::getInt32Ty(Vectorized->getContext()),
                                 argTypes, false);
    func = M->getFunction("int_sum_i32");
    if (!func) {
      func = Function::Create(funcType, Function::ExternalLinkage,
                              "int_sum_i32", M);
    }

  } else if (BitWidth == 64) {
    funcType = FunctionType::get(Type::getInt64Ty(Vectorized->getContext()),
                                 argTypes, false);
    func = M->getFunction("int_sum_i64");
    if (!func) {
      func = Function::Create(funcType, Function::ExternalLinkage,
                              "int_sum_i64", M);
    }
  }
  Instruction *Branch = Vectorized->getTerminator();
  CallInst *CI = CallInst::Create(func, Summands, "", Branch);
  
  // Experiments
  /*
  BranchInst *Br = dyn_cast<BranchInst>(Latch->getTerminator());
  ConstantInt *Condition = ConstantInt::getTrue(Latch->getContext());
  BasicBlock *Target = Br->getSuccessor(0);
  BranchInst *NewBr = BranchInst::Create(Target, Vectorized, Condition);
  ReplaceInstWithInst(Br, NewBr);
  */
  BranchInst *END = dyn_cast<BranchInst>(Header->getTerminator());
  BasicBlock *ENDBLOCK = &F->getEntryBlock();
  BranchInst *VECEND = dyn_cast<BranchInst>(Vectorized->getTerminator());
  VECEND->setSuccessor(0, ENDBLOCK);
  /*
  // Debug Zone
  
  for(Instruction &Inst: *Vectorized) {
    errs() << Inst << "\n";
  }
  errs() << "Approached here\n";
  // Insert Vectorized Block to the function
  Function *ParentF = Header->getParent();
  L.addBasicBlockToLoop(Vectorized, AR.LI);
  */
  errs() << *M;
  
  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPLoop2Sum", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop2sum") {
                    FPM.addPass(createFunctionToLoopPassAdaptor(Loop2SumPass()));
                    return true;
                  }
                  return false;
                });
          }};
}

}; // namespace sc::opt::loop2sum
