#ifndef SC_OPT_GV2ALLOCA_H
#define SC_OPT_GV2ALLOCA_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::gv2alloca {
class GV2AllocaPass : public llvm::PassInfoMixin<GV2AllocaPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};
} // namespace sc::opt::gv2alloca

#endif
