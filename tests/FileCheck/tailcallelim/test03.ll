define i32 @sumOfDigits(i32 %n) {
    ; CHECK-LABEL: @sumOfDigits(i32 %n)
    ; CHECK:      entry:
    ; CHECK-NEXT: br label %tailrecurse
    ; CHECK:      tailrecurse:
    ; CHECK-NEXT: [[ACCTR:%.*]] = phi i32 [ 0, %entry ], [ [[ADD:%.*]], %if.else ]
    ; CHECK-NEXT: [[NTR:%.*]] = phi i32 [ [[N:%.*]], %entry ], [ [[DIV:%.*]], %if.else ]
    ; CHECK-NEXT: [[CMP:%.*]] = icmp eq i32 [[NTR]], 0
    ; CHECK-NEXT: br i1 [[CMP]], label %if.then, label %if.else
    ; CHECK:      if.then:
    ; CHECK-NEXT: br label %return
    ; CHECK:      if.else:
    ; CHECK-NEXT: [[REM:%.*]] = srem i32 [[NTR]], 10
    ; CHECK-NEXT: [[DIV]] = sdiv i32 [[NTR]], 10
    ; CHECK-NEXT: [[ADD]] = add nsw i32 [[REM]], [[ACCTR]]
    ; CHECK-NEXT: br label %tailrecurse
    ; CHECK:      return:
    ; CHECK-NEXT: [[ACCRETTR:%.*]] = add nsw i32 0, [[ACCTR]]
    ; CHECK-NEXT: ret i32 [[ACCRETTR]]
entry:
  %cmp = icmp eq i32 %n, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %return

if.else:                                          ; preds = %entry
  %rem = srem i32 %n, 10
  %div = sdiv i32 %n, 10
  %call = call i32 @sumOfDigits(i32 noundef %div)
  %add = add nsw i32 %rem, %call
  br label %return

return:                                           ; preds = %if.else, %if.then
  %retval.0 = phi i32 [ 0, %if.then ], [ %add, %if.else ]
  ret i32 %retval.0
}

define i32 @main() #0 {
entry:
  %call = call i32 @sumOfDigits(i32 noundef 958291734)
  ret i32 0
}
