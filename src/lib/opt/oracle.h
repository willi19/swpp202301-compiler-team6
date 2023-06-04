#ifndef SC_OPT_ORACLE_H
#define SC_OPT_ORACLE_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

using namespace llvm;

namespace sc::opt::oracle {
class OraclePass : public llvm::PassInfoMixin<OraclePass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};
} // namespace sc::opt::oracle

#endif // SC_LIB_OPT_ORACLE_H
