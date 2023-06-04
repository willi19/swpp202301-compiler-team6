; ModuleID = '/tmp/a.ll'
source_filename = "mine1/src/mine1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i8* @malloc_upto_8(i64 noundef %x) #0 {
entry:
  %add = add i64 %x, 7
  %div = udiv i64 %add, 8
  %mul = mul i64 %div, 8
  %call = call noalias i8* @malloc(i64 noundef %mul) #4
  ret i8* %call
}

; Function Attrs: nounwind allocsize(0)
declare noalias i8* @malloc(i64 noundef) #1

; Function Attrs: nounwind uwtable
define dso_local i32 @main() #0 {
; CHECK-NOT: @oracle
entry:
  %call = call i64 (...) @read()
  %mul = mul i64 4, %call
  %call1 = call i8* @malloc_upto_8(i64 noundef %mul)
  %0 = bitcast i8* %call1 to i32*
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %conv = sext i32 %i.0 to i64
  %cmp = icmp ult i64 %conv, %call
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  br label %for.end

for.body:                                         ; preds = %for.cond
  %call3 = call i64 (...) @read()
  %conv4 = trunc i64 %call3 to i32
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, i32* %0, i64 %idxprom
  store i32 %conv4, i32* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !5

for.end:                                          ; preds = %for.cond.cleanup
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc15, %for.end
  %i5.0 = phi i32 [ 0, %for.end ], [ %inc16, %for.inc15 ]
  %conv7 = sext i32 %i5.0 to i64
  %cmp8 = icmp ult i64 %conv7, %call
  br i1 %cmp8, label %for.body11, label %for.cond.cleanup10

for.cond.cleanup10:                               ; preds = %for.cond6
  br label %for.end17

for.body11:                                       ; preds = %for.cond6
  %idxprom12 = sext i32 %i5.0 to i64
  %arrayidx13 = getelementptr inbounds i32, i32* %0, i64 %idxprom12
  %1 = load i32, i32* %arrayidx13, align 4
  %conv14 = sext i32 %1 to i64
  call void @write(i64 noundef %conv14)
  br label %for.inc15

for.inc15:                                        ; preds = %for.body11
  %inc16 = add nsw i32 %i5.0, 1
  br label %for.cond6, !llvm.loop !8

for.end17:                                        ; preds = %for.cond.cleanup10
  br label %for.cond19

for.cond19:                                       ; preds = %for.inc115, %for.end17
  %i18.0 = phi i32 [ 0, %for.end17 ], [ %inc116, %for.inc115 ]
  %conv20 = sext i32 %i18.0 to i64
  %cmp21 = icmp ult i64 %conv20, %call
  br i1 %cmp21, label %for.body24, label %for.cond.cleanup23

for.cond.cleanup23:                               ; preds = %for.cond19
  br label %for.end117

