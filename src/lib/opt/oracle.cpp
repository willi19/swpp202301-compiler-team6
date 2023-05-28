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

namespace {
  int gettypenum(Type * type)
  {
    ///i1 i8 i16 i32 i64 array type pointer type
    if (type->isIntegerTy() && type->getIntegerBitWidth()==1)
      return 0;
    if (type->isIntegerTy() && type->getIntegerBitWidth()==8)
      return 1;
    if (type->isIntegerTy() && type->getIntegerBitWidth()==32)
      return 2;
    if (type->isIntegerTy() && type->getIntegerBitWidth()==64)
      return 3;
    if (type->isPointerTy())
      return 4;
    if (type->isArrayTy())
      return 5;
    return -1;
  }
};

namespace sc::opt::oracle {
PreservedAnalyses OraclePass::run(Module &M, ModuleAnalysisManager &MAM) {
  FunctionAnalysisManager &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  vector<Loop*> oracle_loop;
  for (Function &F : M) {
    if (F.isDeclaration() || F.empty())
      continue;
    LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
    for (Loop *L : LI) {
      unsigned LoopSize = 0;
      bool hasCall = false;
      bool hasLoad = false;
      bool hasStore = false;
      bool hasBadPhinode = false;
      bool haveconditionalBranch = false;
      bool usedOutside = false;


      for (BasicBlock *BB : L->blocks()) {
        for (Instruction &I : *BB) {
          if (CallInst *callInst = dyn_cast<CallInst>(&I))
            hasCall = true;
          if (LoadInst *loadInst = dyn_cast<LoadInst>(&I))
            hasLoad = true;
          if (StoreInst *storeInst = dyn_cast<StoreInst>(&I))
            hasStore = true;
          //need to be improved
          if (PHINode* phiNode = dyn_cast<PHINode>(&I)) {
            int outerBlock = 0;
            for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i) {
              BasicBlock* incomingBlock = phiNode->getIncomingBlock(i);
              if (!(L->contains(incomingBlock))) {
                outerBlock++;
              }
            }
            if(outerBlock>1)
              hasBadPhinode = true;
          } 
          LoopSize++;
          for (llvm::User* user : I.users()) {
            llvm::Instruction* userInst = llvm::dyn_cast<llvm::Instruction>(user);
            // If the user is an instruction and not contained within the loop
            if (userInst && !L->contains(userInst)) {
              usedOutside=true;
            }
          }
        }
      }

      for (BasicBlock* block : L->blocks()) {
        for (BasicBlock* pred : predecessors(block)) {
          if((L->contains(pred))) {
            continue;
          }
          if (BranchInst* branchInst = dyn_cast<BranchInst>(pred->getTerminator())) {
            if (branchInst->isConditional()) {
              haveconditionalBranch = true;
            }
          }
        }
      }

      if(!hasCall && !hasBadPhinode && !haveconditionalBranch && !usedOutside && LoopSize < 50 && (hasLoad || hasStore))
      {
        oracle_loop.push_back(L);
      }
    }
  }

  if(oracle_loop.size()==0)
    return PreservedAnalyses::all();


  Loop* L = oracle_loop[0];
  BasicBlock *exitBlock = L->getExitBlock();
  vector<Value *> replacedoperand; 
  bool hasouterPhiNode = false;
  
