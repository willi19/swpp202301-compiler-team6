#include "functioninline.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <set>
#include <vector>

using namespace llvm;
using namespace std;

namespace {
bool hasCycleInFunction(Function *F, set<Function *> &VisitedFunctions,
                        set<Function *> &CurrentPath) {
  // check wheter there are cycle between
  // functions like A -- call --> B
  VisitedFunctions.insert(F);
  CurrentPath.insert(F);

  for (auto &BB : *F) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *CalledF = CI->getCalledFunction();
        // Indirect function call, skip it
        if (CalledF == nullptr)
          continue;

        if (!VisitedFunctions.count(CalledF)) {
          if (hasCycleInFunction(CalledF, VisitedFunctions, CurrentPath))
            return true;
        } else if (CurrentPath.count(CalledF)) {
          return true; // Found a cycle
        }
      }
    }
  }

  CurrentPath.erase(F);
  return false;
}
} // namespace

namespace sc::opt::functioninline {
PreservedAnalyses FunctionInlinePass::run(Module &M,
                                          ModuleAnalysisManager &MAM) {

  bool Changed = false;

  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  auto GetAssumptionCache = [&](Function &F) -> AssumptionCache & {
    return FAM.getResult<AssumptionAnalysis>(F);
  };
  InlineFunctionInfo IFI(nullptr, GetAssumptionCache);

  set<Function *> VisitedFunctions;
  vector<CallBase *> InlinableCallers;
  for (auto &F : M) {
    set<Function *> CurrentPath;

    if (F.isDeclaration())
      continue;

    if (!VisitedFunctions.count(&F) &&
        hasCycleInFunction(&F, VisitedFunctions, CurrentPath))
      continue;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *Caller = dyn_cast<CallInst>(&I);
        if (!Caller)
          continue;

        Function *Callee = Caller->getCalledFunction();
        if (!Callee || Callee->isDeclaration())
          continue;
        if (!(isInlineViable(*Callee).isSuccess()))
          continue;
        if (Callee->hasFnAttribute(Attribute::NoInline))
          continue;
        if (Callee->getInstructionCount() > 50)
          continue;

        InlinableCallers.push_back(Caller);
      }
    }
  }

  for (auto *Caller : InlinableCallers) {
    Changed |=
        InlineFunction(*Caller, IFI, nullptr, false, nullptr).isSuccess();
  }

  // Allows GlobalDCE to remove dead functions that are inlined.
  for (auto &F : M) {
    if (!F.isDeclaration() && F.getName() != "main") {
      F.setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPFunctionInlinePass", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "functioninline") {
                    MPM.addPass(FunctionInlinePass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::functioninline
