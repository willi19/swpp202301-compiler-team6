#include "intrinsic_eliminate.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

using namespace llvm;

namespace sc::opt::intrinsic_elim {
PreservedAnalyses
IntrinsicEliminatePass::run(llvm::Function &F,
                            llvm::FunctionAnalysisManager &FAM) {
  bool Changed = false;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
        Changed = true;
        errs() << "[dbg:intrinsic_elim] Intrinsic instruction " << *II << "\n";
        IRBuilder<> Builder(F.getContext());
        switch (II->getIntrinsicID()) {
        case Intrinsic::smin: {
          Builder.SetInsertPoint(II);
          Value *Arg1 = II->getArgOperandUse(0);
          Value *Arg2 = II->getArgOperandUse(1);
          Value *Cmp = Builder.CreateICmpSLT(Arg1, Arg2);
          Value *Sel = Builder.CreateSelect(Cmp, Arg1, Arg2);
          II->replaceAllUsesWith(Sel);
          break;
        }
        case Intrinsic::smax: {
          Builder.SetInsertPoint(II);
          Value *Arg1 = II->getArgOperandUse(0);
          Value *Arg2 = II->getArgOperandUse(1);
          Value *Cmp = Builder.CreateICmpSGT(Arg1, Arg2);
          Value *Sel = Builder.CreateSelect(Cmp, Arg1, Arg2);
          II->replaceAllUsesWith(Sel);
          break;
        }
        case Intrinsic::umin: {
          Builder.SetInsertPoint(II);
          Value *Arg1 = II->getArgOperandUse(0);
          Value *Arg2 = II->getArgOperandUse(1);
          Value *Cmp = Builder.CreateICmpULT(Arg1, Arg2);
          Value *Sel = Builder.CreateSelect(Cmp, Arg1, Arg2);
          II->replaceAllUsesWith(Sel);
          break;
        }
        case Intrinsic::umax: {
          Builder.SetInsertPoint(II);
          Value *Arg1 = II->getArgOperandUse(0);
          Value *Arg2 = II->getArgOperandUse(1);
          Value *Cmp = Builder.CreateICmpUGT(Arg1, Arg2);
          Value *Sel = Builder.CreateSelect(Cmp, Arg1, Arg2);
          II->replaceAllUsesWith(Sel);
          break;
        }
        default:
          llvm_unreachable("Found unhandled intrinsic instruction");
          break;
        }
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
} // namespace sc::opt::intrinsic_elim
