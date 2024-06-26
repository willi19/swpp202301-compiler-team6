#ifndef SC_OPT_FUNCTIONINLINE_H
#define SC_OPT_FUNCTIONINLINE_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::functioninline {
class FunctionInlinePass : public llvm::PassInfoMixin<FunctionInlinePass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
} // namespace sc::opt::functioninline

#endif // SC_OPT_FUNCTIONINLINE_H
