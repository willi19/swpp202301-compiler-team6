#include "heap2stack.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

using namespace llvm;
using namespace std;

namespace {
const uint64_t StackAllocSize = 102400 * 0.8;
const uint64_t StackMax = 102400;

bool hasCycleInFunction(Function *F, set<Function *> &VisitedFunctions,
                        set<Function *> &CurrentPath) {
  // check wheter there are cycle between
  // functions like A -- call --> B
  VisitedFunctions.insert(F);
  CurrentPath.insert(F);

  for (auto &BB : *F) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *CalledF = CI->getCalledFunction();
        // Indirect function call, skip it
        if (CalledF == nullptr)
          continue;

        if (!VisitedFunctions.count(CalledF)) {
          if (hasCycleInFunction(CalledF, VisitedFunctions, CurrentPath))
            return true;
        } else if (CurrentPath.count(CalledF)) {
          return true; // Found a cycle
        }
      }
    }
  }

  CurrentPath.erase(F);
  return false;
}
} // namespace

namespace sc::opt::heap2stack {
PreservedAnalyses Heap2StackPass::run(Module &M, ModuleAnalysisManager &MAM) {
  for (auto &F : M) {
    set<Function *> VisitedFunctions;
    set<Function *> CurrentPath;

    if (hasCycleInFunction(&F, VisitedFunctions, CurrentPath))
      return PreservedAnalyses::all();
  }

  auto &C = M.getContext();
  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  Function *MainFn = M.getFunction("main");
  Function *MallocFn = M.getFunction("malloc");
  Function *FreeFn = M.getFunction("free");
  Function *WriteFn = M.getFunction("write");

  if (!MallocFn)
    return PreservedAnalyses::all();

  Type *Int64Ty = Type::getInt64Ty(C);
  Type *Int8Ty = Type::getInt8Ty(C);
  Type *Int8PtrTy = Type::getInt8PtrTy(C);

  SmallVector<CallInst *, 64> MallocInsts;
  SmallVector<CallInst *, 64> FreeInsts;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          if (&F == MainFn && CI->getCalledFunction() == MallocFn)
            MallocInsts.push_back(CI);
          else if (FreeFn && CI->getCalledFunction() == FreeFn)
            FreeInsts.push_back(CI);
        }
      }
    }
  }

  // No need to optimize if there is no candidate malloc
  if (MallocInsts.empty()) {
    return PreservedAnalyses::all();
  }

  IRBuilder<> Builder(C);

  Builder.SetInsertPoint(&*MainFn->begin()->getFirstNonPHI());
  // Note: the provided backend doesn't handle array-sized allocas
  auto *BlockPtr = Builder.CreatePointerCast(
      Builder.CreateAlloca(ArrayType::get(Int8Ty, StackAllocSize)), Int8PtrTy,
      "block.ptr");
  auto *NextAllocPtr =
      Builder.CreateAlloca(Int8PtrTy, nullptr, "nextalloc.ptr");
  auto *LeftSizePtr = Builder.CreateAlloca(Int64Ty, nullptr, "leftsize.ptr");
  Builder.CreateStore(ConstantInt::get(Int64Ty, StackAllocSize), LeftSizePtr);
  Builder.CreateStore(BlockPtr, NextAllocPtr);

  for (auto &CI : MallocInsts) {
    auto *BB = CI->getParent();
    auto *SplitBB = BB->splitBasicBlock(CI, BB->getName() + ".split");
    // Split on CI, SplitBB now starts with CI.
    auto *StackBB = BasicBlock::Create(C, BB->getName() + ".stack", MainFn);
    auto *HeapBB = BasicBlock::Create(C, BB->getName() + ".heap", MainFn);

    // remove unconditional br BB->SplitBB
    BB->getTerminator()->eraseFromParent();
    Builder.SetInsertPoint(BB);

    // Transforms from:
    //
    //   malloc(AllocSize)
    //
    // to:
    //
    //   if (LeftSize >= AllocSize) {
    //     LeftSize -= AllocSize;
    //     void *Alloc = NextAlloc;
    //     NextAlloc += AllocSize;
    //     Alloc
    //   } else {
    //     malloc(AllocSize)
    //   }

    auto *AllocSize = CI->getOperand(0);
    auto *LeftSize = Builder.CreateLoad(Int64Ty, LeftSizePtr, "leftsize");
    auto *Cond =
        Builder.CreateCmp(ICmpInst::ICMP_SGE, LeftSize, AllocSize, "cond");
    Builder.CreateCondBr(Cond, StackBB, HeapBB);

    // Stack Basic Block
    Builder.SetInsertPoint(StackBB);
    auto *NewLeftSize = Builder.CreateBinOp(Instruction::Sub, LeftSize,
                                            AllocSize, "leftsize.sub");
    Builder.CreateStore(NewLeftSize, LeftSizePtr);
    auto *NextAlloc = Builder.CreateLoad(Int8PtrTy, NextAllocPtr, "nextalloc");
    auto *NewNextAlloc = Builder.CreateIntToPtr(
        Builder.CreateBinOp(Instruction::Add,
                            Builder.CreatePtrToInt(NextAlloc, Int64Ty),
                            AllocSize),
        Int8PtrTy, "nextptr.add");
    Builder.CreateStore(NewNextAlloc, NextAllocPtr);
    Builder.CreateBr(SplitBB);

    Builder.SetInsertPoint(HeapBB);
    auto *MallocPtr = Builder.CreateCall(MallocFn, {AllocSize}, "malloc");
    Builder.CreateBr(SplitBB);

    Builder.SetInsertPoint(&*SplitBB->begin());
    auto *PHI = Builder.CreatePHI(Int8PtrTy, 2);
    PHI->addIncoming(NextAlloc, StackBB);
    PHI->addIncoming(MallocPtr, HeapBB);

    CI->replaceAllUsesWith(PHI);
    CI->eraseFromParent();
  }

  FAM.invalidate(*MainFn, PreservedAnalyses::none());
  auto &MainDT = FAM.getResult<DominatorTreeAnalysis>(*MainFn);
  PromoteMemToReg({NextAllocPtr, LeftSizePtr}, MainDT);

  for (auto &CI : FreeInsts) {
    auto *BB = CI->getParent();
    auto *SplitBB = BB->splitBasicBlock(CI, BB->getName() + ".split");
    auto *FreeBB =
        BasicBlock::Create(C, BB->getName() + ".free", BB->getParent());

    // remove unconditional br BB->SplitBB
    BB->getTerminator()->eraseFromParent();
    Builder.SetInsertPoint(BB);

    // Transforms from:
    //
    //   free(AllocPtr)
    //
    // to:
    //
    //   if (AllocPtr >= StackMax)
    //     free(AllocPtr);

    auto *AllocPtr = CI->getOperand(0);
    auto *Cond = Builder.CreateCmp(ICmpInst::ICMP_SGE,
                                   Builder.CreatePtrToInt(AllocPtr, Int64Ty),
                                   ConstantInt::get(Int64Ty, StackMax), "cond");
    Builder.CreateCondBr(Cond, FreeBB, SplitBB);

    Builder.SetInsertPoint(FreeBB);
    Builder.CreateCall(FreeFn, {AllocPtr});
    Builder.CreateBr(SplitBB);

    CI->eraseFromParent();
  }

  return PreservedAnalyses::none();
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
