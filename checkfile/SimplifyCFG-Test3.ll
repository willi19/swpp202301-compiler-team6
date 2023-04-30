declare void @f(i32)

define i32 @main(i32 %a) {
; CHECK-LABEL: @main(i32 %a)
; CHECK-NEXT:	[[COND:%.*]] = icmp eq i32 [[A:%.*]], 1
; CHECK-NEXT:	br i1 [[COND]], label [[CASE:%.*]], label [[E:%,*]]
; CHECK:	case2:
; CHECK-NEXT:	call void @f([[A]])
; CHECK-NEXT:	br label [[E]]
; CHECK:	exit:
; CHECK-NEXT:	ret i32 [[A]]
;
  %cond1 = icmp eq i32 %a, 1
  br i1 %cond1, label %case1, label %exit
case1:
  %cond2 = icmp eq i32 %a, 1
  br i1 %cond2, label %case2, label %exit
case2:
  call void @f(i32 %a)
  br label %exit
exit:
  ret i32 %a
}
