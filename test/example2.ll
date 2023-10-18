; ModuleID = 'test/example2.c'
source_filename = "test/example2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [13 x i8] c"flag is 123\0A\00", align 1
@.str.1 = private unnamed_addr constant [13 x i8] c"flag is 456\0A\00", align 1
@.str.2 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.3 = private unnamed_addr constant [19 x i8] c"Permission denied\0A\00", align 1
@.str.4 = private unnamed_addr constant [18 x i8] c"my_open suceeded\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @my_open(i32 noundef %flag) #0 {
entry:
  %retval = alloca i32, align 4
  %flag.addr = alloca i32, align 4
  store i32 %flag, ptr %flag.addr, align 4
  %0 = load i32, ptr %flag.addr, align 4
  switch i32 %0, label %sw.default [
    i32 123, label %sw.bb
    i32 456, label %sw.bb1
  ]

sw.bb:                                            ; preds = %entry
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str)
  store i32 -13, ptr %retval, align 4
  br label %return

sw.bb1:                                           ; preds = %entry
  %call2 = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  store i32 -13, ptr %retval, align 4
  br label %return

sw.default:                                       ; preds = %entry
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.default
  store i32 5, ptr %retval, align 4
  br label %return

return:                                           ; preds = %sw.epilog, %sw.bb1, %sw.bb
  %1 = load i32, ptr %retval, align 4
  ret i32 %1
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %argc, ptr noundef %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %num = alloca i32, align 4
  %ret = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  store ptr %argv, ptr %argv.addr, align 8
  %call = call i32 (ptr, ...) @__isoc99_scanf(ptr noundef @.str.2, ptr noundef %num)
  %0 = load i32, ptr %num, align 4
  %call1 = call i32 @my_open(i32 noundef %0)
  store i32 %call1, ptr %ret, align 4
  %1 = load i32, ptr %ret, align 4
  switch i32 %1, label %sw.default [
    i32 -13, label %sw.bb
  ]

sw.bb:                                            ; preds = %entry
  %call2 = call i32 (ptr, ...) @printf(ptr noundef @.str.3)
  br label %sw.epilog

sw.default:                                       ; preds = %entry
  %call3 = call i32 (ptr, ...) @printf(ptr noundef @.str.4)
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.default, %sw.bb
  ret i32 0
}

declare i32 @__isoc99_scanf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Homebrew clang version 17.0.1"}
