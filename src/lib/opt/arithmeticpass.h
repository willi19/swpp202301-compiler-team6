#ifndef SC_OPT_ARITHMETICPASS_H
#define SC_OPT_ARITHMETICPASS_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::arithmeticpass {
class ArithmeticPass : public llvm::PassInfoMixin<ArithmeticPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};
} // namespace sc::opt::arithmeticpass

#endif // SC_OPT_ARITHMETICPASS_H
