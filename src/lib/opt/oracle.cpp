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
      if (isa<CallInst>(&I))
        HasCall = false;
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
