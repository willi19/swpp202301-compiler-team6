define i32 @main(i32 %a, i32 %b) {
; CHECK-LABEL: @main(i32 %a, i32 %b)
; CHECK-NEXT: [[C1:%.*]] = call i32 @incr_i32(i32 [[A:%.*]])
; CHECK-NEXT: [[C2:%.*]] = call i32 @incr_i32(i32 [[C1]])
; CHECK-NEXT: [[D:%.*]] = sub i32 2, [[B:%.*]]
; CHECK-NEXT: [[E:%.*]] = add i32 [[C2]], [[D]]
; CHECK-NEXT: ret i32 [[E]]
;
  %c = add i32 2, %a
  %d = sub i32 2, %b
  %e = add i32 %c, %d
  ret i32 %e
}
