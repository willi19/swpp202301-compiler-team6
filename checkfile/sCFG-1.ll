; ModuleID = '/tmp/a.ll'
source_filename = "sCFG-1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #0 {
; CHECK-LABEL: @main()
; CHECK:	entry:
; CHECK-NEXT:	[[CALL:%.*]] = call i64 @read()
; CHECK-NEXT:	[[CALL1:%.*]] = call i64 @read()
; CHECK-NEXT:	[[CMP:%.*]] = icmp eq i64 [[CALL]], [[CALL1]]
; CHECK-NEXT:	br i1 [[CMP]], label [[IT:%.*]], label [[IE:%.*]]
; CHECK:	if.then:
; CHECK-NEXT:	[[ADD:%.*]] = add i64 [[CALL]], [[CALL1]]
; CHECK-NEXT:	[[MUL:%.*]] = mul i64 [[CALL]], [[CALL1]]
; CHECK-NEXT:	call void @write(i64 noundef [[ADD]])
; CHECK-NEXT:	call void @write(i64 noundef [[MUL]])
; CHECK-NEXT:	br label [[IE]]
; CHECK:	if.end:
; CHECK-NEXT:	call void @write(i64 noundef[[CALL]])
; CHECK-NEXT:	ret i32, 0
;

entry:
  %call = call i64 @read()
  %call1 = call i64 @read()
  %cmp = icmp eq i64 %call, %call1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %add = add i64 %call, %call1
  %mul = mul i64 %call, %call1
  br label %merge

merge:
  call void @write(i64 noundef %add)
  call void @write(i64 noundef %mul)
  br label %if.end

bb_unreachable:
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
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
