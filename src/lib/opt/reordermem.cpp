#include "reordermem.h"

#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

    bool canStoreMemory(Instruction *I) {
        if (isa<StoreInst>(I)) {
            return true;
        }
        if (auto *Call = dyn_cast<CallBase>(I)) {
            return !Call->onlyReadsMemory() && !Call->doesNotAccessMemory();
        }
        if (auto *Intrin = dyn_cast<IntrinsicInst>(I)) {
            return Intrin->mayHaveSideEffects();
        }
        return false;
    }
} // namespace

namespace sc::opt::reordermem {
    PreservedAnalyses ReorderMemPass::run(Function &F,
                                          FunctionAnalysisManager &FAM) {
        bool Changed = false;
        for (auto &BB : F) {
            Instruction *I = &BB.front();
            while (I) {
                auto Next = I->getNextNode();
                if (I->comesBefore(BB.getFirstNonPHI())) {
                    I = Next;
                    continue;
                }

                if (auto *LI = dyn_cast<LoadInst>(I)) {
                    // Value *result = LI;
                    // Instruction* firstUse = nullptr;
                    // for (User *user : result->users()) {
                    //     if (Instruction* userInstruction = dyn_cast<Instruction>(user)) {
                    //         if (userInstruction->getParent() == &BB) {
                    //             if (!firstUse || userInstruction->comesBefore(firstUse)) {
                    //                 firstUse = userInstruction;
                    //             }
                    //         }
                    //     }
                    // }
                    // if (!firstUse || firstUse->getParent() != &BB)
                    //     firstUse = BB.getTerminator();
                    
                    // int InstCost = 0;
                    // int threshold = 34;
                    // Value* addressOperand = LI->getPointerOperand();
                    // Constant* constantAddress = dyn_cast<Constant>(addressOperand);
                    // if (constantAddress) {
                    //     if (ConstantInt* constantInt = dyn_cast<ConstantInt>(constantAddress)) {
                    //         // Extract the underlying value from the ConstantInt
                    //         int64_t addressValue = constantInt->getSExtValue();
                    //         // Compare the address value with the threshold
                    //         if (addressValue >= 204800) {
                    //             threshold = 24;
                    //         }
                    //     }
                    // }

                    Instruction *InsertPos = nullptr;
                    Value *V = LI->getPointerOperand();
                    auto *VI = dyn_cast<Instruction>(V);

                    // for (BasicBlock::iterator it = firstUse->getIterator(), begin = BB.begin(); it != begin; ) {
                    for (BasicBlock::iterator it = I->getIterator(), begin = BB.begin(); it != begin; ) {
                        Instruction* currentInstruction = &(*(--it));
                        
                        // if (currentInstruction == I)
                        //     continue;
                        
                        bool reachCeiling = canStoreMemory(currentInstruction) || currentInstruction->comesBefore(BB.getFirstNonPHI()) || (VI && (&BB == VI->getParent()) && currentInstruction->comesBefore(VI->getNextNode()));                            

                        if (reachCeiling) {
                            InsertPos = currentInstruction->getNextNode();
                            // if (InstCost > 4) {
                            //     InsertPos = currentInstruction->getNextNode();
                            // }
                            break;
                        }

                        /*
                        InstCost += getExpectedCost(currentInstruction);
                        if (InstCost >= threshold) {
                            InsertPos = currentInstruction;
                            break;
                        }
                        */
                    }

                    if (!InsertPos) {
                        I = Next;
                        continue;
                    }
                    assert(!InsertPos->comesBefore(InsertPos->getParent()->getFirstNonPHI()) &&
                           "The insertion position cannot come before the first non-phi.");
                    if (VI && InsertPos->getParent() == VI->getParent() &&
                        InsertPos->comesBefore(VI->getNextNode())) {
                        assert(false && "hey where is it?");
                    }
                    // if (firstUse->comesBefore(InsertPos)) {
                    //     assert(false && "wtf");
                    // }

                    LI->moveBefore(InsertPos);

                    // Type *Ty = LI->getType();
                    // FunctionCallee Aload = nullptr;
                    // auto *M = F.getParent();
                    // auto &C = F.getContext();
                    // Instruction* ArgBitCast = nullptr;
                    // if (Ty->isIntegerTy(8)) {
                    //     auto *FTy = FunctionType::get(Type::getInt8Ty(C),
                    //                                   {Type::getInt8PtrTy(C)}, false);
                    //     Aload = M->getOrInsertFunction("aload_i8", FTy);
                    // } else if (Ty->isIntegerTy(16)) {
                    //     auto *FTy = FunctionType::get(Type::getInt16Ty(C),
                    //                                   {Type::getInt16PtrTy(C)}, false);
                    //     Aload = M->getOrInsertFunction("aload_i16", FTy);
                    // } else if (Ty->isIntegerTy(32)) {
                    //     auto *FTy = FunctionType::get(Type::getInt32Ty(C),
                    //                                   {Type::getInt32PtrTy(C)}, false);
                    //     Aload = M->getOrInsertFunction("aload_i32", FTy);
                    // } else if (Ty->isIntegerTy(64)) {
                    //     auto *FTy = FunctionType::get(Type::getInt64Ty(C),
                    //                                   {Type::getInt64PtrTy(C)}, false);
                    //     Aload = M->getOrInsertFunction("aload_i64", FTy);
                    // } else if (auto* ElemTy = Ty->getNonOpaquePointerElementType()) {
                    //     auto* FTy = FunctionType::get(Type::getInt64Ty(C), {Type::getInt64PtrTy(C)}, false);
                    //     Aload = M->getOrInsertFunction("aload_i64", FTy);
                    //     ArgBitCast = new BitCastInst(V, Type::getInt64PtrTy(C));
                    // } else {
                    //     assert(false && "Invalid to specification");
                    // }
                    
                    // CallInst *CI = CallInst::Create(Aload, (ArgBitCast ? ArgBitCast : V));
                    // // CI->print(errs());
                    // CI->insertBefore(InsertPos);
                    // if (ArgBitCast) {
                    //     ArgBitCast->insertBefore(CI);
                    //     Instruction *RetToPtr = new IntToPtrInst(CI, Ty);
                    //     RetToPtr->insertAfter(CI);
                    //     LI->replaceAllUsesWith(RetToPtr);
                    // } else {
                    //     LI->replaceAllUsesWith(CI);
                    // }
                    // Next = LI->getNextNode();
                    // LI->eraseFromParent();
                    
                    // if (firstUse->comesBefore(CI)) {
                    //     assert(false && "wtf");
                    // }
                    
                    Changed = true;
                }

                I = Next;
            }
        }
        
        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

    extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
        return {LLVM_PLUGIN_API_VERSION, "SWPPReorderMemPass", "v0.1",
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                                                   [](StringRef Name, FunctionPassManager &FPM,
                                                      ArrayRef<PassBuilder::PipelineElement>) {
                                                       if (Name == "reordermem") {
                                                           FPM.addPass(ReorderMemPass());
                                                           return true;
                                                       }
                                                       return false;
                                                   });
            }};
    }
} // namespace sc::opt::reordermem
