; CHECK: @oracle
define dso_local i8* @malloc_upto_8(i64 noundef %x) #0 {
entry:
  %add = add i64 %x, 7
  %div = udiv i64 %add, 8
  %mul = mul i64 %div, 8
  %call = call noalias i8* @malloc(i64 noundef %mul) #4
  ret i8* %call
}

declare noalias i8* @malloc(i64 noundef) #1

define dso_local i32 @main() #0 {
entry:
  %call = call i64 (...) @read()
  %mul = mul i64 4, %call
  %call1 = call i8* @malloc_upto_8(i64 noundef %mul)
  %0 = bitcast i8* %call1 to i32*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %conv = sext i32 %i.0 to i64
  %cmp = icmp ult i64 %conv, %call
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %call3 = call i64 (...) @read()
  %conv4 = trunc i64 %call3 to i32
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, i32* %0, i64 %idxprom
  store i32 %conv4, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc14, %for.end
  %i5.0 = phi i32 [ 0, %for.end ], [ %inc15, %for.inc14 ]
  %conv7 = sext i32 %i5.0 to i64
  %cmp8 = icmp ult i64 %conv7, %call
  br i1 %cmp8, label %for.body11, label %for.cond.cleanup10

for.cond.cleanup10:                               ; preds = %for.cond6
  br label %for.end16

for.body11:                                       ; preds = %for.cond6
  %idxprom12 = sext i32 %i5.0 to i64
  %arrayidx13 = getelementptr inbounds i32, i32* %0, i64 %idxprom12
  store i32 0, i32* %arrayidx13, align 4
  br label %for.inc14

for.inc14:                                        ; preds = %for.body11
  %inc15 = add nsw i32 %i5.0, 1
  br label %for.cond6

for.end16:                                        ; preds = %for.cond.cleanup10
  ret i32 0
}

declare i64 @read(...) #3