declare void @foo2(i32* %x)

define i32 @foo(i32 %a, i32 %b) {
; CHECK-LABEL: @foo(i32 %a, i32 %b)
; CHECK-NEXT: [[C:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT: [[M:%.*]] = alloca i32, align 4
; CHECK-NEXT: store i32 [[C]], i32* [[M]], align 4
; CHECK-NEXT: call void @foo2(i32* [[M]])
; CHECK-NEXT: [[L:%.*]] = load i32, i32* [[M]], align 4
; CHECK-NEXT: [[D:%.*]] = add i32 [[L]], 10
; CHECK-NEXT: ret i32 [[D]]
;
  %c = add i32 %a, %b
  %mem = alloca i32, align 4
  store i32 %c, i32* %mem, align 4
  call void @foo2(i32* %mem)
  %ld = load i32, i32* %mem, align 4
  %d = add i32 %ld, 10
  ret i32 %d
}
