define i32 @main(i32 %a, i32 %b) {
; CHECK-LABEL: @main(i32 %a, i32 %b)
; CHECK-NEXT: [[C1:%.*]] = call i32 @incr_i32(i32 [[A:%.*]])
; CHECK-NEXT: [[D2:%.*]] = call i32 @decr_i32(i32 [[B:%.*]])
; CHECK-NEXT: [[D3:%.*]] = call i32 @decr_i32(i32 [[D2]])
; CHECK-NEXT: [[E:%.*]] = add i32 [[C1]], [[D3]]
; CHECK-NEXT: ret i32 [[E]]
;
  %c = add i32 %a, 1
  %d = sub i32 %b, 2
  %e = add i32 %c, %d
  ret i32 %e
}
