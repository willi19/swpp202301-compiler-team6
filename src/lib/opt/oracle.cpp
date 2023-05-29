#include "oracle.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
using namespace std;

namespace {
int numOuterEntryBlock(Loop *L, PHINode *phiNode) {
  int cnt = 0;
  for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i) {
    if (!(L->contains(phiNode->getIncomingBlock(i)))) {
      cnt++;
    }
  }
  return cnt;
}

bool isLoopValid(Loop *L) {
  int LoopSize = 0;
  // Loop that can be optimized should have following properties
  // No Call -> project restriction
  // Have Load or Store -> For optimization to be meaningful
  bool WorthOpt = false;

  // Due to complexity and code length I removed following loop from the
  // candidate
  //  HasCondBranch ->
  //  I'm changing
  //  br %for.entry
  //      |
  //      v
  //  call oracle
  //  br %for.exit
  // But with conditional branch, we need to add basic block to function. This
  // will be fixed during review

  // HasBadPhinode
  // If Phinode have multiple incoming basic block outside the loop
  // It is hard to call oracle with corresponding value so I ignored it

  // UsedOutside
  // If register being updated in loop is used outside
  // than we must return or store value. This was complicated so I ignored it
  // However this must be fixed oracle optimization on more cases.
  // ex)
  // for(int i=0;i<n) sum += a[i];
  // print(sum)

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      LoopSize++;

      if (CallInst *callInst = dyn_cast<CallInst>(&I))
        return false;

      if (dyn_cast<LoadInst>(&I))
        WorthOpt = true;
      if (dyn_cast<StoreInst>(&I))
        WorthOpt = true;

      if (PHINode *phiNode = dyn_cast<PHINode>(&I))
        if (numOuterEntryBlock(L, phiNode) > 1)
          return false;

      for (User *user : I.users()) {
        auto *userInst = dyn_cast<Instruction>(user);
        if (userInst && !L->contains(userInst)) {
          return false;
        }
      }
    }
    // Checking there is conditional branch that comes to loop
    for (BasicBlock *pred : predecessors(BB)) {
      if (auto *branchInst = dyn_cast<BranchInst>(pred->getTerminator())) {
        if (!(L->contains(pred)) && branchInst->isConditional()) {
          return false;
        }
      }
    }
  }

  // optimizable, 45 for safety
  return (LoopSize < 45 && WorthOpt);
}
} // namespace

