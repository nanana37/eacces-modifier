
Function: may_open
; Function Attrs: fn_ret_thunk_extern noredzone nounwind null_pointer_is_valid sspstrong
define internal i32 @may_open(ptr noundef %idmap, ptr noundef %path, i32 noundef %acc_mode, i32 noundef %flag) #0 align 16 !dbg !22527 {
entry:
  %retval = alloca i32, align 4
  %idmap.addr = alloca ptr, align 8, !DIAssignID !22538
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22531, metadata !DIExpression(), metadata !22538, metadata ptr %idmap.addr, metadata !DIExpression()), !dbg !22539
  %path.addr = alloca ptr, align 8, !DIAssignID !22540
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22532, metadata !DIExpression(), metadata !22540, metadata ptr %path.addr, metadata !DIExpression()), !dbg !22539
  %acc_mode.addr = alloca i32, align 4, !DIAssignID !22541
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22533, metadata !DIExpression(), metadata !22541, metadata ptr %acc_mode.addr, metadata !DIExpression()), !dbg !22539
  %flag.addr = alloca i32, align 4, !DIAssignID !22542
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22534, metadata !DIExpression(), metadata !22542, metadata ptr %flag.addr, metadata !DIExpression()), !dbg !22539
  %dentry = alloca ptr, align 8, !DIAssignID !22543
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22535, metadata !DIExpression(), metadata !22543, metadata ptr %dentry, metadata !DIExpression()), !dbg !22539
  %inode = alloca ptr, align 8, !DIAssignID !22544
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22536, metadata !DIExpression(), metadata !22544, metadata ptr %inode, metadata !DIExpression()), !dbg !22539
  %error = alloca i32, align 4, !DIAssignID !22545
  call void @llvm.dbg.assign(metadata i1 undef, metadata !22537, metadata !DIExpression(), metadata !22545, metadata ptr %error, metadata !DIExpression()), !dbg !22539
  %cleanup.dest.slot = alloca i32, align 4
  store ptr %idmap, ptr %idmap.addr, align 8, !DIAssignID !22546
  call void @llvm.dbg.assign(metadata ptr %idmap, metadata !22531, metadata !DIExpression(), metadata !22546, metadata ptr %idmap.addr, metadata !DIExpression()), !dbg !22539
  store ptr %path, ptr %path.addr, align 8, !DIAssignID !22547
  call void @llvm.dbg.assign(metadata ptr %path, metadata !22532, metadata !DIExpression(), metadata !22547, metadata ptr %path.addr, metadata !DIExpression()), !dbg !22539
  store i32 %acc_mode, ptr %acc_mode.addr, align 4, !DIAssignID !22548
  call void @llvm.dbg.assign(metadata i32 %acc_mode, metadata !22533, metadata !DIExpression(), metadata !22548, metadata ptr %acc_mode.addr, metadata !DIExpression()), !dbg !22539
  store i32 %flag, ptr %flag.addr, align 4, !DIAssignID !22549
  call void @llvm.dbg.assign(metadata i32 %flag, metadata !22534, metadata !DIExpression(), metadata !22549, metadata ptr %flag.addr, metadata !DIExpression()), !dbg !22539
  call void @llvm.lifetime.start.p0(i64 8, ptr %dentry) #28, !dbg !22550
  %0 = load ptr, ptr %path.addr, align 8, !dbg !22551
  %dentry1 = getelementptr inbounds %struct.path, ptr %0, i32 0, i32 1, !dbg !22552
  %1 = load ptr, ptr %dentry1, align 8, !dbg !22552
  store ptr %1, ptr %dentry, align 8, !dbg !22553, !DIAssignID !22554
  call void @llvm.dbg.assign(metadata ptr %1, metadata !22535, metadata !DIExpression(), metadata !22554, metadata ptr %dentry, metadata !DIExpression()), !dbg !22539
  call void @llvm.lifetime.start.p0(i64 8, ptr %inode) #28, !dbg !22555
  %2 = load ptr, ptr %dentry, align 8, !dbg !22556
  %d_inode = getelementptr inbounds %struct.dentry, ptr %2, i32 0, i32 5, !dbg !22557
  %3 = load ptr, ptr %d_inode, align 8, !dbg !22557
  store ptr %3, ptr %inode, align 8, !dbg !22558, !DIAssignID !22559
  call void @llvm.dbg.assign(metadata ptr %3, metadata !22536, metadata !DIExpression(), metadata !22559, metadata ptr %inode, metadata !DIExpression()), !dbg !22539
  call void @llvm.lifetime.start.p0(i64 4, ptr %error) #28, !dbg !22560
  %4 = load ptr, ptr %inode, align 8, !dbg !22561
  %tobool = icmp ne ptr %4, null, !dbg !22561
  br i1 %tobool, label %if.end, label %if.then, !dbg !22563

if.then:                                          ; preds = %entry
  store i32 -2, ptr %retval, align 4, !dbg !22564
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22564

