define i32 @main(i32 %arg1, i32 %arg2, i8 %arg3, i1 %arg4) {
; CHECK-LABEL: @main(i32 %arg1, i32 %arg2, i8 %arg3, i1 %arg4)
; CHECK-NEXT:    [[CHECK1:%.*]] = add i32 0, 5
; CHECK-NEXT:    [[AND2:%.*]] = and i32 [[ARG1:%.*]], [[ARG2:%.*]]
; CHECK-NEXT:    [[AND3:%.*]] = urem i8 [[ARG3:%.*]], 2
; CHECK-NEXT:    [[AND4:%.*]] = and i8 [[ARG3:%.*]], 2
; CHECK-NEXT:    [[AND5:%.*]] = urem i8 [[ARG3:%.*]], 4
; CHECK-NEXT:    [[AND6:%.*]] = and i8 [[ARG3]], 4
; CHECK-NEXT:    [[AND7:%.*]] = and i8 [[ARG3]], 5
; CHECK-NEXT:    [[AND9:%.*]] = and i8 [[ARG3]], [[ARG3]]
; CHECK-NEXT:    [[CHECK2:%.*]] = mul i1 [[ARG4:%.*]], [[ARG4]]
; CHECK-NEXT:    ret i32 0
;
  %and1 = and i32 %arg1, 0
  %check1 = add i32 %and1, 5
  %and2 = and i32 %arg1, %arg2
  %and3 = and i8 %arg3, 1
  %and4 = and i8 %arg3, 2
  %and5 = and i8 %arg3, 3
  %and6 = and i8 %arg3, 4
  %and7 = and i8 %arg3, 5
  %and8 = and i8 %arg3, 255
  %and9 = and i8 %and8, %and8
  %and10 = and i1 %arg4, 0
  %and11 = and i1 %arg4, %and10
  %and12 = and i1 %arg4, 1
  %check2 = mul i1 %and12, %and12
  ret i32 0
}
