#include "heap2stack.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
using namespace std;

namespace {
bool HasCycleInFunction(
    Function *function, set<Function *> &VisitedFunctions,
    set<Function *> &CurrentPath) { // check wheter there are cycle between
                                    // functions like A -- call --> B
  VisitedFunctions.insert(function);
  CurrentPath.insert(function);

  for (auto &BB : *function) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *CalledFunction = CI->getCalledFunction();

        if (CalledFunction == nullptr) {
          // Indirect function call, skip it
          continue;
        }

        if (!VisitedFunctions.count(CalledFunction)) {
          if (HasCycleInFunction(CalledFunction, VisitedFunctions, CurrentPath))
            return true;
        } else if (CurrentPath.count(CalledFunction)) {
          return true; // Found a cycle
        }
      }
    }
  }

  CurrentPath.erase(function);
  return false;
}
} // namespace

namespace sc::opt::heap2stack {
PreservedAnalyses Heap2StackPass::run(Module &M, ModuleAnalysisManager &MAM) {
  for (auto &F : M) {
    set<Function *> VisitedFunctions;
    set<Function *> CurrentPath;

    if (HasCycleInFunction(&F, VisitedFunctions, CurrentPath)) {
      return PreservedAnalyses::all();
    }
  }

  LLVMContext &Context = M.getContext();

  Function *MainFn, *MallocFn, *FreeFn;
  FunctionType *MainTy, *MallocTy, *FreeTy;

  MallocFn = MainFn = FreeFn = NULL;
  // Fetch the corresponding functions from the module
  for (auto &F : M) {
    if (F.getName() == "malloc")
      MallocFn = &F, MallocTy = dyn_cast<FunctionType>(F.getValueType());
    else if (F.getName() == "free")
      FreeFn = &F, FreeTy = dyn_cast<FunctionType>(F.getValueType());
    else if (F.getName() == "main")
      MainFn = &F, MainTy = dyn_cast<FunctionType>(F.getValueType());
  }

  Type *Int64Ty = Type::getInt64Ty(Context);
  llvm::FunctionCallee DecrSpFn =
      M.getOrInsertFunction("$decr_sp", Int64Ty, Int64Ty);

  vector<CallInst *> MallocInsts;
  vector<CallInst *> FreeInsts;
  // Find malloc in main and free
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          if (CI->getCalledFunction()->getName() == "malloc" && &F == MainFn)
            MallocInsts.push_back(CI);
          if (CI->getCalledFunction()->getName() == "free")
            FreeInsts.push_back(CI);
        }
      }
    }
  }
  // No need to optimize if there is no candidate malloc
  if (MallocInsts.empty()) {
    return PreservedAnalyses::all();
  }
  /* Convert all candidate malloc(ReqSize) to:
   * if (SP - ReqSize >= StackBoundary) (Remaining space after malloc >
   * StackBoundary) decr_sp(ReqSize); else malloc(ReqSize); */
  for (auto &CI : MallocInsts) {
    BasicBlock *BB = CI->getParent();
    BasicBlock *SplitBB = BB->splitBasicBlock(CI, "div." + BB->getName());
    // Split base on CI, orig block doesn't contain
    // CU while next block start with CI.
    BasicBlock *StackBB =
        BasicBlock::Create(Context, "stack." + BB->getName(), MainFn);
    BasicBlock *HeapBB =
        BasicBlock::Create(Context, "heap." + BB->getName(), MainFn);
    /*BB
      |      \
    stackBB  heapBB
       |     /
    divBB(start with CI(Malloc))
    */

    Value *AllocSize = CI->getOperand(0);

    auto *LoadCurSp = CallInst::Create(
        DecrSpFn, ArrayRef<Value *>{ConstantInt::get(Int64Ty, 0, true)},
        "cur.sp"); // load current stackpointer
    auto *LeftSpace =
        BinaryOperator::CreateSub(LoadCurSp, AllocSize, "space.left");
    auto *CmpSP = new ICmpInst(ICmpInst::ICMP_SGE, LeftSpace,
                               ConstantInt::get(Int64Ty, StackBoundary),
                               "alloca_possible"); // SpaceLeft > MinumumSpace
    // True: stackBB, False: heapBB
    auto *BrSP = BranchInst::Create(StackBB, HeapBB, CmpSP);
    BB->getInstList().pop_back(); // Remove BranchInst BB->SplitBB
    BB->getInstList().push_back(LoadCurSp);
    BB->getInstList().push_back(LeftSpace);
    BB->getInstList().push_back(CmpSP);
    BB->getInstList().push_back(BrSP);

    // Stack Basic Block
    auto *DecrSp = CallInst::Create(DecrSpFn, {AllocSize}, "by.alloca");
    llvm::CastInst *SpCast =
        llvm::CastInst::CreateBitOrPointerCast(DecrSp, CI->getType(), "");
    auto *BackFromStack = BranchInst::Create(SplitBB);
    StackBB->getInstList().push_back(DecrSp);        // move sp by decr_sp
    StackBB->getInstList().push_back(SpCast);        // change to type of malloc
    StackBB->getInstList().push_back(BackFromStack); // Branch back to divBB

    // Heap Basic Block
    auto *Malloc =
        CallInst::Create(MallocTy, MallocFn, {AllocSize}, "by.malloc");
    auto *BackFromHeap = BranchInst::Create(SplitBB);
    HeapBB->getInstList().push_back(Malloc); // malloc(size)
    HeapBB->getInstList().push_back(BackFromHeap);

    // Splited Basic Block; Absolb allocation using a PHI node
    auto *Phi = PHINode::Create(CI->getType(), 2, "alloc." + CI->getName());
    Phi->addIncoming(SpCast, StackBB);
    Phi->addIncoming(Malloc, HeapBB);
    SplitBB->getInstList().push_front(Phi);

    CI->replaceAllUsesWith(Phi);
    CI->eraseFromParent();
  }

  /* Convert all candidate free(ptr) to: (can be either STACK or HEAP alloc)
   * if ((int) ptr >= MaxStackSize) free(ptr)
   * else ; // do nothing */
  for (auto &CI : FreeInsts) {
    /*BB
      |      \
      |   FreeBB
      |     /
    divBB(start with CI(Free))
    */
    BasicBlock *BB = CI->getParent();
    BasicBlock *SplitBB = BB->splitBasicBlock(CI, "div.free." + BB->getName());
    Function *CurFn = BB->getParent(); // Function to add splited BB
    BasicBlock *FreeBB =
        BasicBlock::Create(Context, "free." + BB->getName(), CurFn);
    Value *AllocPtr = CI->getOperand(0);

    // Basic Block Conditional Branch Update
    auto *AllocPos = new PtrToIntInst(AllocPtr, Int64Ty, "sp.as.int");
    auto *CmpSP =
        new ICmpInst(ICmpInst::ICMP_SGE, AllocPos,
                     ConstantInt::get(Int64Ty, MaxStackSize), "cmp.sp");
    auto *BrSP = BranchInst::Create(FreeBB, SplitBB, CmpSP);
    BB->getInstList().pop_back();          // Remove BranchInst BB->SplitBB
    BB->getInstList().push_back(AllocPos); // (int) ptr
    BB->getInstList().push_back(CmpSP);    // (int) ptr >= STACK_BOUNDARY
    BB->getInstList().push_back(BrSP);     // True: FreeBB, False: SplitBB

    // FreeBB for actual free in HEAP area
    auto *Free = CallInst::Create(FreeTy, FreeFn, {AllocPtr});
    auto *Back = BranchInst::Create(SplitBB);
    FreeBB->getInstList().push_back(Free);
    FreeBB->getInstList().push_back(Back);

    CI->eraseFromParent();
  }

  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPHeap2Stack", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "heap2stack") {
                    MPM.addPass(Heap2StackPass());
                    return true;
                  }
                  return false;
                });
          }};
}
}; // namespace sc::opt::heap2stack
