#ifndef PROPAGATEINTEGEREQUALITY_H
#define PROPAGATEINTEGEREQUALITY_H

#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

class PropagateIntegerEqualityPass : public PassInfoMixin<PropagateIntegerEqualityPass>
{
  public : PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

#endif