#include "phierase.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <vector>

using namespace llvm;
using namespace std;

namespace sc::opt::phierase {
PreservedAnalyses PHIErasePass::run(Function &F, FunctionAnalysisManager &FAM) {
  vector<BasicBlock *> cands;
  for (auto &BB : F) {
    if (&(*BB.begin()) == BB.getFirstNonPHI() ||
        !isa<ReturnInst>(BB.getTerminator()))
      continue;
    cands.push_back(&BB);
  }

  if (cands.empty())
    return PreservedAnalyses::all();

  for (BasicBlock *BB : cands) {
    for (auto predItr = pred_begin(BB), predEnd = pred_end(BB);
         predItr != predEnd;) {
      BasicBlock *predecessor = *predItr;
      predItr++;

      ValueToValueMapTy VMap; // create an empty value map
      BasicBlock *ClonedBB = CloneBasicBlock(BB, VMap); // clone the basic block
      for (auto instItr = ClonedBB->begin(), instEnd = ClonedBB->end();
           instItr != instEnd;) {
        Instruction *I = &(*instItr);
        instItr++;
        if (auto *PN = dyn_cast<PHINode>(I)) {
          // for PHI nodes, replace incoming blocks
          for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            if (PN->getIncomingBlock(i) == predecessor) {
              auto instructionIterator = I->getIterator();
              ReplaceInstWithValue(ClonedBB->getInstList(), instructionIterator,
                                   PN->getOperand(i));
              break;
            }
          }
        } else {

          for (Use &U : I->operands()) {
            Value *oldValue = U.get();
            if (Value *newValue = VMap.lookup(oldValue)) {
              U.set(newValue);
            }
          }
        }
      }

      ClonedBB->insertInto(&F, BB);
      predecessor->getTerminator()->replaceSuccessorWith(BB, ClonedBB);
      MergeBlockIntoPredecessor(ClonedBB); // Function from simplifyCFG
    }

    // delete or detach BB from F
    BB->eraseFromParent();
  }

  return PreservedAnalyses::none();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPPhierasePass", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "phierase") {
                    FPM.addPass(PHIErasePass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::phierase
