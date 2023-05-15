define i32 @main() {

; CHECK-LABEL: @main()
; CHECK-LABEL: for.cond:
; CHECK-NEXT: [[I:%.*]] = phi i64
; CHECK-LABEL: for.body:
; CHECK: [[P:%.*]] = getelementptr {{.*}} i64 [[I]]
; CHECK: [[V:%.*]] = call i64 @aload_i64(i64* [[P]])
; CHECK: mul nsw i64
; CHECK: add nsw i64
; CHECK: call void @write(i64 noundef [[V]])
; CHECK: call void @write
; CHECK-NEXT: br label

entry:
  %call = call noalias i8* @malloc(i64 noundef 160)
  %0 = bitcast i8* %call to i64*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i64 %i.0, 20
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %add.ptr = getelementptr inbounds i64, i64* %0, i64 %i.0
  %mul = mul nsw i64 %i.0, %i.0
  %mul1 = mul nsw i64 %mul, %i.0
  %mul2 = mul nsw i64 2, %i.0
  %add = add nsw i64 %mul1, %mul2
  %add3 = add nsw i64 %add, 1
  %1 = load i64, i64* %add.ptr, align 8
  call void @write(i64 noundef %1)
  call void @write(i64 noundef %add3)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i64 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  ret i32 0
}


; Function Attrs: allocsize(0)
declare noalias i8* @malloc(i64 noundef)

declare void @write(i64 noundef)
