define i32 @main(i32 %arg1, i64 %arg2, i8 %arg3, i32 %arg4) {
check_add:
  %addarg1 = mul i32 %arg1, 2
  %addarg2 = add i32 %arg1, %arg4
  %addarg3 = mul i64 %arg2, 2
  %addinst1 = mul i64 %addarg3, 2
  %addinst2 = add i32 %addarg1, %arg4
  %addinst3 = add i32 %addarg1, %addarg2
  %addinst4 = add i32 %addarg1, 100
  br label %check_shl

check_shl:                                        ; preds = %check_add
  %shlarg1 = mul i32 %arg1, 4
  %shlarg2 = mul i8 %arg3, 8
  %shlarg3 = mul i64 %arg2, 16
  %shlinst1 = mul i32 %shlarg1, 8
  %shlinst2 = shl i32 %shlarg1, %arg1
  br label %check_ashr

check_ashr:                                       ; preds = %check_shl
  %ashrarg1 = sdiv i32 %arg1, 4
  %ashrarg2 = sdiv i8 %arg3, 8
  %ashrarg3 = sdiv i64 %arg2, 16
  %ashrinst1 = sdiv i32 %ashrarg1, 8
  %ashrinst2 = ashr i32 %ashrarg1, %arg1
  br label %check_lshr

check_lshr:                                       ; preds = %check_ashr
  %lshrarg1 = udiv i32 %arg1, 4
  %lshrarg2 = udiv i8 %arg3, 8
  %lshrarg3 = udiv i64 %arg2, 16
  %lshrinst1 = udiv i32 %lshrarg1, 8
  %lshrinst2 = lshr i32 %lshrarg1, %arg1
  ret i32 0
}
