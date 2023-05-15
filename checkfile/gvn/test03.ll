define i32 @foo() {
       ; CHECK-LABEL: @foo()
       ; CHECK:      entry:
       ; CHECK-NEXT: ret i32 3       
entry:
  %add = add nsw i32 1, 2
  ret i32 %add
}

define i32 @main() {
entry:
  %call = call i32 @foo()
  ret i32 0
}