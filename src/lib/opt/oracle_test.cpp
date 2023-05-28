#include "oracle.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <queue>
#include <string>
#include <unordered_set>
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Transforms/Utils/LoopRotationUtils.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ScalarEvolution.h"

using namespace llvm;
using namespace std;


namespace sc::opt::oracle {
PreservedAnalyses OraclePass::run(Module &M, ModuleAnalysisManager &MAM) {
    FunctionAnalysisManager &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
    for (Function &F : M) {
      if (F.isDeclaration() || F.empty())
        continue;

      LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
      ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);

      for (Loop *L : LI) {
        unsigned LoopSize = 0;
        bool validLoop = true;
        bool hasLoad = false;
        const SCEV *TripCount = SE.getBackedgeTakenCount(L); 
        /*
        if (isa<SCEVCouldNotCompute>(TripCount)) {
            outs() << "Loop trip count could not be computed.\n";
        } 
        else if (TripCount->getType()->isIntegerTy()) {
          if (const SCEVConstant *ConstantSCEV = dyn_cast<SCEVConstant>(TripCount)) {
            ConstantInt *ConstantValue = ConstantSCEV->getValue();
            const APInt &APValue = ConstantValue->getValue();
            outs() << "Loop executes a constant number of times.\n";
            outs() << "Constant Trip Count: " << APValue << "\n";
          }else {
            outs() << "Loop executes a variable number of times.\n";
            outs() << "Trip Count is not constant.\n";
          }
        } else {
          outs() << "Loop trip count could not be computed.\n";
        }
        */
        for (BasicBlock *BB : L->blocks()) {
          for (Instruction &I : *BB) {
            if (CallInst *callInst = dyn_cast<CallInst>(&I))
              validLoop = false;
            if (LoadInst *loadInst = dyn_cast<LoadInst>(&I))
              hasLoad = true;
            if (StoreInst *storeInst = dyn_cast<StoreInst>(&I))
              hasLoad = true;
            LoopSize++;
          }
        }
        if(validLoop && LoopSize < 50&&hasLoad)
        {
          for (BasicBlock *BB : L->blocks()) {
            for (Instruction &I : *BB) {
              outs() << I << "\n";
            }
          }
          /*
          for (BasicBlock *BB : L->blocks()) {
            for (Instruction &I : *BB) {
              for (Value *operand : I.operands()) {
                Instruction *operandInst = dyn_cast<Instruction>(operand);
                if (operandInst && !L->contains(operandInst)) {
                  // The value is from outside the loop
                  // You can perform any desired action
                  outs() << "Value: " << *operand << " is from outside the loop.\n";
                }
              }
            }
          }
          outs() << "Loop in function '" << F.getName() << "' has size: " << LoopSize << "\n=============================\n";
          for (BasicBlock *BB : L->blocks()) {
          for (Instruction &I : *BB){
            for (llvm::User* user : I.users()) {
              llvm::Instruction* userInst = llvm::dyn_cast<llvm::Instruction>(user);
              // If the user is an instruction and not contained within the loop
              if (userInst && !L->contains(userInst)) {
                outs() << "Value: " << I << " is used outside the loop. \n";
              }
            }
          }
          }*/
        }
      }
    }
  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPOracle", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "oracle") {
                    MPM.addPass(OraclePass());
                    return true;
                  }
                  return false;
                });
          }};
}
}; // namespace sc::opt::heap2stack
