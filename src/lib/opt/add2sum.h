#ifndef SC_OPT_ADD2SUM_H
#define SC_OPT_ADD2SUM_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace sc::opt::add2sum {
class Add2SumPass : public PassInfoMixin<Add2SumPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // namespace sc::opt::add2sum

#endif // SC_LIB_OPT_ADD2SUM_H
