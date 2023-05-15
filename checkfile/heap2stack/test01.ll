; CHECK: declare i64 @read(...)
; CHECK: declare noalias i8* @malloc(i64 noundef)
; CHECK: declare void @write(i64 noundef)
; CHECK: define dso_local i32 @main() {

; CHECK-LABEL: entry:
; CHECK: %call = call i64 (...) @read()
; CHECK: %mul = mul i64 8, %call
; CHECK: %cur.sp = call i64 @"$decr_sp"(i64 0)
; CHECK: %left.space = mul i64 %cur.sp, 4
; CHECK: %req.space = mul i64 %mul, 5
; CHECK: %alloca_possible = icmp sge i64 %left.space, %req.space
; CHECK: br i1 %alloca_possible, label %alloca.entry, label %malloc.entry

; Check div.entry block
; CHECK-LABEL: div.entry:
; CHECK: %allocation.call1 = phi i8* [ %6, %alloca.entry ], [ %by.malloc, %malloc.entry ]
; CHECK: %0 = bitcast i8* %allocation.call1 to i64*
; CHECK: %mul2 = mul i64 8, %call
; CHECK: %cur.sp1 = call i64 @"$decr_sp"(i64 0)
; CHECK: %left.space2 = mul i64 %cur.sp1, 4
; CHECK: %req.space3 = mul i64 %mul2, 5
; CHECK: %alloca_possible4 = icmp sge i64 %left.space2, %req.space3
; CHECK: br i1 %alloca_possible4, label %alloca.div.entry, label %malloc.div.entry

; Check div.div.entry block
; CHECK-LABEL: div.div.entry:
; CHECK: %allocation.call3 = phi i8* [ %7, %alloca.div.entry ], [ %by.malloc6, %malloc.div.entry ]
; CHECK: %1 = bitcast i8* %allocation.call3 to i64*
; CHECK: br label %for.cond

; Check div.free.for.end26 block
; CHECK-LABEL: div.free.for.end26:
; CHECK: %5 = bitcast i64* %1 to i8*
; CHECK: %sp.as.int7 = ptrtoint i8* %5 to i64
; CHECK: %cmp.sp8 = icmp sge i64 %sp.as.int7, 102400
; CHECK: br i1 %cmp.sp8, label %free.div.free.for.end26, label %div.free.div.free.for.end26

; Check div.free.div.free.for.end26 block
; CHECK-LABEL: div.free.div.free.for.end26:
; CHECK: ret i32 0

; Check alloca.entry block
; CHECK-LABEL: alloca.entry:
; CHECK: %by.alloca = call i64 @"$decr_sp"(i64 %mul)
; CHECK: %6 = inttoptr i64 %by.alloca to i8*
; CHECK: br label %div.entry

; Check malloc.entry block
; CHECK-LABEL: malloc.entry:
; CHECK: %by.malloc = call i8* @malloc(i64 %mul)
; CHECK: br label %div.entry

; Check alloca.div.entry block
; CHECK-LABEL: alloca.div.entry:
; CHECK: %by.alloca5 = call i64 @"$decr_sp"(i64 %mul2)
; CHECK: %7 = inttoptr i64 %by.alloca5 to i8*
; CHECK: br label %div.div.entry

; Check malloc.div.entry block
; CHECK-LABEL: malloc.div.entry:
; CHECK: %by.malloc6 = call i8* @malloc(i64 %mul2)
; CHECK: br label %div.div.entry

; Check free.for.end26 block
; CHECK-LABEL: free.for.end26:
; CHECK: call void @free(i8* %4)
; CHECK: br label %div.free.for.end26

; Check free.div.free.for.end26 block
; CHECK-LABEL: free.div.free.for.end26:
; CHECK: call void @free(i8* %5)
; CHECK: br label %div.free.div.free.for.end26


declare i64 @read(...) #2
declare noalias i8* @malloc(i64 noundef) #3
declare void @write(i64 noundef) #2
declare void @free(i8* noundef) #4

define dso_local i32 @main() #0 {
entry:
  %call = call i64 (...) @read()
  %mul = mul i64 8, %call
  %call1 = call noalias i8* @malloc(i64 noundef %mul) #5
  %0 = bitcast i8* %call1 to i64*
  %mul2 = mul i64 8, %call
  %call3 = call noalias i8* @malloc(i64 noundef %mul2) #5
  %1 = bitcast i8* %call3 to i64*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %ans.0 = phi i64 [ 0, %entry ], [ %add9, %for.inc ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %conv = sext i32 %i.0 to i64
  %cmp = icmp ult i64 %conv, %call
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %mul5 = mul nsw i32 %i.0, 2
  %add = add nsw i32 %mul5, 1
  %conv6 = sext i32 %add to i64
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i64, i64* %0, i64 %idxprom
  store i64 %conv6, i64* %arrayidx, align 8
  %idxprom7 = sext i32 %i.0 to i64
  %arrayidx8 = getelementptr inbounds i64, i64* %0, i64 %idxprom7
  %2 = load i64, i64* %arrayidx8, align 8
  %add9 = add i64 %ans.0, %2
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  call void @write(i64 noundef %ans.0)
  br label %for.cond11

for.cond11:                                       ; preds = %for.inc24, %for.end
  %ans.1 = phi i64 [ %ans.0, %for.end ], [ %add23, %for.inc24 ]
  %i10.0 = phi i32 [ 0, %for.end ], [ %inc25, %for.inc24 ]
  %conv12 = sext i32 %i10.0 to i64
  %cmp13 = icmp ult i64 %conv12, %call
  br i1 %cmp13, label %for.body16, label %for.cond.cleanup15

for.cond.cleanup15:                               ; preds = %for.cond11
  br label %for.end26

for.body16:                                       ; preds = %for.cond11
  %mul17 = mul nsw i32 2, %i10.0
  %conv18 = sext i32 %mul17 to i64
  %idxprom19 = sext i32 %i10.0 to i64
  %arrayidx20 = getelementptr inbounds i64, i64* %1, i64 %idxprom19
  store i64 %conv18, i64* %arrayidx20, align 8
  %idxprom21 = sext i32 %i10.0 to i64
  %arrayidx22 = getelementptr inbounds i64, i64* %1, i64 %idxprom21
  %3 = load i64, i64* %arrayidx22, align 8
  %add23 = add i64 %ans.1, %3
  br label %for.inc24

for.inc24:                                        ; preds = %for.body16
  %inc25 = add nsw i32 %i10.0, 1
  br label %for.cond11

for.end26:                                        ; preds = %for.cond.cleanup15
  call void @write(i64 noundef %ans.1)
  %4 = bitcast i64* %0 to i8*
  call void @free(i8* noundef %4) #6
  %5 = bitcast i64* %1 to i8*
  call void @free(i8* noundef %5) #6
  ret i32 0
}

