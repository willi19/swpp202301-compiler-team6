#ifndef SC_OPT_INTRINSIC_ELIMINATE_H
#define SC_OPT_INTRINSIC_ELIMINATE_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::intrinsic_elim {
class IntrinsicEliminatePass
    : public llvm::PassInfoMixin<IntrinsicEliminatePass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};
} // namespace sc::opt::intrinsic_elim

#endif // SC_OPT_INTRINSIC_ELIMINATE_H
