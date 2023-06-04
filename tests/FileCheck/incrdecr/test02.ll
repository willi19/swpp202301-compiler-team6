define i64 @main(i64 %a, i64 %b) {
; CHECK-LABEL: @main(i64 %a, i64 %b)
; CHECK-NEXT: [[C1:%.*]] = call i64 @incr_i64(i64 [[A:%.*]])
; CHECK-NEXT: [[C2:%.*]] = call i64 @incr_i64(i64 [[C1]])
; CHECK-NEXT: [[C3:%.*]] = call i64 @incr_i64(i64 [[C2]])
; CHECK-NEXT: [[D4:%.*]] = call i64 @decr_i64(i64 [[B:%.*]])
; CHECK-NEXT: [[D5:%.*]] = call i64 @decr_i64(i64 [[D4]])
; CHECK-NEXT: [[D6:%.*]] = call i64 @decr_i64(i64 [[D5]])
; CHECK-NEXT: [[D7:%.*]] = call i64 @decr_i64(i64 [[D6]])
; CHECK-NEXT: [[E:%.*]] = add i64 [[C3]], [[D7]]
; CHECK-NEXT: ret i64 [[E]]
;
  %c = sub i64 %a, -3
  %d = add i64 %b, -4
  %e = add i64 %c, %d
  ret i64 %e
}
