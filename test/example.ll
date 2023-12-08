; ModuleID = 'test/example.c'
source_filename = "test/example.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"OK\00", align 1
@path = dso_local global [100 x i8] zeroinitializer, align 16
@.str.1 = private unnamed_addr constant [22 x i8] c"Enter mode and flag: \00", align 1
@.str.2 = private unnamed_addr constant [6 x i8] c"%d %d\00", align 1
@.str.3 = private unnamed_addr constant [19 x i8] c"Enter a pathname: \00", align 1
@.str.4 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.5 = private unnamed_addr constant [11 x i8] c"Error: %s\0A\00", align 1
@.str.6 = private unnamed_addr constant [13 x i8] c"Success: %d\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @isOK(ptr noundef %str) #0 {
entry:
  %str.addr = alloca ptr, align 8
  %ret = alloca i32, align 4
  store ptr %str, ptr %str.addr, align 8
  %0 = load ptr, ptr %str.addr, align 8
  %call = call i32 @strcmp(ptr noundef %0, ptr noundef @.str) #4
  store i32 %call, ptr %ret, align 4
  %1 = load ptr, ptr %str.addr, align 8
  %call1 = call i32 @strcmp(ptr noundef %1, ptr noundef @.str) #4
  %cmp = icmp eq i32 %call1, 0
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: nounwind willreturn memory(read)
declare i32 @strcmp(ptr noundef, ptr noundef) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @may_open(i32 noundef %mode, i32 noundef %flag) #0 {
entry:
  %retval = alloca i32, align 4
  %mode.addr = alloca i32, align 4
  %flag.addr = alloca i32, align 4
  store i32 %mode, ptr %mode.addr, align 4
  store i32 %flag, ptr %flag.addr, align 4
  %0 = load i32, ptr %mode.addr, align 4
  switch i32 %0, label %sw.default [
    i32 1, label %sw.bb
    i32 2, label %sw.bb1
    i32 3, label %sw.bb6
    i32 4, label %sw.bb6
    i32 5, label %sw.bb10
    i32 6, label %sw.bb10
    i32 7, label %sw.bb15
  ]

sw.bb:                                            ; preds = %entry
  store i32 -40, ptr %retval, align 4
  br label %return

sw.bb1:                                           ; preds = %entry
  %1 = load i32, ptr %flag.addr, align 4
  %and = and i32 %1, 1
  %tobool = icmp ne i32 %and, 0
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %sw.bb1
  store i32 -21, ptr %retval, align 4
  br label %return

if.end:                                           ; preds = %sw.bb1
  %2 = load i32, ptr %flag.addr, align 4
  %and2 = and i32 %2, 2
  %tobool3 = icmp ne i32 %and2, 0
  br i1 %tobool3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %if.end
  store i32 -13, ptr %retval, align 4
  br label %return

if.end5:                                          ; preds = %if.end
  br label %sw.epilog

sw.bb6:                                           ; preds = %entry, %entry
  %call = call i32 @isOK(ptr noundef @path)
  %tobool7 = icmp ne i32 %call, 0
  br i1 %tobool7, label %if.end9, label %if.then8

if.then8:                                         ; preds = %sw.bb6
  store i32 -13, ptr %retval, align 4
  br label %return

if.end9:                                          ; preds = %sw.bb6
  br label %sw.bb10

sw.bb10:                                          ; preds = %entry, %entry, %if.end9
  %3 = load i32, ptr %flag.addr, align 4
  %and11 = and i32 %3, 1
  %tobool12 = icmp ne i32 %and11, 0
  br i1 %tobool12, label %if.then13, label %if.end14

if.then13:                                        ; preds = %sw.bb10
  store i32 -13, ptr %retval, align 4
  br label %return

if.end14:                                         ; preds = %sw.bb10
  br label %sw.epilog

sw.bb15:                                          ; preds = %entry
  %4 = load i32, ptr %flag.addr, align 4
  %and16 = and i32 %4, 1
  %tobool17 = icmp ne i32 %and16, 0
  br i1 %tobool17, label %land.lhs.true, label %if.end21

land.lhs.true:                                    ; preds = %sw.bb15
  %call18 = call i32 @isOK(ptr noundef @path)
  %tobool19 = icmp ne i32 %call18, 0
  br i1 %tobool19, label %if.then20, label %if.end21

if.then20:                                        ; preds = %land.lhs.true
  store i32 -13, ptr %retval, align 4
  br label %return

if.end21:                                         ; preds = %land.lhs.true, %sw.bb15
  br label %sw.default

sw.default:                                       ; preds = %entry, %if.end21
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.default, %if.end14, %if.end5
  store i32 5, ptr %retval, align 4
  br label %return

return:                                           ; preds = %sw.epilog, %if.then20, %if.then13, %if.then8, %if.then4, %if.then, %sw.bb
  %5 = load i32, ptr %retval, align 4
  ret i32 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %argc, ptr noundef %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %mode = alloca i32, align 4
  %flag = alloca i32, align 4
  %ret = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  store ptr %argv, ptr %argv.addr, align 8
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  %call1 = call i32 (ptr, ...) @__isoc99_scanf(ptr noundef @.str.2, ptr noundef %mode, ptr noundef %flag)
  %call2 = call i32 (ptr, ...) @printf(ptr noundef @.str.3)
  %call3 = call i32 (ptr, ...) @__isoc99_scanf(ptr noundef @.str.4, ptr noundef @path)
  %0 = load i32, ptr %mode, align 4
  %1 = load i32, ptr %flag, align 4
  %call4 = call i32 @may_open(i32 noundef %0, i32 noundef %1)
  store i32 %call4, ptr %ret, align 4
  %2 = load i32, ptr %ret, align 4
  %cmp = icmp slt i32 %2, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %3 = load i32, ptr %ret, align 4
  %sub = sub nsw i32 0, %3
  %call5 = call ptr @strerror(i32 noundef %sub) #5
  %call6 = call i32 (ptr, ...) @printf(ptr noundef @.str.5, ptr noundef %call5)
  br label %if.end

if.else:                                          ; preds = %entry
  %4 = load i32, ptr %ret, align 4
  %call7 = call i32 (ptr, ...) @printf(ptr noundef @.str.6, i32 noundef %4)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #2

declare i32 @__isoc99_scanf(ptr noundef, ...) #2

; Function Attrs: nounwind
declare ptr @strerror(i32 noundef) #3

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind willreturn memory(read) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind willreturn memory(read) }
attributes #5 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Homebrew clang version 17.0.6"}
