define i32 @sumArray(i32* %arr, i32 %n) {
       ; CHECK-LABEL: @sumArray(i32* %arr, i32 %n)
       ; CHECK:      entry:
       ; CHECK-NEXT: br label %for.cond
       ; CHECK:      for.cond:
       ; CHECK-NEXT: [[SUM0:%.*]] = phi i32 [ 0, %entry ], [ [[ADD:%.*]], %for.body ]
       ; CHECK-NEXT: [[I0:%.*]] = phi i32 [ 0, %entry ], [ [[INC:%.*]], %for.body ]
       ; CHECK-NEXT: [[CMP:%.*]] = icmp slt i32 [[I0]], %n
       ; CHECK-NEXT: br i1 [[CMP]], label %for.body, label %for.cond.cleanup
       ; CHECK:      for.cond.cleanup:
       ; CHECK-NEXT: ret i32 [[SUM0]]
       ; CHECK:      for.body:
       ; CHECK-NEXT: [[IDXPROM:%.*]] = sext i32 [[I0]] to i64
       ; CHECK-NEXT: [[ARRIDX:%.*]] = getelementptr inbounds i32, i32* %arr, i64 [[IDXPROM]]
       ; CHECK-NEXT: %0 = load i32, i32* [[ARRIDX]], align 4
       ; CHECK-NEXT: [[ADD]] = add nsw i32 [[SUM0]], %0
       ; CHECK-NEXT: [[INC]] = add nsw i32 [[I0]], 1
       ; CHECK-NEXT: br label %for.cond
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %sum.0 = phi i32 [ 0, %entry ], [ %add, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, i32* %arr, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %add = add nsw i32 %sum.0, %0
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  ret i32 %sum.0
}

define i32 @main() {
entry:
  %arr = alloca [3 x i32], align 4
  %0 = bitcast [3 x i32]* %arr to i8*
  call void @llvm.memset.p0i8.i64(i8* align 4 %0, i8 0, i64 12, i1 false)
  %arraydecay = getelementptr inbounds [3 x i32], [3 x i32]* %arr, i64 0, i64 0
  %call = call i32 @sumArray(i32* %arraydecay, i32 3)
  ret i32 0
}

declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)