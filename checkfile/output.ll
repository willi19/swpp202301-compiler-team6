; ModuleID = 'Mem2Reg-Test3.ll'
source_filename = "Mem2Reg-Test3.ll"

define i32 @foo(i32 %a, i32 %b) {
  %c = add i32 %a, %b
  %d = add i32 %c, 10
  ret i32 %d
}
