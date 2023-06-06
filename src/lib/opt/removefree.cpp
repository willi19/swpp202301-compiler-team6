#include "removefree.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <queue>
#include <unordered_set>

using namespace llvm;

namespace sc::opt::removefree {
PreservedAnalyses RemoveFreePass::run(Module &M, ModuleAnalysisManager &MAM) {

  // Assume F is a pointer to the main function
  Function *F = M.getFunction("main");

  bool Changed = false;
  for (auto &BB : *F) {
    if (isa<ReturnInst>(BB.getTerminator())) {
      for (auto I = BB.rbegin(), E = BB.rend(); I != E;) {
        // increase iterator first to avoid invalidation
        if (auto *CB = dyn_cast<CallBase>(&*(I++))) {
          Function *calledFunction = CB->getCalledFunction();
          if (calledFunction->getName().equals("free")) {
            CB->eraseFromParent();
            Changed = true;
          } else {
            break;
          }
        }
      }
      break;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPRemoveFreePass", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "removefree") {
                    MPM.addPass(RemoveFreePass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::removefree