  for(BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      for (Value *operand : I.operands()) {
        if(Instruction *operandInst = dyn_cast<Instruction>(operand)){
          if (operandInst && !L->contains(operandInst)) {
            replacedoperand.push_back(operand);
          }
        }
        else if(Argument *arg = dyn_cast<Argument>(operand))
        {
          replacedoperand.push_back(operand);
        }
      }
      if (PHINode* phiNode = dyn_cast<PHINode>(&I)) {
        for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i) {
          BasicBlock* incomingBlock = phiNode->getIncomingBlock(i);
          if (!(L->contains(incomingBlock))) {
            hasouterPhiNode=true;
          }
        }
      }
    }
  }
  
  //generate oracle function
  LLVMContext &context = M.getContext();
  Type *returnType = Type::getVoidTy(context);
  std::vector<Type *> paramTypes; // Fill in the parameter types
  for(Value * I:replacedoperand) {
    paramTypes.push_back(I->getType());
  }
  FunctionType *functionType = FunctionType::get(returnType, paramTypes, false);
  Function *oracleFunction = Function::Create(functionType, Function::ExternalLinkage, "oracle", &M);
  BasicBlock* entryBlock = BasicBlock::Create(context, "entry", oracleFunction);
  IRBuilder<> entrybuilder(entryBlock);

  BasicBlock* retBlock = BasicBlock::Create(context, "ret", oracleFunction);
  IRBuilder<> retbuilder(retBlock);
  retbuilder.CreateRetVoid();
  //Start copying
  ValueToValueMapTy valueMap;   
  std::map<BasicBlock*, BasicBlock*> blockMap;
  blockMap[exitBlock] = retBlock; 

  for (BasicBlock *BB : L->blocks())
  {
    BasicBlock *NewBB = BasicBlock::Create(context, BB->getName()+".oracle", oracleFunction);
    blockMap[BB] = NewBB;
    IRBuilder<> Builder(NewBB);
    if(BB==L->getHeader())
    {
      entrybuilder.CreateBr(NewBB);
    }

    for (Instruction &I : *BB) 
    {
      Instruction *NewInst = I.clone();
      if (I.hasName())
        NewInst->setName(I.getName()+".oracle");

      //NewInst->insertInto(NewBB, NewBB->end());
      Builder.Insert(NewInst);
      valueMap[&I] = NewInst; // Add instruction map to value.
    }
  }  
  //update operands for new function
  //replacedoperand => to arg
  //if inside valueMap then value map
  
  for (BasicBlock& basicBlock : *oracleFunction) {
    for (Instruction &I : basicBlock) {
      for (Use &U : I.operands()) {
        if (Value *operand = dyn_cast<Value>(U)) {
          if (valueMap.count(operand)) {
            U.set(valueMap[operand]);
          }
          else
          {
            for(int i=0;i<replacedoperand.size();i++)//Value* replaced:replacedoperand)
            {
              if(operand == replacedoperand[i])
              {
                Argument *arg = oracleFunction->getArg(i);
                U.set(arg);
                break;
              }
            }
          }
        }
      }

      if (PHINode* phiNode = dyn_cast<PHINode>(&I)) {
        for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i) {
          BasicBlock* incomingBlock = phiNode->getIncomingBlock(i);
          if (!(L->contains(incomingBlock))) {
            phiNode->setIncomingBlock(i,entryBlock);
          }
          else
          {
            BasicBlock *newSuccessor = blockMap[incomingBlock];
            phiNode->setIncomingBlock(i, newSuccessor);
          }
        }
      }
      
      if (BranchInst *branchInst = dyn_cast<BranchInst>(&I)) {
        for (unsigned i = 0; i < branchInst->getNumSuccessors(); ++i) {
          BasicBlock *originalSuccessor = branchInst->getSuccessor(i);
          // Update the successor basic block
          BasicBlock *newSuccessor = blockMap[originalSuccessor];
          if(newSuccessor) {
            branchInst->setSuccessor(i, newSuccessor);
          }
        }
      }
    }
  }
  
  Function *curFunc = L->getHeader()->getParent();
  //change for loop to oracle
  vector<Instruction *> loopentrybranch;
  for (BasicBlock& basicBlock : *curFunc) {
    if(L->contains(&basicBlock))
    {
      continue;
    }
    if (BranchInst* branchInst = dyn_cast<BranchInst>(basicBlock.getTerminator())) {
      if (!(branchInst->isConditional())) {
        ArrayRef<Value*> args(replacedoperand.data(),replacedoperand.size());
        

        BasicBlock *successor = branchInst->getSuccessor(0);
        BasicBlock *headerBlock = L->getHeader();
        if (successor == headerBlock) {
          
          IRBuilder<> Builder(&basicBlock);
          Builder.SetInsertPoint(branchInst);
          
          CallInst *call = Builder.CreateCall(oracleFunction, args);
          loopentrybranch.push_back(branchInst);
        }
      }
    }
  }

  
  for(auto bI:loopentrybranch)
  {
    bI->setSuccessor(0,exitBlock);
  }

  
  //Remove original Loop
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(*curFunc);
  for(BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      outs()<<I<<"\n";
      if (PHINode* phiNode = dyn_cast<PHINode>(&I)) {
          Type *phiType = phiNode->getType();
          Constant *zeroValue = Constant::getNullValue(phiType);
          phiNode->replaceAllUsesWith(zeroValue);  
      }
    }
  }

  while (true)
  {
    vector<Instruction *> removeInst;
    for(BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        unsigned numUsers = I.getNumUses();
        //outs()<<I<<" "<<numUsers<<"numuses\n";
        if(numUsers==0) {
          removeInst.push_back(&I);
        }
      }
    }
    if(removeInst.size()==0)
      break;
    for(Instruction * I : removeInst)
      I->eraseFromParent();
  }


  std::vector<BasicBlock*> blocks_to_remove;
  for (auto BBI = L->block_begin(), BBE = L->block_end(); BBI != BBE; ++BBI) {
      BasicBlock* BB = *BBI;
      blocks_to_remove.push_back(BB);
  }
  
  // After collecting all blocks in the loop, remove them
  for (BasicBlock* BB : blocks_to_remove) {
      LI.removeBlock(BB);  // Remove block from LoopInfo
      BB->eraseFromParent();  // Remove block from its parent function
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
