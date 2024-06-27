; ModuleID = 'TEST/result.opt.ll'
source_filename = "LICM.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [25 x i8] c"%d,%d,%d,%d,%d,%d,%d,%d\0A\00", align 1

define dso_local void @foo(i32 noundef %0, i32 noundef %1) {
  %3 = add nsw i32 %0, 3
  %4 = add nsw i32 %0, 7
  br label %5

5:                                                ; preds = %17, %2
  %.05 = phi i32 [ 0, %2 ], [ %21, %17 ]
  %.04 = phi i32 [ 0, %2 ], [ %19, %17 ]
  %.03 = phi i32 [ 0, %2 ], [ %18, %17 ]
  %.01 = phi i32 [ 9, %2 ], [ %.1, %17 ]
  %.0 = phi i32 [ %1, %2 ], [ %6, %17 ]
  %6 = add nsw i32 %.0, 1
  %7 = icmp slt i32 %6, 5
  br i1 %7, label %8, label %11

8:                                                ; preds = %5
  %9 = add nsw i32 %.01, 2
  %10 = add nsw i32 %0, 3
  br label %17

11:                                               ; preds = %5
  %12 = sub nsw i32 %.01, 1
  %13 = add nsw i32 %0, 4
  %14 = icmp sge i32 %6, 10
  br i1 %14, label %15, label %16

15:                                               ; preds = %11
  %.lcssa4 = phi i32 [ %12, %11 ]
  %.lcssa3 = phi i32 [ %13, %11 ]
  %.05.lcssa = phi i32 [ %.05, %11 ]
  %.04.lcssa = phi i32 [ %.04, %11 ]
  %.03.lcssa = phi i32 [ %.03, %11 ]
  %.lcssa2 = phi i32 [ %6, %11 ]
  %.lcssa1 = phi i32 [ %3, %11 ]
  %.lcssa = phi i32 [ %4, %11 ]
  br label %22

16:                                               ; preds = %11
  br label %17

17:                                               ; preds = %16, %8
  %.02 = phi i32 [ %10, %8 ], [ %13, %16 ]
  %.1 = phi i32 [ %9, %8 ], [ %12, %16 ]
  %18 = add nsw i32 %3, 7
  %19 = add nsw i32 %.02, 2
  %20 = add nsw i32 %0, 7
  %21 = add nsw i32 %4, 5
  br label %5

22:                                               ; preds = %15
  %23 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %.lcssa4, i32 noundef %.lcssa3, i32 noundef %.03.lcssa, i32 noundef %.04.lcssa, i32 noundef %.lcssa, i32 noundef %.05.lcssa, i32 noundef %.lcssa1, i32 noundef %.lcssa2)
  ret void
}

declare i32 @printf(ptr noundef, ...)

define dso_local i32 @main() {
  call void @foo(i32 noundef 0, i32 noundef 4)
  call void @foo(i32 noundef 0, i32 noundef 12)
  ret i32 0
}

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 14.0.6"}
