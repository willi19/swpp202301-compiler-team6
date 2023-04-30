#ifndef BRANCHPREDICTPASS_H
#define BRANCHPREDICTPASS_H

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "branch-predict-pass"

STATISTIC(NumBranchesChanged, "Number of branches changed");

class BranchPredictPass : public PassInfoMixin<BranchPredictPass> {
public:
    BranchPredictPass() {}
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

#endif
