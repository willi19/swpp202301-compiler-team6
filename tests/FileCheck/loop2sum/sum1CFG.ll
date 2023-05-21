define i32 @main() {
entry:
  %a = alloca [20 x i64], align 16
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i64 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp ult i64 %i.0, 20
  br i1 %cmp, label %for.body, label %for.cond1

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %i.0, %i.0
  %arrayidx = getelementptr inbounds [20 x i64], [20 x i64]* %a, i64 0, i64 %i.0
  store i64 %mul, i64* %arrayidx, align 8
  %inc = add i64 %i.0, 1
  br label %for.cond

for.cond1:                                        ; preds = %for.cond, %for.body3
  %i.1 = phi i64 [ %inc6, %for.body3 ], [ 0, %for.cond ]
  %sum.0 = phi i64 [ %add, %for.body3 ], [ 0, %for.cond ]
  %cmp2 = icmp ult i64 %i.1, 20
  br i1 %cmp2, label %for.body3, label %for.end7

for.body3:                                        ; preds = %for.cond1
  %arrayidx4 = getelementptr inbounds [20 x i64], [20 x i64]* %a, i64 0, i64 %i.1
  %0 = load i64, i64* %arrayidx4, align 8
  %add = add i64 %sum.0, %0
  %inc6 = add i64 %i.1, 1
  br label %for.cond1

for.end7:                                         ; preds = %for.cond1
  call void @write(i64 noundef %sum.0)
  ret i32 0
}

declare void @write(i64 noundef)
