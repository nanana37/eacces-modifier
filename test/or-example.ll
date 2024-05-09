; ModuleID = 'or-example.c'
source_filename = "or-example.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [15 x i8] c"Hello, World!\0A\00", align 1, !dbg !7

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !22 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  call void @llvm.dbg.declare(metadata ptr %x, metadata !27, metadata !DIExpression()), !dbg !28
  %call = call i32 (ptr, ...) @__isoc99_scanf(ptr noundef @.str, ptr noundef %x), !dbg !29
  %0 = load i32, ptr %x, align 4, !dbg !30
  %cmp = icmp eq i32 %0, 0, !dbg !32
  br i1 %cmp, label %if.then, label %lor.lhs.false, !dbg !33

lor.lhs.false:                                    ; preds = %entry
  %1 = load i32, ptr %x, align 4, !dbg !34
  %cmp1 = icmp eq i32 %1, 1, !dbg !35
  br i1 %cmp1, label %if.then, label %if.end4, !dbg !36

if.then:                                          ; preds = %lor.lhs.false, %entry
  %2 = load i32, ptr %x, align 4, !dbg !37
  %rem = srem i32 %2, 2, !dbg !39
  %cmp2 = icmp eq i32 %rem, 0, !dbg !40
  br i1 %cmp2, label %if.then3, label %if.end, !dbg !41

if.then3:                                         ; preds = %if.then
  store i32 -13, ptr %retval, align 4, !dbg !42
  br label %return, !dbg !42

if.end:                                           ; preds = %if.then
  br label %if.end4, !dbg !43

if.end4:                                          ; preds = %if.end, %lor.lhs.false
  %3 = load i32, ptr %x, align 4, !dbg !44
  switch i32 %3, label %sw.epilog [
    i32 2, label %sw.bb
    i32 4, label %sw.bb
    i32 3, label %sw.bb5
  ], !dbg !45

sw.bb:                                            ; preds = %if.end4, %if.end4
  store i32 -13, ptr %retval, align 4, !dbg !46
  br label %return, !dbg !46

sw.bb5:                                           ; preds = %if.end4
  %call6 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !48
  store i32 -13, ptr %retval, align 4, !dbg !49
  br label %return, !dbg !49

sw.epilog:                                        ; preds = %if.end4
  store i32 0, ptr %retval, align 4, !dbg !50
  br label %return, !dbg !50

return:                                           ; preds = %sw.epilog, %sw.bb5, %sw.bb, %if.then3
  %4 = load i32, ptr %retval, align 4, !dbg !51
  ret i32 %4, !dbg !51
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i32 @__isoc99_scanf(ptr noundef, ...) #2

declare i32 @printf(ptr noundef, ...) #2

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!12}
!llvm.module.flags = !{!14, !15, !16, !17, !18, !19, !20}
!llvm.ident = !{!21}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 6, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "or-example.c", directory: "/home/hiron/eacces-modifier/test", checksumkind: CSK_MD5, checksum: "5f4adab5b542a7423bad17f6b993241c")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 24, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 3)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 15, type: !9, isLocal: true, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 120, elements: !10)
!10 = !{!11}
!11 = !DISubrange(count: 15)
!12 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Homebrew clang version 17.0.6", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !13, splitDebugInlining: false, nameTableKind: None)
!13 = !{!0, !7}
!14 = !{i32 7, !"Dwarf Version", i32 5}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{i32 8, !"PIC Level", i32 2}
!18 = !{i32 7, !"PIE Level", i32 2}
!19 = !{i32 7, !"uwtable", i32 2}
!20 = !{i32 7, !"frame-pointer", i32 2}
!21 = !{!"Homebrew clang version 17.0.6"}
!22 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !23, scopeLine: 4, spFlags: DISPFlagDefinition, unit: !12, retainedNodes: !26)
!23 = !DISubroutineType(types: !24)
!24 = !{!25}
!25 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!26 = !{}
!27 = !DILocalVariable(name: "x", scope: !22, file: !2, line: 5, type: !25)
!28 = !DILocation(line: 5, column: 7, scope: !22)
!29 = !DILocation(line: 6, column: 3, scope: !22)
!30 = !DILocation(line: 7, column: 7, scope: !31)
!31 = distinct !DILexicalBlock(scope: !22, file: !2, line: 7, column: 7)
!32 = !DILocation(line: 7, column: 9, scope: !31)
!33 = !DILocation(line: 7, column: 14, scope: !31)
!34 = !DILocation(line: 7, column: 17, scope: !31)
!35 = !DILocation(line: 7, column: 19, scope: !31)
!36 = !DILocation(line: 7, column: 7, scope: !22)
!37 = !DILocation(line: 8, column: 9, scope: !38)
!38 = distinct !DILexicalBlock(scope: !31, file: !2, line: 8, column: 9)
!39 = !DILocation(line: 8, column: 11, scope: !38)
!40 = !DILocation(line: 8, column: 15, scope: !38)
!41 = !DILocation(line: 8, column: 9, scope: !31)
!42 = !DILocation(line: 9, column: 7, scope: !38)
!43 = !DILocation(line: 8, column: 18, scope: !38)
!44 = !DILocation(line: 10, column: 11, scope: !22)
!45 = !DILocation(line: 10, column: 3, scope: !22)
!46 = !DILocation(line: 13, column: 5, scope: !47)
!47 = distinct !DILexicalBlock(scope: !22, file: !2, line: 10, column: 14)
!48 = !DILocation(line: 15, column: 5, scope: !47)
!49 = !DILocation(line: 16, column: 5, scope: !47)
!50 = !DILocation(line: 18, column: 3, scope: !22)
!51 = !DILocation(line: 19, column: 1, scope: !22)
