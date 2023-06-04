#ifndef SC_OPT_INCRDECR_H
#define SC_OPT_INCRDECR_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace sc::opt::incrdecr {
class IncrDecrPass : public PassInfoMixin<IncrDecrPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // namespace sc::opt::incrdecr

#endif // SC_LIB_OPT_INCRDECR_H