namespace sc::opt::oracle {
PreservedAnalyses OraclePass::run(Module &M, ModuleAnalysisManager &MAM) {
  FunctionAnalysisManager &FAM =
      MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  LLVMContext &context = M.getContext();

  vector<Loop *> OracleLoop; // loop that can be optimized

  for (Function &F : M) {
    // ignore if F is not defined
    if (F.isDeclaration() || F.empty())
      continue;

    LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

    for (Loop *L : LI) {
      if (isLoopValid(L))
        OracleLoop.push_back(L);
    }
  }

  if (OracleLoop.size() == 0)
    return PreservedAnalyses::all();

  Loop *L = OracleLoop[0];
  BasicBlock *exitBlock = L->getExitBlock();
  vector<Value *> OuterOperand; // Operand From Outside needs to be changed to
                                // argument of oracle
  vector<Type *> paramTypes;    // Fill in the parameter types

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      for (Value *operand : I.operands()) {
        bool Outer = false;
        if (Instruction *operandInst = dyn_cast<Instruction>(operand)) {
          Outer = (!L->contains(operandInst));
        } else if (Argument *arg = dyn_cast<Argument>(operand)) {
          Outer = true;
        }
        if (Outer && find(OuterOperand.begin(), OuterOperand.end(), operand) ==
                         OuterOperand.end()) {
          OuterOperand.push_back(operand);
          paramTypes.push_back(operand->getType());
        }
      }
    }
  }

  // generate oracle function
  Type *returnType = Type::getVoidTy(context);
  FunctionType *functionType = FunctionType::get(returnType, paramTypes, false);
  Function *oracleFunction =
      Function::Create(functionType, Function::ExternalLinkage, "oracle", &M);

  // For phi-nodes, I add entry block which just branch to loop entry block
  BasicBlock *entryBlock = BasicBlock::Create(context, "entry", oracleFunction);
  IRBuilder<> entrybuilder(entryBlock);

  // Exit block of the loop does not included in loop so branch to such block
  // should be changed to return But if such branch is conditional we need
  // another basic block This happens most of the case so just make it
  BasicBlock *retBlock = BasicBlock::Create(context, "ret", oracleFunction);
  IRBuilder<> retbuilder(retBlock);
  retbuilder.CreateRetVoid();

  // Start copying
  // Use value, blockmap to change corresponding operands to copied one
  ValueToValueMapTy valueMap;
  map<BasicBlock *, BasicBlock *> blockMap;
  blockMap[exitBlock] = retBlock;

  for (BasicBlock *BB : L->blocks()) {
    BasicBlock *NewBB =
        BasicBlock::Create(context, BB->getName() + ".oracle", oracleFunction);
    blockMap[BB] = NewBB;
    IRBuilder<> Builder(NewBB);
    if (BB == L->getHeader()) {
      entrybuilder.CreateBr(NewBB);
    }

    for (Instruction &I : *BB) {
      Instruction *NewInst = I.clone();
      if (I.hasName()) {
        NewInst->setName(I.getName() + ".oracle");
      }
      Builder.Insert(NewInst);
      valueMap[&I] = NewInst;
    }
  }

  // Change the operand to corresponding copied ones
  // Operand to corresponding ones
  // Basic Block of phi node to copied one
  // Basic Block of branch destination to copied one
  for (BasicBlock &BB : *oracleFunction) {
    for (Instruction &I : BB) {
      for (Use &U : I.operands()) {
        if (Value *operand = dyn_cast<Value>(U)) {
          if (valueMap.count(operand)) {
            U.set(valueMap[operand]);
          } else {
            for (int i = 0; i < OuterOperand.size(); i++) {
              if (operand == OuterOperand[i]) {
                U.set(oracleFunction->getArg(i));
                break;
              }
            }
          }
        }
      }

      if (PHINode *phiNode = dyn_cast<PHINode>(&I)) {
        for (unsigned i = 0; i < phiNode->getNumIncomingValues(); ++i) {
          BasicBlock *incomingBlock = phiNode->getIncomingBlock(i);
          if (!(L->contains(incomingBlock))) {
            phiNode->setIncomingBlock(i, entryBlock);
          } else {
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
          if (newSuccessor) {
            branchInst->setSuccessor(i, newSuccessor);
          }
        }
      }
    }
  }

  // entry to for-loop to call oracle
  Function *CurFunc = L->getHeader()->getParent();
  vector<Instruction *> LoopEntryBranch;

  for (BasicBlock &BB : *CurFunc) {
    if (L->contains(&BB)) {
      continue;
    }
    if (BranchInst *BrInst = dyn_cast<BranchInst>(BB.getTerminator())) {
      if (!(BrInst->isConditional())) { 
        // I know this is redundant but for further implementation
        ArrayRef<Value *> args(OuterOperand.data(), OuterOperand.size());
        BasicBlock *successor = BrInst->getSuccessor(0);
        BasicBlock *headerBlock = L->getHeader();
        if (successor == headerBlock) {
          IRBuilder<> Builder(&BB);
          Builder.SetInsertPoint(BrInst);
          CallInst *call = Builder.CreateCall(oracleFunction, args);
          LoopEntryBranch.push_back(BrInst);
        }
      }
    }
  }

  for (auto bI : LoopEntryBranch) {
    bI->setSuccessor(0, exitBlock);
  }

  // Remove original Loop
  // First remove phi nodes to break assignment cycle
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(*CurFunc);
  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      if (PHINode *phiNode = dyn_cast<PHINode>(&I)) {
        Type *phiType = phiNode->getType();
        Constant *zeroValue = Constant::getNullValue(phiType);
        phiNode->replaceAllUsesWith(zeroValue);
      }
    }
  }

  while (true) {
    vector<Instruction *> removeInst;
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        unsigned numUsers = I.getNumUses();
        if (numUsers == 0) {
          removeInst.push_back(&I);
        }
      }
    }
    if (removeInst.size() == 0)
      break;
    for (Instruction *I : removeInst)
      I->eraseFromParent();
  }

  std::vector<BasicBlock *> BlockToRemove;
  for (BasicBlock *BB : L->blocks()) {
    BlockToRemove.push_back(BB);
  }

  // After collecting all blocks in the loop, remove them
  for (BasicBlock *BB : BlockToRemove) {
    LI.removeBlock(BB);    // Remove block from LoopInfo
    BB->eraseFromParent(); // Remove block from its parent function
  }

  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPOracle", "v0.1", [](PassBuilder &PB) {
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
}; // namespace sc::opt::oracle