for.body24:                                       ; preds = %for.cond19
  %idxprom25 = sext i32 %i18.0 to i64
  %arrayidx26 = getelementptr inbounds i32, i32* %0, i64 %idxprom25
  store i32 1, i32* %arrayidx26, align 4
  %idxprom27 = sext i32 %i18.0 to i64
  %arrayidx28 = getelementptr inbounds i32, i32* %0, i64 %idxprom27
  store i32 2, i32* %arrayidx28, align 4
  %idxprom29 = sext i32 %i18.0 to i64
  %arrayidx30 = getelementptr inbounds i32, i32* %0, i64 %idxprom29
  store i32 3, i32* %arrayidx30, align 4
  %idxprom31 = sext i32 %i18.0 to i64
  %arrayidx32 = getelementptr inbounds i32, i32* %0, i64 %idxprom31
  store i32 4, i32* %arrayidx32, align 4
  %idxprom33 = sext i32 %i18.0 to i64
  %arrayidx34 = getelementptr inbounds i32, i32* %0, i64 %idxprom33
  store i32 5, i32* %arrayidx34, align 4
  %idxprom35 = sext i32 %i18.0 to i64
  %arrayidx36 = getelementptr inbounds i32, i32* %0, i64 %idxprom35
  store i32 1, i32* %arrayidx36, align 4
  %idxprom37 = sext i32 %i18.0 to i64
  %arrayidx38 = getelementptr inbounds i32, i32* %0, i64 %idxprom37
  store i32 2, i32* %arrayidx38, align 4
  %idxprom39 = sext i32 %i18.0 to i64
  %arrayidx40 = getelementptr inbounds i32, i32* %0, i64 %idxprom39
  store i32 3, i32* %arrayidx40, align 4
  %idxprom41 = sext i32 %i18.0 to i64
  %arrayidx42 = getelementptr inbounds i32, i32* %0, i64 %idxprom41
  store i32 4, i32* %arrayidx42, align 4
  %idxprom43 = sext i32 %i18.0 to i64
  %arrayidx44 = getelementptr inbounds i32, i32* %0, i64 %idxprom43
  store i32 5, i32* %arrayidx44, align 4
  %idxprom45 = sext i32 %i18.0 to i64
  %arrayidx46 = getelementptr inbounds i32, i32* %0, i64 %idxprom45
  store i32 1, i32* %arrayidx46, align 4
  %idxprom47 = sext i32 %i18.0 to i64
  %arrayidx48 = getelementptr inbounds i32, i32* %0, i64 %idxprom47
  store i32 2, i32* %arrayidx48, align 4
  %idxprom49 = sext i32 %i18.0 to i64
  %arrayidx50 = getelementptr inbounds i32, i32* %0, i64 %idxprom49
  store i32 3, i32* %arrayidx50, align 4
  %idxprom51 = sext i32 %i18.0 to i64
  %arrayidx52 = getelementptr inbounds i32, i32* %0, i64 %idxprom51
  store i32 4, i32* %arrayidx52, align 4
  %idxprom53 = sext i32 %i18.0 to i64
  %arrayidx54 = getelementptr inbounds i32, i32* %0, i64 %idxprom53
  store i32 5, i32* %arrayidx54, align 4
  %idxprom55 = sext i32 %i18.0 to i64
  %arrayidx56 = getelementptr inbounds i32, i32* %0, i64 %idxprom55
  store i32 1, i32* %arrayidx56, align 4
  %idxprom57 = sext i32 %i18.0 to i64
  %arrayidx58 = getelementptr inbounds i32, i32* %0, i64 %idxprom57
  store i32 2, i32* %arrayidx58, align 4
  %idxprom59 = sext i32 %i18.0 to i64
  %arrayidx60 = getelementptr inbounds i32, i32* %0, i64 %idxprom59
  store i32 3, i32* %arrayidx60, align 4
  %idxprom61 = sext i32 %i18.0 to i64
  %arrayidx62 = getelementptr inbounds i32, i32* %0, i64 %idxprom61
  store i32 4, i32* %arrayidx62, align 4
  %idxprom63 = sext i32 %i18.0 to i64
  %arrayidx64 = getelementptr inbounds i32, i32* %0, i64 %idxprom63
  store i32 5, i32* %arrayidx64, align 4
  %idxprom65 = sext i32 %i18.0 to i64
  %arrayidx66 = getelementptr inbounds i32, i32* %0, i64 %idxprom65
  store i32 1, i32* %arrayidx66, align 4
  %idxprom67 = sext i32 %i18.0 to i64
  %arrayidx68 = getelementptr inbounds i32, i32* %0, i64 %idxprom67
  store i32 2, i32* %arrayidx68, align 4
  %idxprom69 = sext i32 %i18.0 to i64
  %arrayidx70 = getelementptr inbounds i32, i32* %0, i64 %idxprom69
  store i32 3, i32* %arrayidx70, align 4
  %idxprom71 = sext i32 %i18.0 to i64
  %arrayidx72 = getelementptr inbounds i32, i32* %0, i64 %idxprom71
  store i32 4, i32* %arrayidx72, align 4
  %idxprom73 = sext i32 %i18.0 to i64
  %arrayidx74 = getelementptr inbounds i32, i32* %0, i64 %idxprom73
  store i32 5, i32* %arrayidx74, align 4
  %idxprom75 = sext i32 %i18.0 to i64
  %arrayidx76 = getelementptr inbounds i32, i32* %0, i64 %idxprom75
  store i32 1, i32* %arrayidx76, align 4
  %idxprom77 = sext i32 %i18.0 to i64
  %arrayidx78 = getelementptr inbounds i32, i32* %0, i64 %idxprom77
  store i32 2, i32* %arrayidx78, align 4
  %idxprom79 = sext i32 %i18.0 to i64
  %arrayidx80 = getelementptr inbounds i32, i32* %0, i64 %idxprom79
  store i32 3, i32* %arrayidx80, align 4
  %idxprom81 = sext i32 %i18.0 to i64
  %arrayidx82 = getelementptr inbounds i32, i32* %0, i64 %idxprom81
  store i32 4, i32* %arrayidx82, align 4
  %idxprom83 = sext i32 %i18.0 to i64
  %arrayidx84 = getelementptr inbounds i32, i32* %0, i64 %idxprom83
  store i32 5, i32* %arrayidx84, align 4
  %idxprom85 = sext i32 %i18.0 to i64
  %arrayidx86 = getelementptr inbounds i32, i32* %0, i64 %idxprom85
  store i32 1, i32* %arrayidx86, align 4
  %idxprom87 = sext i32 %i18.0 to i64
  %arrayidx88 = getelementptr inbounds i32, i32* %0, i64 %idxprom87
  store i32 2, i32* %arrayidx88, align 4
  %idxprom89 = sext i32 %i18.0 to i64
  %arrayidx90 = getelementptr inbounds i32, i32* %0, i64 %idxprom89
  store i32 3, i32* %arrayidx90, align 4
  %idxprom91 = sext i32 %i18.0 to i64
  %arrayidx92 = getelementptr inbounds i32, i32* %0, i64 %idxprom91
  store i32 4, i32* %arrayidx92, align 4
  %idxprom93 = sext i32 %i18.0 to i64
  %arrayidx94 = getelementptr inbounds i32, i32* %0, i64 %idxprom93
  store i32 5, i32* %arrayidx94, align 4
  %idxprom95 = sext i32 %i18.0 to i64
  %arrayidx96 = getelementptr inbounds i32, i32* %0, i64 %idxprom95
  store i32 1, i32* %arrayidx96, align 4
  %idxprom97 = sext i32 %i18.0 to i64
  %arrayidx98 = getelementptr inbounds i32, i32* %0, i64 %idxprom97
  store i32 2, i32* %arrayidx98, align 4
  %idxprom99 = sext i32 %i18.0 to i64
  %arrayidx100 = getelementptr inbounds i32, i32* %0, i64 %idxprom99
  store i32 3, i32* %arrayidx100, align 4
  %idxprom101 = sext i32 %i18.0 to i64
  %arrayidx102 = getelementptr inbounds i32, i32* %0, i64 %idxprom101
  store i32 4, i32* %arrayidx102, align 4
  %idxprom103 = sext i32 %i18.0 to i64
  %arrayidx104 = getelementptr inbounds i32, i32* %0, i64 %idxprom103
  store i32 5, i32* %arrayidx104, align 4
  %idxprom105 = sext i32 %i18.0 to i64
  %arrayidx106 = getelementptr inbounds i32, i32* %0, i64 %idxprom105
  store i32 1, i32* %arrayidx106, align 4
  %idxprom107 = sext i32 %i18.0 to i64
  %arrayidx108 = getelementptr inbounds i32, i32* %0, i64 %idxprom107
  store i32 2, i32* %arrayidx108, align 4
  %idxprom109 = sext i32 %i18.0 to i64
  %arrayidx110 = getelementptr inbounds i32, i32* %0, i64 %idxprom109
  store i32 3, i32* %arrayidx110, align 4
  %idxprom111 = sext i32 %i18.0 to i64
  %arrayidx112 = getelementptr inbounds i32, i32* %0, i64 %idxprom111
  store i32 4, i32* %arrayidx112, align 4
  %idxprom113 = sext i32 %i18.0 to i64
  %arrayidx114 = getelementptr inbounds i32, i32* %0, i64 %idxprom113
  store i32 5, i32* %arrayidx114, align 4
  br label %for.inc115

