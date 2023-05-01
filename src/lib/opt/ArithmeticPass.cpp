#include "ArithmeticPass.h"

PreservedAnalyses ArithmeticPass::run(Function &F, FunctionAnalysisManager &FAM) {
  for(BasicBlock &BB: F){
    vector<Instruction *> ADD, Shl, LShr, AShr, And;
    for(auto itr = BB.begin(), en = BB.end(); itr!=en;){
      auto tmp_itr = itr++; //exist for replaceinstwithvalue in and operation
      Instruction &I = *tmp_itr;
      
      ConstantInt* ConstOp; //Constant Value
      Value* FirstOp; //FirstOperand Value
      //add X X -> mul X 2
      if(match(&I, m_Add(m_Value(FirstOp), m_Deferred(FirstOp))))
        ADD.push_back(&I);

      //Case two: shl X C -> mul X (2^C)
      if(match(&I, m_Shl(m_Value(FirstOp), m_ConstantInt(ConstOp))))
        Shl.push_back(&I);

      //Case three: lshr X C -> Udiv X (2^C)
      if(match(&I, m_LShr(m_Value(FirstOp), m_ConstantInt(ConstOp))))
        LShr.push_back(&I);

      //Case four: ashr X C -> Sdiv X (2^C)
      if(match(&I, m_AShr(m_Value(FirstOp), m_ConstantInt(ConstOp))))
        AShr.push_back(&I);

      //Case five: and X (2^C-1) -> Urem X (2^C)
      if(match(&I, m_And(m_Value(FirstOp), m_ConstantInt(ConstOp))) || match(&I, m_And(m_ConstantInt(ConstOp), m_Value(FirstOp)))){
        uint64_t cons = ConstOp->getZExtValue(), width = FirstOp->getType()->getIntegerBitWidth();;
        if(cons==UINT64_MAX||(cons & (cons+1))==0)
        {
          if(cons==UINT64_MAX)
            ReplaceInstWithValue(BB.getInstList(),tmp_itr,FirstOp);
          else if(width!=64&&(cons+1)==(1ull<<width))
            ReplaceInstWithValue(BB.getInstList(),tmp_itr,FirstOp);
          else if(cons==0)
            ReplaceInstWithValue(BB.getInstList(),tmp_itr,ConstOp);
          else
            And.push_back(&I);
        }
      }
    }
    //Change Instruction to mul x 2
    for(Instruction *AddI : ADD){
      Value* FirstOperand = AddI->getOperand(0);
      Instruction*  NewInst = BinaryOperator::Create(Instruction::Mul, FirstOperand, ConstantInt::get(FirstOperand->getType(),2));
      ReplaceInstWithInst(AddI, NewInst);
    }

    //Change Instruction to mul x (1<<c), if (1<<c) generates overflow, than original instruction was undefined too.
    for(Instruction *ShlI : Shl){
      Value* FirstOperand = ShlI->getOperand(0);
      ConstantInt* ShlVal = dyn_cast<ConstantInt> (ShlI->getOperand(1));
      uint64_t c = ShlVal->getZExtValue();
      Instruction*  NewInst = BinaryOperator::Create(Instruction::Mul, FirstOperand, ConstantInt::get(FirstOperand->getType(),(1ull<<c)));
      ReplaceInstWithInst(ShlI, NewInst);
    }

    //Change Instruction to udiv x (1<<c)
    for(Instruction *LShrI : LShr){
      Value* FirstOperand = LShrI->getOperand(0);
      ConstantInt* LShrVal = dyn_cast<ConstantInt> (LShrI->getOperand(1));
      uint64_t ushrval = LShrVal->getZExtValue();
      Instruction*  NewInst = BinaryOperator::Create(Instruction::UDiv, FirstOperand,ConstantInt::get(FirstOperand->getType(),(1ull<<ushrval)));
      ReplaceInstWithInst(LShrI, NewInst);
    }

    //Change Instruction to sdiv x (1<<c)
    for(Instruction *AShrI : AShr){
      Value* FirstOperand = AShrI->getOperand(0);
      ConstantInt* AShrVal = dyn_cast<ConstantInt> (AShrI->getOperand(1));
      uint64_t ashrval = AShrVal->getZExtValue();
      Instruction*  NewInst = BinaryOperator::Create(Instruction::SDiv, FirstOperand, ConstantInt::get(FirstOperand->getType(),(1ull<<ashrval)));
      ReplaceInstWithInst(AShrI, NewInst);
    }

    //Change Instruction to urem x (1<<c) or const
    for(Instruction *AndI : And){
      Value* FirstOperand = AndI->getOperand(0);
      Value* SecondOperand = AndI->getOperand(1);

      Value* XOperand;
      ConstantInt* AndVal = dyn_cast<ConstantInt> (FirstOperand);

      if(AndVal==NULL){
        AndVal = dyn_cast<ConstantInt> (SecondOperand);
        XOperand = FirstOperand;
      } else {
        XOperand = SecondOperand;
      }

      uint64_t andval = AndVal->getZExtValue();
      Instruction* NewInst = BinaryOperator::Create(Instruction::URem, XOperand, ConstantInt::get(XOperand->getType(),(andval+1)));
      ReplaceInstWithInst(AndI, NewInst);
    }
  }

  return PreservedAnalyses::all();
}