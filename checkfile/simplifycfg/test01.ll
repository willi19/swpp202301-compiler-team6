define i32 @main(i32 %a, i32 %b) {
; CHECK-LABEL: @main(i32 %a, i32 %b)
; CHECK-NEXT:	[[COND:%.*]] = icmp eq i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:	br i1 [[COND]], label [[BB_T:%.*]], label [[EXIT:%.*]]
; CHECK:	bb_true:
; CHECK-NEXT:   [[C:%.*]] = add i32 [[A]], [[B]]
; CHECK-NEXT:   [[D:%.*]] = mul i32 [[A]], [[B]]
; CHECK-NEXT:	br label [[EXIT]]
; CHECK:	exit:
; CHECK-NEXT:   ret i32 [[A]]
;
  %cond = icmp eq i32 %a, %b
  br i1 %cond, label %bb_true, label %bb_false
bb_true:
  %c = add i32 %a, %b
  br label %bb_merge
bb_false:
  br label %exit
bb_merge:
  %d = mul i32 %a, %b
  br label %exit
bb_unreachable:
  br label %exit
exit:
  ret i32 %a
}
