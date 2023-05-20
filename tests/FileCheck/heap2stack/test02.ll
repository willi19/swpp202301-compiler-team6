; CHECK: declare i64 @read(...)
; CHECK: declare noalias i8* @malloc(i64 noundef)
; CHECK: declare void @free(i8* noundef)
; CHECK: define i32 @A(i32* noundef %ptr, i32 noundef %n) {

; CHECK: entry:
; CHECK-NEXT: %cmp = icmp sle i32 %n, 0
; CHECK-NEXT: br i1 %cmp, label %if.end, label %if.then

; CHECK: if.then:
; CHECK-NEXT: %idxprom = sext i32 %n to i64
; CHECK-NEXT: %arrayidx = getelementptr inbounds i32, i32* %ptr, i64 %idxprom
; CHECK-NEXT: %0 = load i32, i32* %arrayidx, align 4
; CHECK-NEXT: %mul = mul nsw i32 %n, %0
; CHECK-NEXT: %div = sdiv i32 %mul, 10
; CHECK-NEXT: %sub = sub nsw i32 %n, 1
; CHECK-NEXT: %call = call i32 @B(i32* noundef %ptr, i32 noundef %sub)
; CHECK-NEXT: %div1 = sdiv i32 %call, 2
; CHECK-NEXT: %add = add nsw i32 %div, %div1
; CHECK-NEXT: br label %if.end

; CHECK: if.end:
; CHECK-NEXT: %ret.0 = phi i32 [ %add, %if.then ], [ 0, %entry ]
; CHECK-NEXT: ret i32 %ret.0

; CHECK: define i32 @B(i32* noundef %ptr, i32 noundef %n) {

; CHECK: entry:
; CHECK-NEXT: %cmp = icmp sle i32 %n, 0
; CHECK-NEXT: br i1 %cmp, label %if.end, label %if.then

; CHECK: if.then:
; CHECK-NEXT: %idxprom = sext i32 %n to i64
; CHECK-NEXT: %arrayidx = getelementptr inbounds i32, i32* %ptr, i64 %idxprom
; CHECK-NEXT: %0 = load i32, i32* %arrayidx, align 4
; CHECK-NEXT: %mul = mul nsw i32 %n, %0
; CHECK-NEXT: %div = sdiv i32 %mul, 7
; CHECK-NEXT: %sub = sub nsw i32 %n, 1
; CHECK-NEXT: %call = call i32 @A(i32* noundef %ptr, i32 noundef %sub)
; CHECK-NEXT: %div1 = sdiv i32 %call, 2
; CHECK-NEXT: %add = add nsw i32 %div, %div1
; CHECK-NEXT: br label %if.end

; CHECK: if.end:
; CHECK-NEXT: %ret.0 = phi i32 [ %add, %if.then ], [ 0, %entry ]
; CHECK-NEXT: ret i32 %ret.0

; CHECK: define i32 @main() {

; CHECK: entry:
; CHECK-NEXT: %call = call i64 (...) @read()
; CHECK-NEXT: %conv = trunc i64 %call to i32
; CHECK-NEXT: %conv1 = sext i32 %conv to i64
; CHECK-NEXT: %mul = mul i64 4, %conv1
; CHECK-NEXT: %call2 = call noalias i8* @malloc(i64 noundef %mul)
; CHECK-NEXT: %0 = bitcast i8* %call2 to i32*
; CHECK-NEXT: br label %for.cond

; CHECK: for.cond:
; CHECK-NEXT: %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
; CHECK-NEXT: %cmp = icmp sge i32 %i.0, %conv
; CHECK-NEXT: br i1 %cmp, label %for.cond.cleanup, label %for.body

; CHECK: for.cond.cleanup:
; CHECK-NEXT: br label %for.end

; CHECK: for.body:
; CHECK-NEXT: %mul4 = mul nsw i32 %i.0, %i.0
; CHECK-NEXT: %div = sdiv i32 %mul4, 2
; CHECK-NEXT: %add = add nsw i32 %i.0, %div
; CHECK-NEXT: %mul5 = mul nsw i32 %i.0, %i.0
; CHECK-NEXT: %mul6 = mul nsw i32 %mul5, %i.0
; CHECK-NEXT: %div7 = sdiv i32 %mul6, 3
; CHECK-NEXT: %add8 = add nsw i32 %add, %div7
; CHECK-NEXT: %idxprom = sext i32 %i.0 to i64
; CHECK-NEXT: %arrayidx = getelementptr inbounds i32, i32* %0, i64 %idxprom
; CHECK-NEXT: store i32 %add8, i32* %arrayidx, align 4
; CHECK-NEXT: br label %for.inc

; CHECK: for.inc:
; CHECK-NEXT: %inc = add nsw i32 %i.0, 1
; CHECK-NEXT: br label %for.cond

; CHECK: for.end:
; CHECK-NEXT: %call9 = call i32 @A(i32* noundef %0, i32 noundef %conv)
; CHECK-NEXT: %1 = bitcast i32* %0 to i8*
; CHECK-NEXT: call void @free(i8* noundef %1)
; CHECK-NEXT: ret i32 0

declare i64 @read(...)

declare noalias i8* @malloc(i64 noundef)

declare void @free(i8* noundef)

define i32 @A(i32* noundef %ptr, i32 noundef %n) {
entry:
  %cmp = icmp sle i32 %n, 0
  br i1 %cmp, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %idxprom = sext i32 %n to i64
  %arrayidx = getelementptr inbounds i32, i32* %ptr, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %mul = mul nsw i32 %n, %0
  %div = sdiv i32 %mul, 10
  %sub = sub nsw i32 %n, 1
  %call = call i32 @B(i32* noundef %ptr, i32 noundef %sub)
  %div1 = sdiv i32 %call, 2
  %add = add nsw i32 %div, %div1
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %ret.0 = phi i32 [ %add, %if.then ], [ 0, %entry ]
  ret i32 %ret.0
}

define i32 @B(i32* noundef %ptr, i32 noundef %n) {
entry:
  %cmp = icmp sle i32 %n, 0
  br i1 %cmp, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %idxprom = sext i32 %n to i64
  %arrayidx = getelementptr inbounds i32, i32* %ptr, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %mul = mul nsw i32 %n, %0
  %div = sdiv i32 %mul, 7
  %sub = sub nsw i32 %n, 1
  %call = call i32 @A(i32* noundef %ptr, i32 noundef %sub)
  %div1 = sdiv i32 %call, 2
  %add = add nsw i32 %div, %div1
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  %ret.0 = phi i32 [ %add, %if.then ], [ 0, %entry ]
  ret i32 %ret.0
}

define i32 @main() {
entry:
  %call = call i64 (...) @read()
  %conv = trunc i64 %call to i32
  %conv1 = sext i32 %conv to i64
  %mul = mul i64 4, %conv1
  %call2 = call noalias i8* @malloc(i64 noundef %mul)
  %0 = bitcast i8* %call2 to i32*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp sge i32 %i.0, %conv
  br i1 %cmp, label %for.cond.cleanup, label %for.body

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %mul4 = mul nsw i32 %i.0, %i.0
  %div = sdiv i32 %mul4, 2
  %add = add nsw i32 %i.0, %div
  %mul5 = mul nsw i32 %i.0, %i.0
  %mul6 = mul nsw i32 %mul5, %i.0
  %div7 = sdiv i32 %mul6, 3
  %add8 = add nsw i32 %add, %div7
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, i32* %0, i64 %idxprom
  store i32 %add8, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond.cleanup
  %call9 = call i32 @A(i32* noundef %0, i32 noundef %conv)
  %1 = bitcast i32* %0 to i8*
  call void @free(i8* noundef %1)
  ret i32 0
}