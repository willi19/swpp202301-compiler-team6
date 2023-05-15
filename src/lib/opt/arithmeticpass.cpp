#include "arithmeticpass.h"

#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <vector>

using namespace llvm;
using namespace std;
using namespace llvm::PatternMatch;

namespace sc::opt::arithmeticpass {
PreservedAnalyses ArithmeticPass::run(Function &F,
                                      FunctionAnalysisManager &FAM) {
  bool Changed = false;
  for (BasicBlock &BB : F) {
    vector<Instruction *> AddInst, ShlInst, LShrInst, AShrInst, AndInst;
    for (auto itr = BB.begin(), en = BB.end(); itr != en;) {
      // exist for ReplaceInstWithValue() in AndInst operation
      auto TmpItr = itr++;
      Instruction &I = *TmpItr;
      ConstantInt *ConstOp; // Constant operand in instruction
      Value *FirstOp;       // FirstOperand Value
      // add X X -> mul X 2
      if (PatternMatch::match(&I,
                              m_Add(m_Value(FirstOp), m_Deferred(FirstOp)))) {
        AddInst.push_back(&I);
      }

      // Case two: Shl X C -> mul X (2^C)
      if (PatternMatch::match(
              &I, m_Shl(m_Value(FirstOp), m_ConstantInt(ConstOp)))) {
        ShlInst.push_back(&I);
      }

      // Case three: LShrInst X C -> Udiv X (2^C)
      if (PatternMatch::match(
              &I, m_LShr(m_Value(FirstOp), m_ConstantInt(ConstOp)))) {
        LShrInst.push_back(&I);
      }

      // Case four: AShrInst X C -> Sdiv X (2^C)
      if (PatternMatch::match(
              &I, m_AShr(m_Value(FirstOp), m_ConstantInt(ConstOp)))) {
        AShrInst.push_back(&I);
      }

      // Case five: AndInst X (2^C-1) -> Urem X (2^C)
      if (PatternMatch::match(
              &I, m_And(m_Value(FirstOp), m_ConstantInt(ConstOp))) ||
          PatternMatch::match(
              &I, m_And(m_ConstantInt(ConstOp), m_Value(FirstOp)))) {
        uint64_t cons = ConstOp->getZExtValue(),
                 width = FirstOp->getType()->getIntegerBitWidth();
        if (cons == UINT64_MAX || (cons & (cons + 1)) == 0) {
          if (cons == UINT64_MAX) {
            ReplaceInstWithValue(BB.getInstList(), TmpItr, FirstOp);
          } else if (width != 64 && (cons + 1) == (1ull << width)) {
            ReplaceInstWithValue(BB.getInstList(), TmpItr, FirstOp);
          } else if (cons == 0) {
            ReplaceInstWithValue(BB.getInstList(), TmpItr, ConstOp);
          } else {
            AndInst.push_back(&I);
          }
        }
      }
    }

    // Change Instruction to mul x 2
    for (Instruction *AddI : AddInst) {
      Changed = true;
      Value *FirstOp = AddI->getOperand(0);
      Instruction *NewInst = BinaryOperator::Create(
          Instruction::Mul, FirstOp, ConstantInt::get(FirstOp->getType(), 2));
      ReplaceInstWithInst(AddI, NewInst);
    }

    // Change Instruction to mul x (1<<c), if (1<<c) generates overflow, than
    // original instruction was undefined too.
    for (Instruction *ShlI : ShlInst) {
      Changed = true;
      Value *FirstOp = ShlI->getOperand(0);
      ConstantInt *ShlVal = dyn_cast<ConstantInt>(ShlI->getOperand(1));
      uint64_t c = ShlVal->getZExtValue();
      Instruction *NewInst = BinaryOperator::Create(
          Instruction::Mul, FirstOp,
          ConstantInt::get(FirstOp->getType(), (1ull << c)));
      ReplaceInstWithInst(ShlI, NewInst);
    }

    // Change Instruction to udiv x (1<<c)
    for (Instruction *LShrI : LShrInst) {
      Changed = true;
      Value *FirstOp = LShrI->getOperand(0);
      ConstantInt *LShrVal = dyn_cast<ConstantInt>(LShrI->getOperand(1));
      uint64_t ushrval = LShrVal->getZExtValue();
      Instruction *NewInst = BinaryOperator::Create(
          Instruction::UDiv, FirstOp,
          ConstantInt::get(FirstOp->getType(), (1ull << ushrval)));
      ReplaceInstWithInst(LShrI, NewInst);
    }

    // Change Instruction to sdiv x (1<<c)
    for (Instruction *AShrI : AShrInst) {
      Changed = true;
      Value *FirstOp = AShrI->getOperand(0);
      ConstantInt *AShrVal = dyn_cast<ConstantInt>(AShrI->getOperand(1));
      uint64_t ashrval = AShrVal->getZExtValue();
      Instruction *NewInst = BinaryOperator::Create(
          Instruction::SDiv, FirstOp,
          ConstantInt::get(FirstOp->getType(), (1ull << ashrval)));
      ReplaceInstWithInst(AShrI, NewInst);
    }

    // Change Instruction to urem x (1<<c) or const
    for (Instruction *AndI : AndInst) {
      Changed = true;
      auto *FirstOp = AndI->getOperand(0);
      auto *SecondOperand = AndI->getOperand(1);

      Value *XOperand;
      ConstantInt *AndVal = dyn_cast<ConstantInt>(FirstOp);

      if (AndVal == NULL) {
        AndVal = dyn_cast<ConstantInt>(SecondOperand);
        XOperand = FirstOp;
      } else {
        XOperand = SecondOperand;
      }

      uint64_t andval = AndVal->getZExtValue();
      Instruction *NewInst = BinaryOperator::Create(
          Instruction::URem, XOperand,
          ConstantInt::get(XOperand->getType(), (andval + 1)));
      ReplaceInstWithInst(AndI, NewInst);
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPArithmetic", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "arithmetic") {
                    FPM.addPass(ArithmeticPass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::arithmeticpass
