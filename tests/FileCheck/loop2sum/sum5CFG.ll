; ModuleID = 'checkfile/loop2sum/sum5.ll'
source_filename = "checkfile/loop2sum/sum5.ll"

define dso_local i32 @main() {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i64 [ 0, %entry ], [ %inc, %for.body ]
  %sum.0 = phi i64 [ 0, %entry ], [ %add, %for.body ]
  %sum2.0 = phi i64 [ 0, %entry ], [ %add1, %for.body ]
  %cmp = icmp ult i64 %i.0, 20
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %i.0, %i.0
  %add = add i64 %sum.0, %mul
  %add1 = add i64 %sum2.0, %add
  %inc = add i64 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  call void @write(i64 noundef %sum.0)
  call void @write(i64 noundef %sum2.0)
  ret i32 0
}

declare void @write(i64 noundef)
