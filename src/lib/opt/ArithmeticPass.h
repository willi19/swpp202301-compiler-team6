#ifndef SC_OPT_ARITHMETICPASS_H
#define SC_OPT_ARITHMETICPASS_H

#include "llvm/IR/PatternMatch.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace sc::opt::arithmeticPass {
class ArithmeticPass : public llvm::PassInfoMixin<ArithmeticPass> {
  public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
  };
} // namespace sc::opt::arithmeticPass

#endif // SC_OPT_ARITHMETICPASS_H
