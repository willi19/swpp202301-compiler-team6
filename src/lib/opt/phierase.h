#ifndef SC_OPT_PHIERASE_H
#define SC_OPT_PHIERASE_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::phierase {
class PhierasePass : public llvm::PassInfoMixin<PhierasePass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &, llvm::FunctionAnalysisManager &);
};
} // namespace sc::opt::phierase
#endif // SC_OPT_REMOVEFREE_H
