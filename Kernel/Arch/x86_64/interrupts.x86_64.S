/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

.intel_syntax noprefix
.code64

.section .text

.macro pushaq
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    mov eax, es
    push rax
    mov eax, ds
    push rax
.endmacro

.macro popaq
    pop rax
    mov ds, eax
    pop rax
    mov es, eax
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
.endmacro

.altmacro
.macro interrupt_handler int_number
.align 16
interrupt_handler_\int_number:
.if \int_number != 8 && \int_number != 10 && \int_number != 11 && \int_number != 12 && \int_number != 13 && \int_number != 14 && \int_number != 17 && \int_number != 30
    push 0
.endif

    cmp qword ptr [rsp + 16], 0x2b
    jne 1f
    swapgs
1:
    push \int_number
    pushaq
    cld
    mov rax, 0x10
    mov ds, rax
    mov es, rax
    mov ss, rax
    mov rdi, rsp

    xor rbp, rbp
    call raiseInterrupt

    popaq
    add rsp, 0x10

    cmp qword ptr [rsp + 8], 0x2b
    jne 2f
    swapgs
2:
    iretq
.endmacro

.set i, 0
.rept 256
    interrupt_handler %i
    .set i, i+1
.endr

.section .data
.macro get_exception_addr num
    .quad interrupt_handler_\num
.endmacro

.global interrupt_handlers
interrupt_handlers:
.set i, 0
.rept 256
	get_exception_addr %i
    .set i, i+1
.endr
