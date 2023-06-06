#ifndef SC_OPT_ORACLE_H
#define SC_OPT_ORACLE_H

#include "llvm/IR/PassManager.h"

namespace sc::opt::oracle {
class OraclePass : public llvm::PassInfoMixin<OraclePass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};
} // namespace sc::opt::oracle

#endif // SC_LIB_OPT_ORACLE_H
