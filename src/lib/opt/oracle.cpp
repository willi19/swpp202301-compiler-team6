#include "oracle.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

using namespace llvm;
using namespace std;

namespace {
const int MaxOracleInstCount = 45;

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

bool isMallocSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "malloc";
  }
  return false;
}

bool isFreeSWPPIntrinsic(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto N = CI->getCalledFunction()->getName();
    return N == "free";
  }
  return false;
}

bool isSWPPIntrinsic(Instruction *I) {
  return isAloadSWPPIntrinsic(I) || isIncrDecrSWPPIntrinsic(I) ||
         isAssertSWPPIntrinsic(I) || isMallocSWPPIntrinsic(I) ||
         isFreeSWPPIntrinsic(I);
}

int getMemAccessCount(BasicBlock *BB) {
  int Count = 0;
  for (auto &I : *BB) {
    if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
      Count++;
    }
  }
  return Count;
}

int getInstCount(Loop *L) {
  int Count = 0;
  for (auto *BB : L->blocks()) {
    for (auto &I : *BB)
      Count++;
  }
  return Count;
}

bool isEligibleForOracle(Loop *L) {
  bool HasCall = false;
  bool HasMemAccess = false;
  int InstCount = 0;
  for (auto *BB: L->blocks()) {
    for (auto &I: *BB) {
      InstCount++;
      if (isa<CallInst>(&I) && !isSWPPIntrinsic(&I))
        HasCall = true;
      else if (isa<LoadInst>(&I) || isa<StoreInst>(&I))
        HasMemAccess = true;
    }
  }

  return !HasCall && InstCount < MaxOracleInstCount && HasMemAccess;
}
} // namespace

namespace sc::opt::oracle {
PreservedAnalyses OraclePass::run(Module &M, ModuleAnalysisManager &MAM) {
  auto &C = M.getContext();
  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  assert(M.getFunction("oracle") == nullptr &&
         "Oracle function already exists.");

  Loop *L = nullptr;
  int EstimatedOutputCount = 0;
  int MaxCost = 0;

  for (Function &F : M) {
    if (F.isDeclaration())
      continue;

    auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
    auto &LI = FAM.getResult<LoopAnalysis>(F);
    auto &BFI = FAM.getResult<BlockFrequencyAnalysis>(F);

    for (auto *LL : LI.getLoopsInPreorder()) {
      if (!isEligibleForOracle(LL))
        continue;

      int Cost = 0;
      for (auto *BB : LL->getBlocks()) {
        Cost += getMemAccessCount(BB) * BFI.getBlockFreq(BB).getFrequency();
      }

      if (Cost >= MaxCost) {
        // If L contains LL, it is always beneficial to oraclize L instead of LL
        if (L && L->contains(LL))
          continue;
        CodeExtractor Extractor(DT, *LL);
        CodeExtractorAnalysisCache CEAC(F);

        SetVector<Value *> Inputs, Outputs, SinkCands, HoistCands;
        BasicBlock *ExitBlock = nullptr;
        Extractor.findAllocas(CEAC, SinkCands, HoistCands, ExitBlock);
        Extractor.findInputsOutputs(Inputs, Outputs, SinkCands);
        if (Cost > MaxCost || Outputs.size() < EstimatedOutputCount) {
          L = LL;
          MaxCost = Cost;
          EstimatedOutputCount = Outputs.size();
        }
      }
    }
  }

  if (!L)
    return PreservedAnalyses::all();

  auto &F = *L->getHeader()->getParent();
  auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  CodeExtractor Extractor(DT, *L);
  CodeExtractorAnalysisCache CEAC(F);
  SetVector<Value *> Inputs, Outputs;
  Function *OracleF = Extractor.extractCodeRegion(CEAC, Inputs, Outputs);
  if (!OracleF)
    return PreservedAnalyses::all();

  // TODO: bail out if Outputs.size() > 16
  OracleF->setName("oracle");

  return PreservedAnalyses::none();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPOracle", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "oracle") {
                    MPM.addPass(OraclePass());
                    return true;
                  }
                  return false;
                });
          }};
}
}; // namespace sc::opt::oracle
