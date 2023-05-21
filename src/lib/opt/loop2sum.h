#ifndef SC_OPT_LOOP2SUM_H
#define SC_OPT_LOOP2SUM_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

using namespace llvm;

namespace sc::opt::loop2sum {
class Loop2SumPass : public llvm::PassInfoMixin<Loop2SumPass> {
public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

  bool interleave(Loop &L) { return false; }
  void setDebugLocFromInst(Instruction *To, Instruction *From) {
    To->setDebugLoc(From->getDebugLoc());
  }
};
} // namespace sc::opt::loop2sum

#endif // SC_LIB_OPT_LOOP2SUM_H
