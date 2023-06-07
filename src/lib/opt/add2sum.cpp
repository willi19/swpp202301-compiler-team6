#include "add2sum.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
using namespace std;

namespace sc::opt::add2sum {
PreservedAnalyses Add2SumPass::run(Function &F, FunctionAnalysisManager &FAM) {
  bool Changed = false;
  if (F.getName() == "oracle") {
    return PreservedAnalyses::all();
  }
  for (BasicBlock &BB : F) {
    // ReplaceInst : Instructions that should be replaced
    // Summands : Summands that are tied to ReplaceInst[i]
    vector<Instruction *> ReplaceInst;
    vector<vector<Value *>> Summands;
    vector<Instruction *> Handled;
    Module *M = BB.getModule();

    // Iterate reversely over last-wise add instructions
    for (BasicBlock::reverse_iterator it = BB.rbegin(); it != BB.rend(); ++it) {
      Instruction *I = &(*it);
      if (I->getOpcode() != Instruction::Add) {
        continue;
      }
      if (find(Handled.begin(), Handled.end(), I) != Handled.end()) {
        continue;
      }
      vector<Value *> Candidate;
      vector<Instruction *> Erase;
      Candidate.push_back(I->getOperand(0));
      Candidate.push_back(I->getOperand(1));

      // For each candidate summand, DFS upwards
      for (int i = 0; i < Candidate.size(); ++i) {
        if (Candidate.size() == 8)
          break;
        Value *CurValue = Candidate[i];

        // Candidate must be deprecated if Value is constant 0.
        if (ConstantInt *Zero = dyn_cast<ConstantInt>(CurValue)) {
          if (Zero->isZero()) {
            Candidate.erase(Candidate.begin() + i);
            i--;
            continue;
          }
        }

        // The candidate value can be classified as,
        // 1. Newborn / 2. Untrackable / 3. Propagate
        // Newborn Value: value created within the BB not by 'add'.
        // Untrackable Value : value not created within the BB.
        // Propagate Value : value created within the BB by 'add'.
        //
        // Data Dependency:
        // Newborn/Untrackable is not erased, so does not care
        // Propagate Value is erased, so should not be used more than once in
        // the function, only the add instruction.

        bool isNewborn = false;
        bool isUntrackable = true;
        Instruction *DefInst = nullptr;
        for (Instruction &InnerI : BB) {
          Value *InnerV = static_cast<Value *>(&InnerI);
          if (InnerV != CurValue)
            continue;
          DefInst = &InnerI;
          isUntrackable = false;
          if (InnerI.getOpcode() != Instruction::Add) {
            isNewborn = true;
          }
          break;
        }

        // Automatically Candidated
        if (isNewborn or isUntrackable)
          continue;

        // Count number of uses of Candidate Value
        int count = 0;
        for (BasicBlock &b : F) {
          for (Instruction &ii : b) {
            for (Use &u : ii.operands()) {
              if (u.get() == CurValue)
                count++;
            }
          }
        }

        // If not Newborn / Untrackable / More than 1 use -> Candidate
        // Remove original value, append 2 summands, move back iterator
        if (!isNewborn and !isUntrackable and count == 1) {
          Candidate.erase(Candidate.begin() + i);
          i--;
          Erase.push_back(DefInst);
          Candidate.push_back(DefInst->getOperand(0));
          Candidate.push_back(DefInst->getOperand(1));
        }
      }

      if (Candidate.size() < 4)
        continue;
      ReplaceInst.push_back(I);
      Summands.push_back(Candidate);
      for (Instruction *Insts : Erase) {
        Handled.push_back(Insts);
      }
    }

    if (ReplaceInst.empty())
      continue;
    Changed = true;

    // Replace last instruction to sum
    for (int i = 0; i < ReplaceInst.size(); ++i) {
      Instruction *OldInst = ReplaceInst[i];
      unsigned int BitWidth = OldInst->getType()->getIntegerBitWidth();
      vector<Type *> ArgTypes(8, Summands[i][0]->getType());
      vector<Value *> Params = Summands[i];
      for (; Params.size() < 8;) {
        Value *ZeroValue = ConstantInt::get(Summands[i][0]->getType(), 0);
        Params.push_back(ZeroValue);
      }

      // Create functions based on operadn types
      FunctionType *FuncType =
          FunctionType::get(OldInst->getType(), ArgTypes, false);
      Function *Func = nullptr;
      if (BitWidth == 1) {
        Func = M->getFunction("int_sum_i1");
        if (!Func) {
          Func = Function::Create(FuncType, Function::ExternalLinkage,
                                  "int_sum_i1", M);
        }
      } else if (BitWidth == 8) {
        Func = M->getFunction("int_sum_i8");
        if (!Func) {
          Func = Function::Create(FuncType, Function::ExternalLinkage,
                                  "int_sum_i8", M);
        }
      } else if (BitWidth == 16) {
        Func = M->getFunction("int_sum_i16");
        if (!Func) {
          Func = Function::Create(FuncType, Function::ExternalLinkage,
                                  "int_sum_i16", M);
        }
      } else if (BitWidth == 32) {
        Func = M->getFunction("int_sum_i32");
        if (!Func) {
          Func = Function::Create(FuncType, Function::ExternalLinkage,
                                  "int_sum_i32", M);
        }
      } else if (BitWidth == 64) {
        Func = M->getFunction("int_sum_i64");
        if (!Func) {
          Func = Function::Create(FuncType, Function::ExternalLinkage,
                                  "int_sum_i64", M);
        }
      }

      Instruction *NewInst =
          CallInst::Create(FuncType, Func, Params, OldInst->getName(), OldInst);
      Value *NewV = NewInst;
      OldInst->replaceAllUsesWith(NewV);
      OldInst->eraseFromParent();
    }
    // Erase all instructions in Handled
    for (Instruction *I : Handled) {
      I->eraseFromParent();
    }
  }
  // Module *M = F.getParent();
  // errs() << *M;
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPAdd2Sum", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "add2sum") {
                    FPM.addPass(Add2SumPass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::add2sum
