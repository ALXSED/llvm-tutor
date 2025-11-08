define void @simple(i32 %n, i32* %arr) {
entry:
  br label %loop

loop:
  %i = phi i32 [0, %entry], [%next, %loop]
  %x = add i32 5, 3             ; <== loop-invariant candidate
  %idx = getelementptr i32, i32* %arr, i32 %i
  store i32 %x, i32* %idx
  %next = add i32 %i, 1
  %cond = icmp slt i32 %next, %n
  br i1 %cond, label %loop, label %exit

exit:
  ret void
}

