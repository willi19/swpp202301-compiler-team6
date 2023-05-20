; Check free_wrap function
; CHECK-LABEL: @free_wrap
; CHECK: %0 = bitcast i64* %ptr to i8*
; CHECK: %sp.as.int = ptrtoint i8* %0 to i64
; CHECK: %cmp.sp = icmp sge i64 %sp.as.int, 102400
; CHECK: br i1 %cmp.sp, label %free.entry, label %div.free.entry

; Check div.free.entry block
; CHECK-LABEL: div.free.entry:
; CHECK: ret void

; Check free.entry block
; CHECK-LABEL: free.entry:
; CHECK: call void @free(i8* %0)
; CHECK: br label %div.free.entry

; Check main function
; CHECK-LABEL: @main
; CHECK: %call = call i64 (...) @read()
; CHECK: %mul = mul i64 8, %call
; CHECK: %cur.sp = call i64 @"$decr_sp"(i64 0)
; CHECK: %space.left = sub i64 %cur.sp, %mul
; CHECK: %alloca_possible = icmp sge i64 %space.left, 20480
; CHECK: br i1 %alloca_possible, label %stack.entry, label %heap.entry

; Check div.entry block
; CHECK-LABEL: div.entry:
; CHECK: %alloc.call1 = phi i8* [ %6, %stack.entry ], [ %by.malloc, %heap.entry ]
; CHECK: %0 = bitcast i8* %alloc.call1 to i64*
; CHECK: %mul2 = mul i64 8, %call
; CHECK: %call3 = call i8* @malloc_upto_8(i64 noundef %mul2)
; CHECK: %1 = bitcast i8* %call3 to i64*
; CHECK: br label %for.cond

; Check for.end24 block
; CHECK-LABEL: for.end24:
; CHECK: call void @write(i64 noundef %ans.1)
; CHECK: %5 = bitcast i64* %0 to i8*
; CHECK: %sp.as.int = ptrtoint i8* %5 to i64
; CHECK: %cmp.sp = icmp sge i64 %sp.as.int, 102400
; CHECK: br i1 %cmp.sp, label %free.for.end24, label %div.free.for.end24

; Check div.free.for.end24 block
; CHECK-LABEL: div.free.for.end24:
; CHECK: call void @free_wrap(i64* noundef %1)
; CHECK: ret i32 0

; Check alloca.entry block
; CHECK-LABEL: stack.entry:
; CHECK: %by.alloca = call i64 @"$decr_sp"(i64 %mul)
; CHECK: %6 = inttoptr i64 %by.alloca to i8*
; CHECK: br label %div.entry

; Check heap.entry block
; CHECK-LABEL: heap.entry:
; CHECK: %by.malloc = call i8* @malloc(i64 %mul)
; CHECK: br label %div.entry

; Check free.for.end24 block
; CHECK-LABEL: free.for.end24:
; CHECK: call void @free(i8* %5)
; CHECK: br label %div.free.for.end24

define dso_local i8* @malloc_upto_8(i64 noundef %x) #0 {
entry:
  %add = add i64 %x, 7
  %div = udiv i64 %add, 8
  %mul = mul i64 %div, 8
  %call = call noalias i8* @malloc(i64 noundef %mul) #5
  ret i8* %call
}

declare noalias i8* @malloc(i64 noundef) #1

define dso_local void @free_wrap(i64* noundef %ptr) #0 {
entry:
  %0 = bitcast i64* %ptr to i8*
  call void @free(i8* noundef %0) #6
  ret void
}

declare void @free(i8* noundef) #2

define dso_local i32 @main() #0 {
entry:
  %call = call i64 (...) @read()
  %mul = mul i64 8, %call
  %call1 = call noalias i8* @malloc(i64 noundef %mul) #5
  %0 = bitcast i8* %call1 to i64*
  %mul2 = mul i64 8, %call
  %call3 = call i8* @malloc_upto_8(i64 noundef %mul2)
  %1 = bitcast i8* %call3 to i64*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %ans.0 = phi i64 [ 0, %entry ], [ %add6, %for.inc ]
  %i.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %i.0, %call
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %mul4 = mul i64 %ans.0, 2
  %add = add i64 %i.0, %mul4
  %rem = urem i64 %add, 3
  %arrayidx = getelementptr inbounds i64, i64* %0, i64 %i.0
  store i64 %rem, i64* %arrayidx, align 8
  %arrayidx5 = getelementptr inbounds i64, i64* %0, i64 %i.0
  %2 = load i64, i64* %arrayidx5, align 8
  %add6 = add i64 %ans.0, %2
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  call void @write(i64 noundef %ans.0)
  br label %for.cond8

for.cond8:                                        ; preds = %for.inc22, %for.end
  %ans.1 = phi i64 [ %ans.0, %for.end ], [ %add21, %for.inc22 ]
  %i7.0 = phi i32 [ 0, %for.end ], [ %inc23, %for.inc22 ]
  %conv = sext i32 %i7.0 to i64
  %cmp9 = icmp ult i64 %conv, %call
  br i1 %cmp9, label %for.body12, label %for.cond.cleanup11

for.cond.cleanup11:                               ; preds = %for.cond8
  br label %for.end24

for.body12:                                       ; preds = %for.cond8
  %idxprom = sext i32 %i7.0 to i64
  %arrayidx13 = getelementptr inbounds i64, i64* %0, i64 %idxprom
  %3 = load i64, i64* %arrayidx13, align 8
  %conv14 = sext i32 %i7.0 to i64
  %add15 = add i64 %3, %conv14
  %rem16 = urem i64 %add15, 10
  %idxprom17 = sext i32 %i7.0 to i64
  %arrayidx18 = getelementptr inbounds i64, i64* %1, i64 %idxprom17
  store i64 %rem16, i64* %arrayidx18, align 8
  %idxprom19 = sext i32 %i7.0 to i64
  %arrayidx20 = getelementptr inbounds i64, i64* %1, i64 %idxprom19
  %4 = load i64, i64* %arrayidx20, align 8
  %add21 = add i64 %ans.1, %4
  br label %for.inc22

for.inc22:                                        ; preds = %for.body12
  %inc23 = add nsw i32 %i7.0, 1
  br label %for.cond8

for.end24:                                        ; preds = %for.cond.cleanup11
  call void @write(i64 noundef %ans.1)
  %5 = bitcast i64* %0 to i8*
  call void @free(i8* noundef %5) #6
  call void @free_wrap(i64* noundef %1)
  ret i32 0
}

declare i64 @read(...) #4
declare void @write(i64 noundef) #4
