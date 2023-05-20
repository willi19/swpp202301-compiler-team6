#ifndef MEM_USE_OPTIMIZATION
#define MEM_USE_OPTIMIZATION

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

const std::string AllocaBytesFnName = "$decr_sp";

const int STACK_BOUNDARY = 102400;
const int BOUNDARY_DENOMINATOR = 5;
const int BOUNDARY_NUMERATOR = 4;

namespace sc::opt::heap2stack {
class Heap2Stack : public PassInfoMixin<Heap2Stack> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};
}
#endif