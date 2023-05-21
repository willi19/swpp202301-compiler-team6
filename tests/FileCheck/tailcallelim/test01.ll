define i32 @factorial(i32 %x, i32 %y) {
    ; CHECK-LABEL: @factorial(i32 %x, i32 %y)
    ; CHECK:      entry:
    ; CHECK-NEXT: br label %tailrecurse
    ; CHECK:      tailrecurse:
    ; CHECK-NEXT: [[XTR:%.*]] = phi i32 [ %x, %entry ], [ [[SUB:%.*]], %if.else ]
    ; CHECK-NEXT: [[YTR:%.*]] = phi i32 [ %y, %entry ], [ [[MUL:%.*]], %if.else ]
    ; CHECK-NEXT: [[CMP:%.*]] = icmp eq i32 [[XTR]], 0
    ; CHECK-NEXT: br i1 [[CMP]], label %if.then, label %if.else
    ; CHECK:      if.then:
    ; CHECK-NEXT: br label %return
    ; CHECK:      if.else:
    ; CHECK-NEXT: [[SUB]] = sub nsw i32 [[XTR]], 1
    ; CHECK-NEXT: [[MUL]] = mul nsw i32 [[YTR]], [[XTR]]
    ; CHECK-NEXT: br label %tailrecurse
    ; CHECK: return:
    ; CHECK-NEXT: ret i32 [[YTR]]
entry:
  %cmp = icmp eq i32 %x, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %return

if.else:                                          ; preds = %entry
  %sub = sub nsw i32 %x, 1
  %mul = mul nsw i32 %y, %x
  %call = call i32 @factorial(i32 %sub, i32 %mul)
  br label %return

return:                                           ; preds = %if.else, %if.then
  %retval.0 = phi i32 [ %y, %if.then ], [ %call, %if.else ]
  ret i32 %retval.0
}

define i32 @main() {
entry:
  %call = call i32 @factorial(i32 10, i32 1)
  ret i32 0
}
