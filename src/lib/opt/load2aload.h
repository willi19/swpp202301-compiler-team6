#ifndef SC_OPT_LOAD2ALOAD_H
#define SC_OPT_LOAD2ALOAD_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::load2aload {
class Load2AloadPass : public llvm::PassInfoMixin<Load2AloadPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};
} // namespace sc::opt::load2aload

#endif // SC_LIB_OPT_LOAD2ALOAD_H
