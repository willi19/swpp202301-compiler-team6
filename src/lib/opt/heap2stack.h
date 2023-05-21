#ifndef SC_OPT_HEAP2STACK
#define SC_OPT_HEAP2STACK

#include "llvm/IR/PassManager.h"

const int MaxStackSize = 102400;
const int BoundaryDenominator = 5;
const int BoundaryNumerator = 1; // maximum usage of stack will be 
const int StackBoundary = MaxStackSize * BoundaryNumerator / BoundaryDenominator; //Stack that can be used are MaxStackSzie * 0.8
                                                                                  //Don't change malloc if StackPointer goes under the StackBoundary
namespace sc::opt::heap2stack {
class Heap2StackPass : public llvm::PassInfoMixin<Heap2StackPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};
} // namespace sc::opt::heap2stack

#endif