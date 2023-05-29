define void @functionA(i32 noundef %x) #0 {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %sub = sub nsw i32 %x, 1
  call void @functionB(i32 noundef %sub)
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

define void @functionB(i32 noundef %y) #0 {
entry:
  %cmp = icmp sgt i32 %y, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %sub = sub nsw i32 %y, 1
  call void @functionA(i32 noundef %sub)
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

define i32 @main() #0 {
; CHECK-LABEL:     @main()
; CHECK-NEXT:      call void @functionA(i32 noundef 10)
; CHECK-NEXT:      call void @functionB(i32 noundef 7)
entry:
  call void @functionA(i32 noundef 10)
  call void @functionB(i32 noundef 7)
  ret i32 0
}