if.end:                                           ; preds = %entry
  %5 = load ptr, ptr %inode, align 8, !dbg !22565
  %i_mode = getelementptr inbounds %struct.inode, ptr %5, i32 0, i32 0, !dbg !22566
  %6 = load i16, ptr %i_mode, align 8, !dbg !22566
  %conv = zext i16 %6 to i32, !dbg !22565
  %and = and i32 %conv, 61440, !dbg !22567
  switch i32 %and, label %sw.epilog [
    i32 40960, label %sw.bb
    i32 16384, label %sw.bb2
    i32 24576, label %sw.bb11
    i32 8192, label %sw.bb11
    i32 4096, label %sw.bb14
    i32 49152, label %sw.bb14
    i32 32768, label %sw.bb20
  ], !dbg !22568

sw.bb:                                            ; preds = %if.end
  store i32 -40, ptr %retval, align 4, !dbg !22569
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22569

sw.bb2:                                           ; preds = %if.end
  %7 = load i32, ptr %acc_mode.addr, align 4, !dbg !22571
  %and3 = and i32 %7, 2, !dbg !22573
  %tobool4 = icmp ne i32 %and3, 0, !dbg !22573
  br i1 %tobool4, label %if.then5, label %if.end6, !dbg !22574

if.then5:                                         ; preds = %sw.bb2
  store i32 -21, ptr %retval, align 4, !dbg !22575
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22575

if.end6:                                          ; preds = %sw.bb2
  %8 = load i32, ptr %acc_mode.addr, align 4, !dbg !22576
  %and7 = and i32 %8, 1, !dbg !22578
  %tobool8 = icmp ne i32 %and7, 0, !dbg !22578
  br i1 %tobool8, label %if.then9, label %if.end10, !dbg !22579

if.then9:                                         ; preds = %if.end6
  store i32 -13, ptr %retval, align 4, !dbg !22580
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22580

if.end10:                                         ; preds = %if.end6
  br label %sw.epilog, !dbg !22581

sw.bb11:                                          ; preds = %if.end, %if.end
  %9 = load ptr, ptr %path.addr, align 8, !dbg !22582
  %call = call zeroext i1 @may_open_dev(ptr noundef %9) #29, !dbg !22584
  br i1 %call, label %if.end13, label %if.then12, !dbg !22585

if.then12:                                        ; preds = %sw.bb11
  store i32 -13, ptr %retval, align 4, !dbg !22586
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22586

if.end13:                                         ; preds = %sw.bb11
  br label %sw.bb14, !dbg !22587

sw.bb14:                                          ; preds = %if.end, %if.end, %if.end13
  %10 = load i32, ptr %acc_mode.addr, align 4, !dbg !22588
  %and15 = and i32 %10, 1, !dbg !22590
  %tobool16 = icmp ne i32 %and15, 0, !dbg !22590
  br i1 %tobool16, label %if.then17, label %if.end18, !dbg !22591

if.then17:                                        ; preds = %sw.bb14
  store i32 -13, ptr %retval, align 4, !dbg !22592
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22592

if.end18:                                         ; preds = %sw.bb14
  %11 = load i32, ptr %flag.addr, align 4, !dbg !22593
  %and19 = and i32 %11, -513, !dbg !22593
  store i32 %and19, ptr %flag.addr, align 4, !dbg !22593, !DIAssignID !22594
  call void @llvm.dbg.assign(metadata i32 %and19, metadata !22534, metadata !DIExpression(), metadata !22594, metadata ptr %flag.addr, metadata !DIExpression()), !dbg !22539
  br label %sw.epilog, !dbg !22595

sw.bb20:                                          ; preds = %if.end
  %12 = load i32, ptr %acc_mode.addr, align 4, !dbg !22596
  %and21 = and i32 %12, 1, !dbg !22598
  %tobool22 = icmp ne i32 %and21, 0, !dbg !22598
  br i1 %tobool22, label %land.lhs.true, label %if.end26, !dbg !22599

land.lhs.true:                                    ; preds = %sw.bb20
  %13 = load ptr, ptr %path.addr, align 8, !dbg !22600
  %call23 = call zeroext i1 @path_noexec(ptr noundef %13) #29, !dbg !22601
  br i1 %call23, label %if.then25, label %if.end26, !dbg !22602

if.then25:                                        ; preds = %land.lhs.true
  store i32 -13, ptr %retval, align 4, !dbg !22603
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22603

if.end26:                                         ; preds = %land.lhs.true, %sw.bb20
  br label %sw.epilog, !dbg !22604

sw.epilog:                                        ; preds = %if.end, %if.end26, %if.end18, %if.end10
  %14 = load ptr, ptr %idmap.addr, align 8, !dbg !22605
  %15 = load ptr, ptr %inode, align 8, !dbg !22606
  %16 = load i32, ptr %acc_mode.addr, align 4, !dbg !22607
  %or = or i32 32, %16, !dbg !22608
  %call27 = call i32 @inode_permission(ptr noundef %14, ptr noundef %15, i32 noundef %or) #29, !dbg !22609
  store i32 %call27, ptr %error, align 4, !dbg !22610, !DIAssignID !22611
  call void @llvm.dbg.assign(metadata i32 %call27, metadata !22537, metadata !DIExpression(), metadata !22611, metadata ptr %error, metadata !DIExpression()), !dbg !22539
  %17 = load i32, ptr %error, align 4, !dbg !22612
  %tobool28 = icmp ne i32 %17, 0, !dbg !22612
  br i1 %tobool28, label %if.then29, label %if.end30, !dbg !22614

