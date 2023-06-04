define i32 @fact(i32 noundef %n) #0 {
entry:
  %cmp = icmp eq i32 %n, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %return

if.end:                                           ; preds = %entry
  %sub = sub nsw i32 %n, 1
  %call = call i32 @fact(i32 noundef %sub)
  %mul = mul nsw i32 %call, %n
  br label %return

return:                                           ; preds = %if.end, %if.then
  %retval.0 = phi i32 [ 1, %if.then ], [ %mul, %if.end ]
  ret i32 %retval.0
}

define i32 @max(i32 noundef %a, i32 noundef %b) #0 {
entry:
  %call = call i32 @fact(i32 noundef %a)
  %call1 = call i32 @fact(i32 noundef %b)
  %cmp = icmp slt i32 %call, %call1
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:                                        ; preds = %entry
  br label %cond.end

cond.false:                                       ; preds = %entry
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %b, %cond.true ], [ %a, %cond.false ]
  ret i32 %cond
}

define i32 @main() #0 {
; CHECK-LABEL:     @main()
; CHECK:           [[CALL:%.*]] = call i32 @fact(i32 noundef [[CONV:%.*]])
; CHECK-NEXT:      [[CALL2:%.*]] = call i32 @fact(i32 noundef [[CONV2:%.*]])
; CHECK-NOT:       [[CALL3:%.*]] = call i32 @max(i32 noundef [[CONV]], i32 noundef [[CONV2]])
entry:
  %call = call i64 @read()
  %conv = trunc i64 %call to i32
  %call1 = call i64 @read()
  %conv2 = trunc i64 %call1 to i32
  %call3 = call i32 @max(i32 noundef %conv, i32 noundef %conv2)
  ret i32 0
}
declare i64 @read() #2
