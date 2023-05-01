; ModuleID = '/tmp/a.ll'
source_filename = "../swpp202301-compiler-team6/checkfile/sCFG-2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #0 {
; CHECK-LABEL: @main()
; CHECK:	entry:
; CHECK-NEXT:	[[CALL:%.*]] = call i64 @read()
; CHECK-NEXT:	switch i64 [[CALL]], label [[IE:%.*]] [
; CHECK-NEXT:	  i64 1, label [[IT1:%.*]]
; CHECK-NEXT:	  i64 2, label [[IT2:%.*]]
; CHECK:	if.then:
; CHECK-NEXT:	call void @write(i64 noundef [[CALL]])
; CHECK-NEXT:	br label [[IE:%.*]]
; CHECK:	if.then2:
; CHECK-NEXT:	[[ADD:%.*]] = add i64 [[CALL]], 1
; CHECK-NEXT:	call void @write(i64 noundef [[ADD]])
; CHECK-NEXT:	br label [[IE]]
; CHECK:	if.else3:
; CHECK-NEXT:	[[ADD2:%.*]] = add i64 [[CALL]], 2
; CHECK-NEXT:	call void @write(i64 noundef [[ADD2]])
; CHECK:	if.end5:
; CHECK-NEXT:	call void @write(i64 noundef [[CALL]])
; CHECK-NEXT:	ret i32 0
;

entry:
  %call = call i64 @read()
  %cmp = icmp eq i64 %call, 1
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  call void @write(i64 noundef %call)
  br label %if.end5

if.else:                                          ; preds = %entry
  %cmp1 = icmp eq i64 %call, 2
  br i1 %cmp1, label %if.then2, label %if.else3

if.then2:                                         ; preds = %if.else
  %add = add i64 %call, 1
  call void @write(i64 noundef %add)
  br label %if.end

if.else3:                                         ; preds = %if.else
  %add4 = add i64 %call, 2
  call void @write(i64 noundef %add4)
  br label %if.end

if.end:                                           ; preds = %if.else3, %if.then2
  br label %if.end5

if.end5:                                          ; preds = %if.end, %if.then
  call void @write(i64 noundef %call)
  ret i32 0
}

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

declare i64 @read() #2

declare void @write(i64 noundef) #2

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

attributes #0 = { nounwind uwtable "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { argmemonly nocallback nofree nosync nounwind willreturn }
attributes #2 = { "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
