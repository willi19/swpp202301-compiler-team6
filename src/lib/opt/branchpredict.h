#ifndef SC_OPT_BRANCHPREDICTPASS_H
#define SC_OPT_BRANCHPREDICTPASS_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::branchpredict {
class BranchPredictPass : public llvm::PassInfoMixin<BranchPredictPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};
} // namespace sc::opt::branchpredict

#endif // SC_OPT_BRANCHPREDICTPASS_H
