#ifndef SC_OPT_REMOVEFREE_H
#define SC_OPT_REMOVEFREE_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::removefree {
class RemoveFreePass : public llvm::PassInfoMixin<RemoveFreePass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
} // namespace sc::opt::removefree

#endif // SC_OPT_REMOVEFREE_H
