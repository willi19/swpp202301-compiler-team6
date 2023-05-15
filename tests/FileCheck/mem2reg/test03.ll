define i32 @foo(i32 %a, i32 %b) {
; CHECK-LABEL: @foo(i32 %a, i32 %b)
; CHECK-NEXT: [[C:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT: [[D:%.*]] = add i32 [[C]], 10
; CHECK-NEXT: ret i32 [[D]]
;
  %c = add i32 %a, %b
  %mem = alloca i32, align 4
  store i32 %c, i32* %mem, align 4
  %ld = load i32, i32* %mem, align 4
  %d = add i32 %ld, 10
  store i32 %d, i32* %mem, align 4
  %e = load i32, i32* %mem, align 4
  ret i32 %e
}
