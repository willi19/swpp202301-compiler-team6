#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "branchpredict.h"

using namespace llvm;

namespace sc::opt::branchpredict {
PreservedAnalyses BranchPredictPass::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  auto &BPI = FAM.getResult<BranchProbabilityAnalysis>(F);
  auto &TTI = FAM.getResult<TargetIRAnalysis>(F);
  bool Changed = false;
  for (auto &BB : F) {
    auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
    if (BI && BI->isConditional()) {
      // Get the true and false successors of the branch
      auto *TrueBB = BI->getSuccessor(0);
      auto *FalseBB = BI->getSuccessor(1);

      // Get the branch probabilities for each successor
      auto TrueProb = BPI.getEdgeProbability(&BB, TrueBB);
      auto FalseProb = BPI.getEdgeProbability(&BB, FalseBB);

      // Check if the true branch has higher probability than the false branch
      if (TrueProb > FalseProb) {
        // Swap the successors and negate the condition
        BI->setSuccessor(0, FalseBB);
        BI->setSuccessor(1, TrueBB);

        // Get the condition of the branch
        auto *Cond = BI->getCondition();

        // Check if it is an icmp instruction
        if (auto *ICI = dyn_cast<ICmpInst>(Cond)) {
          // Get the inverse predicate of the icmp
          auto InvPred = ICI->getInversePredicate();

          // Set the predicate of the icmp to the inverse predicate
          ICI->setPredicate(InvPred);
        } else {
          // Negate the condition with an xor
          BI->setCondition(BinaryOperator::CreateNot(Cond, "", BI));
        }

        Changed = true;
      }
    }
  }
  return (Changed ? PreservedAnalyses::none() : PreservedAnalyses::all());
}
} // namespace sc::opt::branchpredict
