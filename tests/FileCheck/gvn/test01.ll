define i32 @foo(i32 %x, i32 %y) {
       ; CHECK-LABEL: @foo(i32 %x, i32 %y)
       ; CHECK:       entry:
       ; CHECK-NEXT: [[ADD:%.*]] = add nsw i32 %x, %y
       ; CHECK-NEXT: [[ADD2:%.*]]  = add nsw i32 [[ADD]], [[ADD]]
       ; CHECK-NEXT: ret i32 [[ADD2]]
entry:
  %add = add nsw i32 %x, %y
  %add1 = add nsw i32 %x, %y
  %add2 = add nsw i32 %add, %add1
  ret i32 %add2
}

define i32 @main() {
entry:
  %call = call i32 @foo(i32 3, i32 5)
  ret i32 0
}
