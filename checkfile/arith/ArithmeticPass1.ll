define i32 @main(i32 %arg1, i64 %arg2, i8 %arg3, i32 %arg4) {

; CHECK-LABEL: @main(i32 %arg1, i64 %arg2, i8 %arg3, i32 %arg4)
; CHECK-LABEL: check_add:
; CHECK-NEXT:    [[ADDARG1:%.*]] = mul i32 [[A1:%.*]], 2
; CHECK-NEXT:    [[ADDARG2:%.*]] = add i32 [[A1]], [[A4:%.*]]
; CHECK-NEXT:    [[ADDARG3:%.*]] = mul i64 [[A2:%.*]], 2
; CHECK-NEXT:    [[ADDINST1:%.*]] = mul i64 [[ADDARG3]], 2
; CHECK-NEXT:    [[ADDINST2:%.*]] = add i32 [[ADDARG1]], [[A4]]
; CHECK-NEXT:    [[ADDINST3:%.*]] = add i32 [[ADDARG1]], [[ADDARG2]]
; CHECK-NEXT:    [[ADDINST4:%.*]] = add i32 [[ADDARG1]], 100
; CHECK-LABEL: check_shl:
; CHECK-NEXT:    [[SHLARG1:%.*]] = mul i32 [[A1]], 4
; CHECK-NEXT:    [[SHLARG2:%.*]] = mul i8 [[A3:%.*]], 8
; CHECK-NEXT:    [[SHLARG3:%.*]] = mul i64 [[A2]], 16
; CHECK-NEXT:    [[SHLINST1:%.*]] = mul i32 [[SHLARG1]], 8
; CHECK-NEXT:    [[SHLINST2:%.*]] = shl i32 [[SHLARG1]], [[A1]]
; CHECK-LABEL: check_ashr:
; CHECK-NEXT:    [[ASHRARG1:%.*]] = sdiv i32 [[A1]], 4
; CHECK-NEXT:    [[ASHRARG2:%.*]] = sdiv i8 [[A3]], 8
; CHECK-NEXT:    [[ASHRARG3:%.*]] = sdiv i64 [[A2]], 16
; CHECK-NEXT:    [[ASHRINST1:%.*]] = sdiv i32 [[ASHRARG1]], 8
; CHECK-NEXT:    [[ASHRINST2:%.*]] = ashr i32 [[ASHRARG1]], [[A1]]
; CHECK-LABEL: check_lshr:
; CHECK-NEXT:    [[LSHRARG1:%.*]] = udiv i32 [[A1]], 4
; CHECK-NEXT:    [[LSHRARG2:%.*]] = udiv i8 [[A3]], 8
; CHECK-NEXT:    [[LSHRARG3:%.*]] = udiv i64 [[A2]], 16
; CHECK-NEXT:    [[LSHRINST1:%.*]] = udiv i32 [[LSHRARG1]], 8
; CHECK-NEXT:    [[LSHRINST2:%.*]] = lshr i32 [[LSHRARG1]], [[A1]]
; CHECK-NEXT:    ret i32 0
;

check_add:
  %addarg1 = add i32 %arg1, %arg1
  %addarg2 = add i32 %arg1, %arg4
  %addarg3 = add i64 %arg2, %arg2
  %addinst1 = add i64 %addarg3, %addarg3
  %addinst2 = add i32 %addarg1, %arg4
  %addinst3 = add i32 %addarg1, %addarg2
  %addinst4 = add i32 %addarg1, 100
  br label %check_shl

check_shl:
  %shlarg1 = shl i32 %arg1, 2
  %shlarg2 = shl i8 %arg3, 3
  %shlarg3 = shl i64 %arg2, 4
  %shlinst1 = shl i32 %shlarg1, 3
  %shlinst2 = shl i32 %shlarg1, %arg1
  br label %check_ashr

check_ashr:
  %ashrarg1 = ashr i32 %arg1, 2
  %ashrarg2 = ashr i8 %arg3, 3
  %ashrarg3 = ashr i64 %arg2, 4
  %ashrinst1 = ashr i32 %ashrarg1, 3
  %ashrinst2 = ashr i32 %ashrarg1, %arg1
  br label %check_lshr

check_lshr:
  %lshrarg1 = lshr i32 %arg1, 2
  %lshrarg2 = lshr i8 %arg3, 3
  %lshrarg3 = lshr i64 %arg2, 4
  %lshrinst1 = lshr i32 %lshrarg1, 3
  %lshrinst2 = lshr i32 %lshrarg1, %arg1
  ret i32 0
}
