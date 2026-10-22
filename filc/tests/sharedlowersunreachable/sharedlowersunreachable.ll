; Phis, GEPs and a select feeding each other in an unreachable loop once made the derived-lower
; analysis in FilPizlonator iterate forever: dominance treats unreachable blocks as dominated by
; everything, so the lower roots never settled. Found by fuzzing; compiling is the test.
target datalayout = "e-m:e-ni:0-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target datalayout_after_filc = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@msg = private unnamed_addr constant [3 x i8] c"ok\00"

declare ptr @malloc(i64)
declare i32 @puts(ptr)

define ptr @f(i1 %c) noinline {
entry:
  %m = call ptr @malloc(i64 16)
  ret ptr %m

u0:
  %v0 = phi ptr [ %v7, %u0 ], [ %v5, %u1 ]
  %v1 = phi ptr [ %v4, %u0 ], [ %v8, %u1 ]
  %v4 = getelementptr i8, ptr %v1, i64 2
  %v5 = getelementptr i8, ptr %v1, i64 3
  br label %u0

u1:
  %v7 = getelementptr i8, ptr %v1, i64 0
  %v8 = select i1 %c, ptr %v0, ptr %v5
  br label %u0
}

define i32 @main() {
entry:
  %r = call ptr @f(i1 true)
  %ok = icmp ne ptr %r, null
  br i1 %ok, label %good, label %bad

good:
  %x = call i32 @puts(ptr @msg)
  ret i32 0

bad:
  ret i32 1
}
