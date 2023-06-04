define i32 @oracle(i32 %a, i32 %b) {
; CHECK-LABEL: @main(i32 %a, i32 %b)
; CHECK-NEXT: [[C:%.*]] = add i32 2, [[A:%.*]]
; CHECK-NEXT: [[D:%.*]] = sub i32 2, [[B:%.*]]
; CHECK-NEXT: [[E:%.*]] = add i32 [[C]], [[D]]
; CHECK-NEXT: ret i32 [[E]]
;
  %c = add i32 2, %a
  %d = sub i32 2, %b
  %e = add i32 %c, %d
  ret i32 %e
}
