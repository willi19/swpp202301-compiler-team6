define i32 @main(i32 %b, i32 %a) {
; CHECK: start main 3:
  %cond = icmp eq i32 %a, %b
  br i1 %cond, label %bb_true, label %bb_false
bb_true:
  call void @f(i32 %a, i32 %b)
  br label %bb_exit
bb_false:
  call void @f(i32 %a, i32 %b)
  br label %bb_exit
bb_exit:
  call void @f(i32 %a, i32 %b)
  ret i32 %b
}

declare void @f(i32, i32)
