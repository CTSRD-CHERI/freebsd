; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=thumb-eabi -mcpu=cortex-m0 %s -verify-machineinstrs -o - | FileCheck %s

define i32 @test1a(i32 %a, i32 %b) {
; CHECK-LABEL: test1a:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    bne .LBB0_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB0_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp ne i32 %a, %b
  %cond = zext i1 %cmp to i32
  ret i32 %cond
}

define i32 @test1b(i32 %a, i32 %b) {
; CHECK-LABEL: test1b:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    beq .LBB1_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB1_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp eq i32 %a, %b
  %cond = zext i1 %cmp to i32
  ret i32 %cond
}

define i32 @test2a(i32 %a, i32 %b) {
; CHECK-LABEL: test2a:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    beq .LBB2_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB2_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp eq i32 %a, %b
  %cond = zext i1 %cmp to i32
  ret i32 %cond
}

define i32 @test2b(i32 %a, i32 %b) {
; CHECK-LABEL: test2b:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    bne .LBB3_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB3_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp ne i32 %a, %b
  %cond = zext i1 %cmp to i32
  ret i32 %cond
}

define i32 @test3a(i32 %a, i32 %b) {
; CHECK-LABEL: test3a:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #0
; CHECK-NEXT:    movs r3, #4
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    beq .LBB4_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB4_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp eq i32 %a, %b
  %cond = select i1 %cmp, i32 0, i32 4
  ret i32 %cond
}

define i32 @test3b(i32 %a, i32 %b) {
; CHECK-LABEL: test3b:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    movs r2, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r0, r1
; CHECK-NEXT:    beq .LBB5_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r2, r3
; CHECK-NEXT:  .LBB5_2: @ %entry
; CHECK-NEXT:    lsls r0, r2, #2
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp eq i32 %a, %b
  %cond = select i1 %cmp, i32 4, i32 0
  ret i32 %cond
}

; FIXME: This one hasn't changed actually
; but could look like test3b
define i32 @test4a(i32 %a, i32 %b) {
; CHECK-LABEL: test4a:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    mov r2, r0
; CHECK-NEXT:    movs r0, #0
; CHECK-NEXT:    movs r3, #4
; CHECK-NEXT:    cmp r2, r1
; CHECK-NEXT:    bne .LBB6_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r0, r3
; CHECK-NEXT:  .LBB6_2: @ %entry
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp ne i32 %a, %b
  %cond = select i1 %cmp, i32 0, i32 4
  ret i32 %cond
}

define i32 @test4b(i32 %a, i32 %b) {
; CHECK-LABEL: test4b:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    movs r2, #1
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    cmp r0, r1
; CHECK-NEXT:    bne .LBB7_2
; CHECK-NEXT:  @ %bb.1: @ %entry
; CHECK-NEXT:    mov r2, r3
; CHECK-NEXT:  .LBB7_2: @ %entry
; CHECK-NEXT:    lsls r0, r2, #2
; CHECK-NEXT:    bx lr
entry:
  %cmp = icmp ne i32 %a, %b
  %cond = select i1 %cmp, i32 4, i32 0
  ret i32 %cond
}

