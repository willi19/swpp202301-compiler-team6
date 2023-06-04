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
bool HasCycleInFunction(Function *function, set<Function *> &VisitedFunctions,
                        set<Function *> &CurrentPath) {
  VisitedFunctions.insert(function);
  CurrentPath.insert(function);

  for (auto &BB : *function) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *CalledFunction = CI->getCalledFunction();

        if (CalledFunction == nullptr) // indirect function call
          continue;

        if (!VisitedFunctions.count(CalledFunction)) {
          if (HasCycleInFunction(CalledFunction, VisitedFunctions, CurrentPath))
            return true;
        } else if (CurrentPath.count(CalledFunction)) {
          return true;
        }
      }
    }
  }

  CurrentPath.erase(function);
  return false;
}
} // namespace

namespace sc::opt::functioninline {
PreservedAnalyses FunctionInlinePass::run(Module &M,
                                          ModuleAnalysisManager &MAM) {

  // Check if changed to return correct preserved analyses.
  bool Changed = false;

  // InlineFunctionInfo.
  FunctionAnalysisManager &FAM =
      MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  InlineFunctionInfo IFI(nullptr, [&](Function &F) -> AssumptionCache & {
    return FAM.getResult<AssumptionAnalysis>(F);
  });

  set<Function *> VisitedFunctions;
  vector<CallBase *> DoInline;
  for (auto &F : M) {
    set<Function *> CurrentPath;

    errs() << F.getName() << '\n';

    if (F.isDeclaration())
        continue;
    if (!VisitedFunctions.count(&F) &&
        HasCycleInFunction(&F, VisitedFunctions, CurrentPath))
        continue;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *Caller = dyn_cast<CallInst>(&I);
        if (!Caller)
          continue;

        Function *Callee = Caller->getCalledFunction();

        // errs() << Callee->getName() << ' ' << Callee->getInstructionCount() << '\n';

        if (!Callee || Callee->isDeclaration())
          continue;
        if (!(isInlineViable(*Callee).isSuccess()))
          continue;
        if (Callee->hasFnAttribute(Attribute::NoInline))
          continue;
        if (Callee->getInstructionCount() > 50)
          continue;
        
        DoInline.push_back(Caller);
      }
    }
  }

  for (auto &Caller : DoInline) {
    Changed |=
        InlineFunction(*Caller, IFI, nullptr, false, nullptr).isSuccess();
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
