#include "heap2stack.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"

using namespace llvm;
using namespace std;

namespace {
bool hasCycleInFunction(Function *function, std::set<Function *> &visitedFunctions, std::set<Function *> &currentPath) {
  visitedFunctions.insert(function);
  currentPath.insert(function);

  for (auto &BB : *function) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *calledFunction = CI->getCalledFunction();

        if (calledFunction == nullptr) {
          // Indirect function call, skip it
          continue;
        }

        if (!visitedFunctions.count(calledFunction)) {
          if (hasCycleInFunction(calledFunction, visitedFunctions, currentPath))
            return true;
        } else if (currentPath.count(calledFunction)) {
          return true;  // Found a cycle
        }
      }
    }
  }

  currentPath.erase(function);
  return false;
}
} // namespace 

namespace sc::opt::heap2stack {
PreservedAnalyses Heap2Stack::run(Module &M, ModuleAnalysisManager &MAM) {
  
  bool hasCycle = false;

  for (auto &F : M) {
    std::set<Function *> visitedFunctions;
    std::set<Function *> currentPath;

    if (hasCycleInFunction(&F, visitedFunctions, currentPath)) {
      hasCycle = true;
    }
  }

  if (hasCycle)
    return PreservedAnalyses::all();
  
  LLVMContext &Context = M.getContext();
  Type *Int64Ty = Type::getInt64Ty(Context);
  Type *Int1Ty = Type::getInt1Ty(Context);
  Type *Int8PtrTy = Type::getInt8PtrTy(Context);

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

  // Main function must exist
  assert(MainFn != NULL && "No main function in the IR module!");
  
  llvm::FunctionCallee DecrSp = M.getOrInsertFunction("$decr_sp", Int64Ty, Int64Ty);
  

  vector<CallInst*> MallocInsts;
  vector<CallInst*> FreeInsts;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          if (CI->getCalledFunction()->getName() == "malloc"&& &F == MainFn) MallocInsts.push_back(CI);
          if (CI->getCalledFunction()->getName() == "free") FreeInsts.push_back(CI);
        }
      }
    }
  }

  // No need to optimize if there is no candidate malloc
  if (MallocInsts.empty()) {
    return PreservedAnalyses::all();
  }

  /* Convert all candidate malloc(size) to:
   * if (sp - size >= STACK_DANGEROUS_REGION)
   *   decr_sp(size);
   * else malloc(size); */
  for (auto &CI : MallocInsts) {
    BasicBlock *BB = CI->getParent();
    BasicBlock *SplitBB = BB->splitBasicBlock(CI, "div." + BB->getName()); //Split base on CI, orig block doesn't contain CU while next block start with CI.
    BasicBlock *AllocaBB = BasicBlock::Create(Context, "alloca." + BB->getName(), MainFn);
    BasicBlock *MallocBB = BasicBlock::Create(Context, "malloc." + BB->getName(), MainFn);
    /*BB
      |      \
    AllocaBB  MallocBB
       |     /
    divBB(CI==Malloc) 
    */

    Value *AllocSize = CI->getOperand(0);

    // Basic Block Conditional Branch Update
    // CurSp * 
    //ConstantInt *Zero = ConstantInt::get(Int64Ty, 0, true);
    //Value *Args[] = {Zero};
    //auto *LoadCurSp = CallInst::Create(DecrSp, ArrayRef<Value *>(Args), ""); //Load cursp by sp = sub sp 0
    auto *LoadCurSp = CallInst::Create(DecrSp, ArrayRef<Value *>{ConstantInt::get(Int64Ty, 0, true)}, "");
    auto *LetfSpace = BinaryOperator::CreateMul(LoadCurSp, ConstantInt::get(Int64Ty, BOUNDARY_NUMERATOR), "");
    auto *RequiredSpace = BinaryOperator::CreateMul(AllocSize, ConstantInt::get(Int64Ty, BOUNDARY_DENOMINATOR), "");
    
    auto *CmpSP = new ICmpInst(ICmpInst::ICMP_SGE, LetfSpace,RequiredSpace, "alloca_possible"); //Leftspace > RequiredSpace
    auto *BrSP = BranchInst::Create(AllocaBB, MallocBB, CmpSP);
    
    BB->getInstList().pop_back();           // Remove BranchInst BB->SplitBB
    BB->getInstList().push_back(LoadCurSp); // 
    BB->getInstList().push_back(LetfSpace);     // 
    BB->getInstList().push_back(RequiredSpace);
    BB->getInstList().push_back(CmpSP);     // 
    BB->getInstList().push_back(BrSP);      // True: AllocaBB, False: MallocBB

    // Alloca Basic Block
    auto *Alloca = CallInst::Create(DecrSp, {AllocSize}, "by.alloca");
    llvm::CastInst *AllocaCast = llvm::CastInst::CreateBitOrPointerCast(Alloca, CI->getType(), "");
    auto *BackFromAllocaBytes = BranchInst::Create(SplitBB);
    AllocaBB->getInstList().push_back(Alloca);
    AllocaBB->getInstList().push_back(AllocaCast);
    AllocaBB->getInstList().push_back(BackFromAllocaBytes);
            
    // Malloc Basic Block
    auto *Malloc = CallInst::Create(MallocTy, MallocFn, {AllocSize}, "by.malloc");
    auto *BackFromMalloc = BranchInst::Create(SplitBB);
    MallocBB->getInstList().push_back(Malloc); // malloc(size)
    MallocBB->getInstList().push_back(BackFromMalloc);

    // Splited Basic Block; Absolb allocation using a PHI node
    auto *Phi = PHINode::Create(CI->getType(), 2, "allocation." + CI->getName());
    Phi->addIncoming(AllocaCast, AllocaBB);
    Phi->addIncoming(Malloc, MallocBB);
    SplitBB->getInstList().push_front(Phi);

    CI->replaceAllUsesWith(Phi);
    CI->eraseFromParent();
  }

  /* Convert all candidate free(ptr) to: (can be either STACK or HEAP alloc)
   * if ((int) ptr >= STACK_BOUNDARY) free(ptr)
   * else ; // do nothing */
  for (auto &CI : FreeInsts) {
    BasicBlock *BB = CI->getParent();
    BasicBlock *SplitBB = BB->splitBasicBlock(CI, "div." + BB->getName());
    BasicBlock *FreeBB = BasicBlock::Create(Context, "free." + BB->getName(), MainFn);
    Value *AllocPtr = CI->getOperand(0);

    // Basic Block Conditional Branch Update
    auto *AllocPos = new PtrToIntInst(AllocPtr, Int64Ty, "sp.as.int");
    auto *CmpSP = new ICmpInst(ICmpInst::ICMP_SGE, AllocPos,
                        ConstantInt::get(Int64Ty, STACK_BOUNDARY), "cmp.sp");
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
}}; //sc::opt::heap2stack

