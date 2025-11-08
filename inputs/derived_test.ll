define void @test(i32 %n, i32* %A) {
entry:
  br label %loop

loop:
  %i = phi i32 [0, %entry], [%i.next, %loop]
  %j = phi i32 [1, %entry], [%j.next, %loop]
  %idx = getelementptr i32, i32* %A, i32 %j
  store i32 %j, i32* %idx
  %i.next = add i32 %i, 1
  %j.next = add i32 %i.next, 1
  %cmp = icmp slt i32 %i.next, %n
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}
