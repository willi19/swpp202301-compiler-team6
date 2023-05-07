declare void @f(i32)

define i32 @main(i32 %a) {
; CHECK-LABEL: @main(i32 %a)
; CHECK-NEXT:	switch i32 [[A:%.*]], label [[DEF:%.*]] [
; CHECK-NEXT:	i32 1, label [[C1:%.*]]
; CHECK-NEXT:	i32 2, label [[C2:%.*]]
; CHECK:	case1:
; CHECK-NEXT:	call void @f(i32 [[A]])
; CHECK-NEXT:	br label [[E:%.*]]
; CHECK:	case2:
; CHECK-NEXT:	[[B:%.*]] = add i32 [[A]], 1
; CHECK-NEXT:	call void @f(i32 [[B]])
; CHECK-NEXT:	br label [[E]]
; CHECK:	else2:
; CHECK-NEXT:	[[C:%.*]] = add i32 [[A]], 2
; CHECK-NEXT:	call void @f(i32 [[C]])
; CHECK-NEXT:	br label [[E]]
; CHECK:	exit:
; CHECK-NEXT:	ret i32 [[A]]
;
  %cond1 = icmp eq i32 %a, 1
  br i1 %cond1, label %case1, label %else1
case1:
  call void @f(i32 %a)
  br label %exit
else1:
  %cond2 = icmp eq i32 %a, 2
  br i1 %cond2, label %case2, label %else2
case2:
  %b = add i32 %a, 1
  call void @f(i32 %b)
  br label %exit
else2:
  %c = add i32 %a, 2
  call void @f(i32 %c)
  br label %exit
exit:
  ret i32 %a
}
