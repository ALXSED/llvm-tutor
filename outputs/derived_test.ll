; ModuleID = '../inputs/derived_test.ll'
source_filename = "../inputs/derived_test.ll"

define void @test(i32 %n, ptr %A) {
entry:
  br label %loop

loop:                                             ; preds = %loop, %entry
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %j = phi i32 [ 1, %entry ], [ %j.next, %loop ]
  %idx = getelementptr i32, ptr %A, i32 %j
  store i32 %j, ptr %idx, align 4
  %i.next = add i32 %i, 1
  %j.next = add i32 %i.next, 1
  %cmp = icmp slt i32 %i.next, %n
  br i1 %cmp, label %loop, label %exit

exit:                                             ; preds = %loop
  ret void
}
