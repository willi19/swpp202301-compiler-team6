#ifndef SC_OPT_HEAP2STACK
#define SC_OPT_HEAP2STACK

#include "llvm/IR/PassManager.h"

namespace sc::opt::heap2stack {
class Heap2StackPass : public llvm::PassInfoMixin<Heap2StackPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};
} // namespace sc::opt::heap2stack

#endif
