#include "load2aload.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/PassManager.h"
#include <queue>

using namespace llvm;

namespace {
bool canStoreMemory(Instruction *I) {
  if (isa<StoreInst>(I)) {
    return true;
  }
  if (auto *Call = dyn_cast<CallBase>(I)) {
    return !Call->onlyReadsMemory() && !Call->doesNotAccessMemory();
  }
  if (auto *Intrin = dyn_cast<IntrinsicInst>(I)) {
    return Intrin->mayHaveSideEffects();
  }
  return false;
}
} // namespace

namespace sc::opt::load2aload {
PreservedAnalyses Load2AloadPass::run(Function &F,
                                      FunctionAnalysisManager &FAM) {
  bool Changed = false;
  for (auto &BB : F) {
    MapVector<LoadInst *, Instruction *> ReplaceMap;
    std::deque<Instruction *> InstQueue;

    for (auto &I : BB) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (!InstQueue.empty()) {
          ReplaceMap[LI] = InstQueue.front();
        }
      }

      if (canStoreMemory(&I)) {
        InstQueue.clear();
      }
      InstQueue.push_back(&I);
    }

    for (auto &[LI, InsertPos] : ReplaceMap) {
      Type *Ty = LI->getType();
      Value *V = LI->getPointerOperand();
      FunctionCallee Aload = nullptr;
      auto *M = F.getParent();
      auto &C = F.getContext();
      if (Ty->isIntegerTy(8)) {
        auto *FTy = FunctionType::get(Type::getInt8Ty(C),
                                      {Type::getInt8PtrTy(C)}, false);
        Aload = M->getOrInsertFunction("aload_i8", FTy);
      } else if (Ty->isIntegerTy(16)) {
        auto *FTy = FunctionType::get(Type::getInt16Ty(C),
                                      {Type::getInt16PtrTy(C)}, false);
        Aload = M->getOrInsertFunction("aload_i16", FTy);
      } else if (Ty->isIntegerTy(32)) {
        auto *FTy = FunctionType::get(Type::getInt32Ty(C),
                                      {Type::getInt32PtrTy(C)}, false);
        Aload = M->getOrInsertFunction("aload_i32", FTy);
      } else if (Ty->isIntegerTy(64)) {
        auto *FTy = FunctionType::get(Type::getInt64Ty(C),
                                      {Type::getInt64PtrTy(C)}, false);
        Aload = M->getOrInsertFunction("aload_i64", FTy);
      } else {
        continue;
      }
      CallInst *CI = CallInst::Create(Aload, V);
      if (auto *VI = dyn_cast<Instruction>(V)) {
        if (InsertPos->comesBefore(VI->getNextNode())) {
          InsertPos = VI->getNextNode();
        }
      }
      CI->insertBefore(InsertPos);
      LI->replaceAllUsesWith(CI);
      LI->eraseFromParent();
      Changed = true;
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
} // namespace sc::opt::load2aload
