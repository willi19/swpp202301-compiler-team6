define i32 @main(i32 %arg1, i32 %arg2, i8 %arg3, i1 %arg4) {
; CHECK-LABEL: @main(i32 %arg1, i32 %arg2, i8 %arg3, i1 %arg4)
; CHECK-NEXT:    [[CHECK1:%.*]] = add i32 0 5
; CHECK-NEXT:    [[AND2:%.*]] = and i32 [[ARG1:%.*]], [[ARG2%.*]]
; CHECK-NEXT:    [[AND3:%.*]] = urem i8 [[ARG3:%.*]], 2
; CHECK-NEXT:    [[AND4:%.*]] = and i8 [[ARG3:%.*]], 2
; CHECK-NEXT:    [[AND5:%.*]] = urem i8 [[ARG3:%.*]], 4
; CHECK-NEXT:    [[AND6:%.*]] = and i8 [[ARG3]], 4
; CHECK-NEXT:    [[AND7:%.*]] = and i8 [[ARG3]], 5
; CHECK-NEXT:    [[AND9:%.*]] = and i8 [[ARG3]], [[ARG3]]
; CHECK-NEXT:    [[CHECK2:%.*]] = mul i1 [[ARG4:%.*]], [[ARG4]]
; CHECK:       bb_exit:
; CHECK-NEXT:    [[ADDARG1:%.*]] = mul i32 [[A1:%.*]], 2
; CHECK-NEXT:    [[ADDARG2:%.*]] = add i32 [[A1]], [[A4:%.*]]
; CHECK-NEXT:    [[ADDARG3:%.*]] = mul i64 [[A2:%.*]], 2
; CHECK-NEXT:    [[ADDINST1:%.*]] = mul i64 [[ADDARG3]], 2
; CHECK-NEXT:    [[ADDINST2:%.*]] = add i32 [[ADDARG1]], [[A4]]
; CHECK-NEXT:    [[ADDINST3:%.*]] = add i32 [[ADDARG1]], [[ADDARG2]]
; CHECK-NEXT:    [[ADDINST4:%.*]] = add i32 [[ADDARG1]], 100
; CHECK-NEXT:    [[SHLARG1:%.*]] = mul i32 [[A1]], 4
; CHECK-NEXT:    [[SHLARG2:%.*]] = mul i8 [[A3:%.*]], 8
; CHECK-NEXT:    [[SHLARG3:%.*]] = mul i64 [[A2]], 16
; CHECK-NEXT:    [[SHLINST1:%.*]] = mul i32 [[SHLARG1]], 8
; CHECK-NEXT:    [[SHLINST2:%.*]] = shl i32 [[SHLARG1]], [[A1]]
; CHECK-NEXT:    [[ASHRARG1:%.*]] = sdiv i32 [[A1]], 4
; CHECK-NEXT:    [[ASHRARG2:%.*]] = sdiv i8 [[A3]], 8
; CHECK-NEXT:    [[ASHRARG3:%.*]] = sdiv i64 [[A2]], 16
; CHECK-NEXT:    [[ASHRINST1:%.*]] = sdiv i32 [[ASHRARG1]], 8
; CHECK-NEXT:    [[ASHRINST2:%.*]] = ashr i32 [[ASHRARG1]], [[A1]]
; CHECK-NEXT:    [[LSHRARG1:%.*]] = udiv i32 [[A1]], 4
; CHECK-NEXT:    [[LSHRARG2:%.*]] = udiv i8 [[A3]], 8
; CHECK-NEXT:    [[LSHRARG3:%.*]] = udiv i64 [[A2]], 16
; CHECK-NEXT:    [[LSHRINST1:%.*]] = udiv i32 [[LSHRARG1]], 8
; CHECK-NEXT:    [[LSHRINST2:%.*]] = lshr i32 [[LSHRARG1]], [[A1]]
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
  br label %bb_exit

  bb_exit:
      %addarg1 = add i32 %arg1, %arg1
      %addarg2 = add i32 %arg1, %arg4
      %addarg3 = add i64 %arg2, %arg2
      %addinst1 = add i64 %addarg3, %addarg3
      %addinst2 = add i32 %addarg1, %arg4
      %addinst3 = add i32 %addarg1, %addarg2
      %addinst4 = add i32 %addarg1, 100
      %shlarg1 = shl i32 %arg1, 2
      %shlarg2 = shl i8 %arg3, 3
      %shlarg3 = shl i64 %arg2, 4
      %shlinst1 = shl i32 %shlarg1, 3
      %shlinst2 = shl i32 %shlarg1, %arg1
      %ashrarg1 = ashr i32 %arg1, 2
      %ashrarg2 = ashr i8 %arg3, 3
      %ashrarg3 = ashr i64 %arg2, 4
      %ashrinst1 = ashr i32 %ashrarg1, 3
      %ashrinst2 = ashr i32 %ashrarg1, %arg1
      %lshrarg1 = lshr i32 %arg1, 2
      %lshrarg2 = lshr i8 %arg3, 3
      %lshrarg3 = lshr i64 %arg2, 4
      %lshrinst1 = lshr i32 %lshrarg1, 3
      %lshrinst2 = lshr i32 %lshrarg1, %arg1
      ret i32 0
}
