define i8* @malloc_upto_8(i64 noundef %x) #0 {
entry:
  %add = add i64 %x, 7
  %div = udiv i64 %add, 8
  %mul = mul i64 %div, 8
  %call = call noalias i8* @malloc(i64 noundef %mul) #4
  ret i8* %call
}

declare noalias i8* @malloc(i64 noundef) #1

define i32 @main() #0 {
; CHECK-LABEL:     @main()
; CHECK:           [[ADD:%.*]] = add i64 [[CALL:%.*]], 7
; CHECK-NEXT:      [[DIV:%.*]] = udiv i64 [[ADD:%.*]], 8
; CHECK-NEXT:      [[MUL:%.*]] = mul i64 [[DIV:%.*]], 8
; CHECK-NEXT:      [[CALLI:%.*]] = call noalias i8* @malloc(i64 noundef [[MUL]])
entry:
  %call = call i64 @read()
  %call1 = call i8* @malloc_upto_8(i64 noundef %call)
  %0 = bitcast i8* %call1 to i16*
  %1 = bitcast i16* %0 to i8*
  ret i32 0
}

declare i64 @read() #3