if.then29:                                        ; preds = %sw.epilog
  %18 = load i32, ptr %error, align 4, !dbg !22615
  store i32 %18, ptr %retval, align 4, !dbg !22616
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22616

if.end30:                                         ; preds = %sw.epilog
  %19 = load ptr, ptr %inode, align 8, !dbg !22617
  %i_flags = getelementptr inbounds %struct.inode, ptr %19, i32 0, i32 4, !dbg !22617
  %20 = load i32, ptr %i_flags, align 4, !dbg !22617
  br i1 true, label %cont, label %handler.shift_out_of_bounds, !dbg !22617, !prof !9104, !nosanitize !1418

handler.shift_out_of_bounds:                      ; preds = %if.end30
  call void @__ubsan_handle_shift_out_of_bounds(ptr @135, i64 1, i64 2) #28, !dbg !22617, !nosanitize !1418
  br label %cont, !dbg !22617, !nosanitize !1418

cont:                                             ; preds = %handler.shift_out_of_bounds, %if.end30
  %and31 = and i32 %20, 4, !dbg !22617
  %tobool32 = icmp ne i32 %and31, 0, !dbg !22617
  br i1 %tobool32, label %if.then33, label %if.end45, !dbg !22619

if.then33:                                        ; preds = %cont
  %21 = load i32, ptr %flag.addr, align 4, !dbg !22620
  %and34 = and i32 %21, 3, !dbg !22623
  %cmp = icmp ne i32 %and34, 0, !dbg !22624
  br i1 %cmp, label %land.lhs.true36, label %if.end40, !dbg !22625

land.lhs.true36:                                  ; preds = %if.then33
  %22 = load i32, ptr %flag.addr, align 4, !dbg !22626
  %and37 = and i32 %22, 1024, !dbg !22627
  %tobool38 = icmp ne i32 %and37, 0, !dbg !22627
  br i1 %tobool38, label %if.end40, label %if.then39, !dbg !22628

if.then39:                                        ; preds = %land.lhs.true36
  store i32 -1, ptr %retval, align 4, !dbg !22629
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22629

if.end40:                                         ; preds = %land.lhs.true36, %if.then33
  %23 = load i32, ptr %flag.addr, align 4, !dbg !22630
  %and41 = and i32 %23, 512, !dbg !22632
  %tobool42 = icmp ne i32 %and41, 0, !dbg !22632
  br i1 %tobool42, label %if.then43, label %if.end44, !dbg !22633

if.then43:                                        ; preds = %if.end40
  store i32 -1, ptr %retval, align 4, !dbg !22634
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22634

if.end44:                                         ; preds = %if.end40
  br label %if.end45, !dbg !22635

if.end45:                                         ; preds = %if.end44, %cont
  %24 = load i32, ptr %flag.addr, align 4, !dbg !22636
  %and46 = and i32 %24, 262144, !dbg !22638
  %tobool47 = icmp ne i32 %and46, 0, !dbg !22638
  br i1 %tobool47, label %land.lhs.true48, label %if.end51, !dbg !22639

land.lhs.true48:                                  ; preds = %if.end45
  %25 = load ptr, ptr %idmap.addr, align 8, !dbg !22640
  %26 = load ptr, ptr %inode, align 8, !dbg !22641
  %call49 = call zeroext i1 @inode_owner_or_capable(ptr noundef %25, ptr noundef %26) #29, !dbg !22642
  br i1 %call49, label %if.end51, label %if.then50, !dbg !22643

if.then50:                                        ; preds = %land.lhs.true48
  store i32 -1, ptr %retval, align 4, !dbg !22644
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22644

if.end51:                                         ; preds = %land.lhs.true48, %if.end45
  store i32 0, ptr %retval, align 4, !dbg !22645
  store i32 1, ptr %cleanup.dest.slot, align 4
  br label %cleanup, !dbg !22645

cleanup:                                          ; preds = %if.end51, %if.then50, %if.then43, %if.then39, %if.then29, %if.then25, %if.then17, %if.then12, %if.then9, %if.then5, %sw.bb, %if.then
  call void @llvm.lifetime.end.p0(i64 4, ptr %error) #28, !dbg !22646
  call void @llvm.lifetime.end.p0(i64 8, ptr %inode) #28, !dbg !22646
  call void @llvm.lifetime.end.p0(i64 8, ptr %dentry) #28, !dbg !22646
  %27 = load i32, ptr %retval, align 4, !dbg !22646
  ret i32 %27, !dbg !22646
}

