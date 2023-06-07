#include "load2aload.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <queue>

using namespace llvm;

namespace {
bool isAloadSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "aload_i8" || N == "aload_i16" || N == "aload_i32" ||
           N == "aload_i64";
  }
  return false;
}

bool isSumSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "int_sum_i1" || N == "int_sum_i8" || N == "int_sum_i16" ||
           N == "int_sum_i32" || N == "int_sum_i64";
  }
  return false;
}

bool isIncrDecrSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "incr_i1" || N == "incr_i8" || N == "incr_i16" ||
           N == "incr_i32" || N == "incr_i64" || N == "decr_i1" ||
           N == "decr_i8" || N == "decr_i16" || N == "decr_i32" ||
           N == "decr_i64";
  }
  return false;
}

bool isAssertSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "assert_eq_i1" || N == "assert_eq_i8" || N == "assert_eq_i16" ||
           N == "assert_eq_i32" || N == "assert_eq_i64";
  }
  return false;
}

bool isSWPPIntrinsic(Instruction *I) {
  return isAloadSWPPIntrinsic(I) || isIncrDecrSWPPIntrinsic(I) ||
         isAssertSWPPIntrinsic(I);
}

int getExpectedCost(Instruction *I) {
  int Cost = 0;
  if (isa<ReturnInst>(I) || isa<BranchInst>(I)) {
    // Note: terminator instructions don't need cost calculation in this case
    Cost = 0;
  } else if (isa<LoadInst>(I)) {
    // TODO: Check how to determine if pointer is stack or heap
    Cost = (20 + 30) / 2;
  } else if (isa<StoreInst>(I)) {
    Cost = (20 + 30) / 2;
  } else if (auto *CI = dyn_cast<CallInst>(I)) {
    if (isAloadSWPPIntrinsic(CI)) {
      Cost = 1;
    } else if (isSumSWPPIntrinsic(CI)) {
      Cost = 10;
    } else if (isIncrDecrSWPPIntrinsic(CI)) {
      Cost = 1;
    } else if (isAssertSWPPIntrinsic(CI)) {
      Cost = 0;
    } else {
      // Check if it is always function call
      Cost = 2 + CI->getNumOperands();
    }
  } else if (isa<ICmpInst>(I)) {
    Cost = 1;
  } else if (isa<SelectInst>(I)) {
    Cost = 1;
  } else {
    switch (I->getOpcode()) {
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::Mul:
      Cost = 1;
      break;
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      Cost = 4;
      break;
    case Instruction::Add:
    case Instruction::Sub:
      Cost = 5;
      break;
    default:
      // errs() << "Unhandled Opcode " << I->getOpcodeName() << "\n";
      // llvm_unreachable("No opcodes should be left unhandled");
      Cost = 0;
      break;
    }
  }
  return Cost;
}
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
  if (F.getName() == "oracle")
    return PreservedAnalyses::all();

  bool Changed = false;
  for (auto &BB : F) {
    MapVector<LoadInst *, Instruction *> ReplaceMap;
    std::deque<Instruction *> InstQueue;
    int InstCost = 0;

    for (auto &I : BB) {
      // Note: instructions can only be pushed upto the first non-phi node
      if (I.comesBefore(BB.getFirstNonPHI()))
        continue;

      while (!InstQueue.empty() &&
             InstCost - getExpectedCost(InstQueue.front()) > 34) {
        InstCost -= getExpectedCost(InstQueue.front());
        InstQueue.pop_front();
      }
      // TODO: instead of simplistically pushing loads into aloads,
      // we should check the cost until the first use of the loaded value
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (!InstQueue.empty() && InstCost >= 4) {
          ReplaceMap[LI] = InstQueue.front();
        }
      }

      if (canStoreMemory(&I)) {
        InstQueue.clear();
        InstCost = 0;
      } else {
        InstQueue.push_back(&I);
        InstCost += getExpectedCost(&I);
      }
    }

    SmallVector<LoadInst *, 16> ToRemove;

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
      assert(!InsertPos->comesBefore(InsertPos->getParent()->getFirstNonPHI()) &&
             "The insertion position cannot come before the first non-phi.");
      if (auto *VI = dyn_cast<Instruction>(V)) {
        if (InsertPos->getParent() == VI->getParent() &&
            InsertPos->comesBefore(VI->getNextNode())) {
          InsertPos = VI->getNextNode();
        }
      }
      CI->insertBefore(InsertPos);
      LI->replaceAllUsesWith(CI);
      ToRemove.push_back(LI);
      Changed = true;
    }

    for (auto *LI : ToRemove) {
      LI->eraseFromParent();
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPLoad2Aload", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "load2aload") {
                    FPM.addPass(Load2AloadPass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::load2aload
