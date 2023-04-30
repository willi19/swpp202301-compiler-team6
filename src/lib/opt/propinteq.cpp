#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

namespace {
class PropagateIntegerEquality
    : public PassInfoMixin<PropagateIntegerEquality> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
    vector<StringRef> arg;
    vector<StringRef> inst;
    for (Argument &Arg : F.args()){
      arg.push_back(Arg.getName());
    }
    for(BasicBlock &BB:F)
    {
      for (auto &I : BB){ //check for each instruction
        if(std::find(inst.begin(),inst.end(),I.getName())==inst.end())
          inst.push_back(I.getName());
      }
    }
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
      BasicBlock &BB = *I;
      BranchInst *TI = dyn_cast<BranchInst>(BB.getTerminator());
      //outs<<*TI<<"\n";
      if(!TI)
        continue;
      if(!(TI->isConditional()))
        continue;
      CmpInst *cond = dyn_cast<CmpInst>(TI->getCondition());
      if(!cond || cond->getPredicate() != ICmpInst::ICMP_EQ)
        continue;

      auto *x = cond->getOperand(0);
      auto *y = cond->getOperand(1);

      BasicBlock* NextBB = TI->getSuccessor(0);
      BasicBlockEdge BBE(&BB, NextBB);

      if(x->getType()->isPointerTy()||y->getType()->isPointerTy())
        continue;
      
      Value * A; 
      Value * B; //replace A to B

      bool isargX = (std::find(arg.begin(),arg.end(),x->getName())!=arg.end());
      bool isargY = (std::find(arg.begin(),arg.end(),y->getName())!=arg.end());

      int indX = isargX?(std::find(arg.begin(),arg.end(),x->getName())-arg.begin()):(std::find(inst.begin(),inst.end(),x->getName())-inst.end());
      int indY = isargY?(std::find(arg.begin(),arg.end(),y->getName())-arg.begin()):(std::find(inst.begin(),inst.end(),y->getName())-inst.end());
      

      if(!isargX&&isargY)
      {
        A = x;
        B = y;
      }

      else if(isargX&&!isargY)
      {
        A = y;
        B = x;
      }

      else if(isargX&&isargY)
      {
        A = indX>indY?x:y;
        B = indX>indY?y:x;
      }

      else
      {
        Instruction * xI = dyn_cast<Instruction> (x);
        Instruction * yI = dyn_cast<Instruction> (y);
        //outs<<*(xI->getType()).str()<<" "<<*(yI->getType()).str()<<"\n";
        BasicBlock *xBlock = xI->getParent();
        BasicBlock *yBlock = yI->getParent();
        if(xBlock==yBlock)
        {
            A = indX>indY?x:y;
            B = indX>indY?y:x;
        }
        else if(DT.dominates(xBlock,yBlock))
        {
            A = y;
            B = x;
        }
        else
        {
            A = x;
            B = y;
        }
      }
      
      unsigned int cnt = A->getNumUses();
      for (unsigned int i=0; i<cnt; i++){
        for (Use &U : A->uses()){ 
          User *Usr = U.getUser();
          Instruction *UsrI = dyn_cast<Instruction>(Usr);
          BasicBlock *BBParent = UsrI->getParent(); 
          if (DT.dominates(BBE, BBParent)) 
            if(U!=B)
              U.set(B);
        }
      }
    }
    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "PropagateIntegerEquality", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "prop-int-eq") {
                    FPM.addPass(PropagateIntegerEquality());
                    return true;
                  }
                  return false;
                });
          }};
}