for.inc115:                                       ; preds = %for.body24
  %inc116 = add nsw i32 %i18.0, 1
  br label %for.cond19, !llvm.loop !9

for.end117:                                       ; preds = %for.cond.cleanup23
  br label %for.cond119

for.cond119:                                      ; preds = %for.inc125, %for.end117
  %i118.0 = phi i32 [ 0, %for.end117 ], [ %inc126, %for.inc125 ]
  %conv120 = sext i32 %i118.0 to i64
  %cmp121 = icmp ult i64 %conv120, %call
  br i1 %cmp121, label %for.body124, label %for.cond.cleanup123

for.cond.cleanup123:                              ; preds = %for.cond119
  br label %for.end127

for.body124:                                      ; preds = %for.cond119
  br label %for.inc125

for.inc125:                                       ; preds = %for.body124
  %inc126 = add nsw i32 %i118.0, 1
  br label %for.cond119, !llvm.loop !10

for.end127:                                       ; preds = %for.cond.cleanup123
  ret i32 0
}

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #2

declare i64 @read(...) #3

; Function Attrs: argmemonly nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #2

declare void @write(i64 noundef) #3

attributes #0 = { nounwind uwtable "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind allocsize(0) "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { argmemonly nocallback nofree nosync nounwind willreturn }
attributes #3 = { "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind allocsize(0) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
!5 = distinct !{!5, !6, !7}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.unroll.disable"}
!8 = distinct !{!8, !6, !7}
!9 = distinct !{!9, !6, !7}
!10 = distinct !{!10, !6, !7}
