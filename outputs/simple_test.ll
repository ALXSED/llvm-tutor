; ModuleID = '../inputs/simple_test.ll'
source_filename = "../inputs/simple_test.ll"

define void @simple(i32 %n, ptr %arr) {
entry:
  %x = add i32 5, 3
  br label %loop

loop:                                             ; preds = %loop, %entry
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %idx = getelementptr i32, ptr %arr, i32 %i
  store i32 %x, ptr %idx, align 4
  %next = add i32 %i, 1
  %cond = icmp slt i32 %next, %n
  br i1 %cond, label %loop, label %exit

exit:                                             ; preds = %loop
  ret void
}
