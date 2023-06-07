#ifndef SC_OPT_REORDERMEM_H
#define SC_OPT_REORDERMEM_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::reordermem {
class ReorderMemPass : public llvm::PassInfoMixin<ReorderMemPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &, llvm::FunctionAnalysisManager &);
};
} // namespace sc::opt::phierase
#endif // SC_OPT_REORDERMEM_H
