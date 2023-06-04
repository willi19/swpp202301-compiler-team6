define i64 @main(i64 %a, i64 %b) {
; CHECK-LABEL: @main(i64 %a, i64 %b)
; CHECK-NEXT: [[C:%.*]] = add i64 [[A:%.*]], 5
; CHECK-NEXT: [[D:%.*]] = sub i64 [[B:%.*]], 6
; CHECK-NEXT: [[E:%.*]] = add i64 [[C]], [[D]]
; CHECK-NEXT: ret i64 [[E]]
;
  %c = add i64 %a, 5
  %d = sub i64 %b, 6
  %e = add i64 %c, %d
  ret i64 %e
}
