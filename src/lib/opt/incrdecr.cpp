#include "incrdecr.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <string>

using namespace llvm;
using namespace std;

namespace sc::opt::incrdecr {
PreservedAnalyses IncrDecrPass::run(Function &F, FunctionAnalysisManager &FAM) {
  bool Changed = false;
  for (BasicBlock &BB : F) {
    // Separate iteration and replacement since &I : BB will be harmed
    // ReplaceInst: Instructions that should be replaced
    // UsedVal: Values tied on the instructions
    // IncOrDec: Whether the instruction is substituted to incr / decr
    // Repetitions: Number of incr / decr instructions required
    vector<Instruction *> ReplaceInst;
    vector<Value *> UsedVal;
    vector<bool> IncOrDec;
    vector<int> Repetitions;
    Module *M = BB.getModule();
    for (Instruction &I : BB) {
      // Check if operands and result are integers
      if (!I.getType()->isIntegerTy())
        continue;
      Value *V = nullptr;
      bool isIncr = false;
      bool isDecr = false;
      unsigned int n = 0;
      // Check twice for add due to its commutativity
      // Case 1 : %a = add %b, n || %a = add n, %b
      if (auto *AddInst = dyn_cast<BinaryOperator>(&I)) {
        if (AddInst->getOpcode() == Instruction::Add) {
          Value *Op0 = AddInst->getOperand(0);
          ConstantInt *Op1 = dyn_cast<ConstantInt>(AddInst->getOperand(1));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp > 0 and tmp < 5) {
              V = Op0;
              isIncr = true;
              n = tmp;
            }
          }

          Op0 = AddInst->getOperand(1);
          Op1 = dyn_cast<ConstantInt>(AddInst->getOperand(0));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp > 0 and tmp < 5) {
              V = Op0;
              isIncr = true;
              n = tmp;
            }
          }
        }
      }
      // Case 2 : %a = sub %b, -n
      if (auto *SubInst = dyn_cast<BinaryOperator>(&I)) {
        if (SubInst->getOpcode() == Instruction::Sub) {
          Value *Op0 = SubInst->getOperand(0);
          ConstantInt *Op1 = dyn_cast<ConstantInt>(SubInst->getOperand(1));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp < 0 and tmp > -5) {
              V = Op0;
              isIncr = true;
              n = -tmp;
            }
          }
        }
      }
      // Case 3 : %a = add %b, -n
      if (auto *AddInst = dyn_cast<BinaryOperator>(&I)) {
        if (AddInst->getOpcode() == Instruction::Add) {
          Value *Op0 = AddInst->getOperand(0);
          ConstantInt *Op1 = dyn_cast<ConstantInt>(AddInst->getOperand(1));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp < 0 and tmp > -5) {
              V = Op0;
              isDecr = true;
              n = -tmp;
            }
          }

          Op0 = AddInst->getOperand(1);
          Op1 = dyn_cast<ConstantInt>(AddInst->getOperand(0));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp < 0 and tmp > -5) {
              V = Op0;
              isDecr = true;
              n = -tmp;
            }
          }
        }
      }
      // Case 4 : %a = sub %b, n
      if (auto *SubInst = dyn_cast<BinaryOperator>(&I)) {
        if (SubInst->getOpcode() == Instruction::Sub) {
          Value *Op0 = SubInst->getOperand(0);
          ConstantInt *Op1 = dyn_cast<ConstantInt>(SubInst->getOperand(1));
          if (Op1) {
            int tmp = Op1->getSExtValue();
            if (tmp > 0 and tmp < 5) {
              V = Op0;
              isDecr = true;
              n = tmp;
            }
          }
        }
      }
      if (!isIncr and !isDecr)
        continue;

      // Regarding Oracle
      if (F.getName() == "oracle") {
        // Will be modified to n > 1 when incr/decr allowed in oracle
        if (n > 0) {
          continue;
        }
      }
      ReplaceInst.push_back(&I);
      UsedVal.push_back(V);
      IncOrDec.push_back(isIncr ? 1 : 0);
      Repetitions.push_back(n);
    }

    if (ReplaceInst.empty())
      continue;
    Changed = true;

    // Iterate over collected instructions
    for (int i = 0; i < ReplaceInst.size(); ++i) {
      Instruction *I = ReplaceInst[i];
      Value *V = UsedVal[i];
      bool isIncr = IncOrDec[i];
      int n = Repetitions[i];
      unsigned int BitWidth = I->getType()->getIntegerBitWidth();

      // Create functions based on operand types
      FunctionType *funcType =
          FunctionType::get(I->getType(), {V->getType()}, false);
      Function *func = nullptr;
      if (BitWidth == 1 and isIncr) {
        func = M->getFunction("incr_i1");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "incr_i1", M);
        }
      }
      if (BitWidth == 8 and isIncr) {
        func = M->getFunction("incr_i8");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "incr_i8", M);
        }
      }
      if (BitWidth == 16 and isIncr) {
        func = M->getFunction("incr_i16");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "incr_i16", M);
        }
      }
      if (BitWidth == 32 and isIncr) {
        func = M->getFunction("incr_i32");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "incr_i32", M);
        }
      }
      if (BitWidth == 64 and isIncr) {
        func = M->getFunction("incr_i64");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "incr_i64", M);
        }
      }
      if (BitWidth == 1 and !isIncr) {
        func = M->getFunction("decr_i1");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "decr_i1", M);
        }
      }
      if (BitWidth == 8 and !isIncr) {
        func = M->getFunction("decr_i8");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "decr_i8", M);
        }
      }
      if (BitWidth == 16 and !isIncr) {
        func = M->getFunction("decr_i16");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "decr_i16", M);
        }
      }
      if (BitWidth == 32 and !isIncr) {
        func = M->getFunction("decr_i32");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "decr_i32", M);
        }
      }
      if (BitWidth == 64 and !isIncr) {
        func = M->getFunction("decr_i64");
        if (!func) {
          func = Function::Create(funcType, Function::ExternalLinkage,
                                  "decr_i64", M);
        }
      }
      // Add Call instructions and replace/erase original instructions
      Value *NewOp = V;
      for (int j = 0; j < n; ++j) {
        Instruction *NewInst =
            CallInst::Create(funcType, func, {NewOp}, I->getName(), I);
        NewOp = NewInst;
      }
      I->replaceAllUsesWith(NewOp);
      for (int i = 0; i < UsedVal.size(); ++i) {
        if (UsedVal[i] == I)
          UsedVal[i] = NewOp;
      }
      I->eraseFromParent();
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPIncrDecr", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "incrdecr") {
                    FPM.addPass(IncrDecrPass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::incrdecr
