// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

.file "asm.S"
.intel_syntax noprefix

.text
.global signal_interrupter_asm
.type signal_interrupter_asm, @function
.global signal_interrupter_asm_zmm
.type signal_interrupter_asm_zmm, @function
.global signal_interrupter_asm_ymm
.type signal_interrupter_asm_ymm, @function
.global signal_interrupter_asm_xmm
.type signal_interrupter_asm_xmm, @function
.global signal_interrupter_asm_xmm_small
.type signal_interrupter_asm_xmm_small, @function

signal_interrupter_asm:
    //push return address
    //push interrupter
    push rbp
    pushfq

    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
	mov rbp,rsp





    //get interrupter
    mov rax, [rbp + 120]
    and rsp, 0fffffffffffffff0h
    //make buffer zone for c++
	sub rsp,20h
    //call void interrupter()
    call rax
    //restore stack
    mov rsp,rbp
    //restore everything (flags + registers)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq
    pop rbp
    //pop interrupter
    lea rsp, [rsp+8]
    ret
.size   signal_interrupter_asm, .-signal_interrupter_asm

signal_interrupter_asm_zmm:
    //push return address
    //push interrupter
    push rbp

    //save flags
    pushfq

    //save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
	mov rbp,rsp
    sub rsp, 800h
    and rsp, 0ffffffffffffffC0h
    vmovups [rsp], zmm0
    vmovups [rsp+40h], zmm1
    vmovups [rsp+80h], zmm2
    vmovups [rsp+0c0h], zmm3
    vmovups [rsp+100h], zmm4
    vmovups [rsp+140h], zmm5
    vmovups [rsp+180h], zmm6
    vmovups [rsp+1c0h], zmm7
    vmovups [rsp+200h], zmm8
    vmovups [rsp+240h], zmm9
    vmovups [rsp+280h], zmm10
    vmovups [rsp+2c0h], zmm11
    vmovups [rsp+300h], zmm12
    vmovups [rsp+340h], zmm13
    vmovups [rsp+380h], zmm14
    vmovups [rsp+3c0h], zmm15
    vmovups [rsp+400h], zmm16
    vmovups [rsp+440h], zmm17
    vmovups [rsp+480h], zmm18
    vmovups [rsp+4c0h], zmm19
    vmovups [rsp+500h], zmm20
    vmovups [rsp+540h], zmm21
    vmovups [rsp+580h], zmm22
    vmovups [rsp+5c0h], zmm23
    vmovups [rsp+600h], zmm24
    vmovups [rsp+640h], zmm25
    vmovups [rsp+680h], zmm26
    vmovups [rsp+6c0h], zmm27
    vmovups [rsp+700h], zmm28
    vmovups [rsp+740h], zmm29
    vmovups [rsp+780h], zmm30
    vmovups [rsp+7c0h], zmm31



    //get interrupter
    mov rax, [rbp + 120]
    and rsp, 0fffffffffffffff0h
    //make buffer zone for c++
	sub rsp,20h
    //call void interrupter()
    call rax
    //restore stack
    mov rsp,rbp
    //restore everything (flags + registers)
    and rsp, 0ffffffffffffffC0h

    vmovups zmm0, [rsp]
    vmovups zmm1, [rsp+40h]
    vmovups zmm2, [rsp+80h]
    vmovups zmm3, [rsp+0c0h]
    vmovups zmm4, [rsp+100h]
    vmovups zmm5, [rsp+140h]
    vmovups zmm6, [rsp+180h]
    vmovups zmm7, [rsp+1c0h]
    vmovups zmm8, [rsp+200h]
    vmovups zmm9, [rsp+240h]
    vmovups zmm10, [rsp+280h]
    vmovups zmm11, [rsp+2c0h]
    vmovups zmm12, [rsp+300h]
    vmovups zmm13, [rsp+340h]
    vmovups zmm14, [rsp+380h]
    vmovups zmm15, [rsp+3c0h]
    vmovups zmm16, [rsp+400h]
    vmovups zmm17, [rsp+440h]
    vmovups zmm18, [rsp+480h]
    vmovups zmm19, [rsp+4c0h]
    vmovups zmm20, [rsp+500h]
    vmovups zmm21, [rsp+540h]
    vmovups zmm22, [rsp+580h]
    vmovups zmm23, [rsp+5c0h]
    vmovups zmm24, [rsp+600h]
    vmovups zmm25, [rsp+640h]
    vmovups zmm26, [rsp+680h]
    vmovups zmm27, [rsp+6c0h]
    vmovups zmm28, [rsp+700h]
    vmovups zmm29, [rsp+740h]
    vmovups zmm30, [rsp+780h]
    vmovups zmm31, [rsp+7c0h]

    mov rsp,rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq
    pop rbp
    //pop interrupter
    lea rsp, [rsp+8]
    ret

.size   signal_interrupter_asm_zmm, .-signal_interrupter_asm_zmm


signal_interrupter_asm_ymm:
    //push return address
    //push interrupter
    push rbp

    //save flags
    pushfq

    //save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
	mov rbp,rsp

    sub rsp, 400h
    and rsp, 0ffffffffffffffe0h
    vmovups [rsp], ymm0
    vmovups [rsp+40h], ymm1
    vmovups [rsp+80h], ymm2
    vmovups [rsp+0c0h], ymm3
    vmovups [rsp+100h], ymm4
    vmovups [rsp+140h], ymm5
    vmovups [rsp+180h], ymm6
    vmovups [rsp+1c0h], ymm7
    vmovups [rsp+200h], ymm8
    vmovups [rsp+240h], ymm9
    vmovups [rsp+280h], ymm10
    vmovups [rsp+2c0h], ymm11
    vmovups [rsp+300h], ymm12
    vmovups [rsp+340h], ymm13
    vmovups [rsp+380h], ymm14
    vmovups [rsp+3c0h], ymm15

    





    //get interrupter
    mov rax, [rbp + 120]
    and rsp, 0fffffffffffffff0h
    //make buffer zone for c++
	sub rsp,20h
    //call void interrupter()
    call rax
    //restore stack
    mov rsp,rbp
    //restore everything (flags + registers)
    and rsp, 0ffffffffffffffe0h

    vmovups ymm0, [rsp]
    vmovups ymm1, [rsp+40h]
    vmovups ymm2, [rsp+80h]
    vmovups ymm3, [rsp+0c0h]
    vmovups ymm4, [rsp+100h]
    vmovups ymm5, [rsp+140h]
    vmovups ymm6, [rsp+180h]
    vmovups ymm7, [rsp+1c0h]
    vmovups ymm8, [rsp+200h]
    vmovups ymm9, [rsp+240h]
    vmovups ymm10, [rsp+280h]
    vmovups ymm11, [rsp+2c0h]
    vmovups ymm12, [rsp+300h]
    vmovups ymm13, [rsp+340h]
    vmovups ymm14, [rsp+380h]
    vmovups ymm15, [rsp+3c0h]
    mov rsp,rbp
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq
    pop rbp
    //pop interrupter
    lea rsp, [rsp+8]
    ret
.size   signal_interrupter_asm_ymm, .-signal_interrupter_asm_ymm
signal_interrupter_asm_xmm:
    //push return address//136
    //push interrupter//128
    push rbp    //120

    //save flags
    pushfq       //112

    //save registers
    push rax    //104
    push rbx    //96
    push rcx    //88
    push rdx    //80
    push rdi    //72
    push r8     //64
    push r9     //56
    push r10    //48
    push r11    //40
    push r12    //32
    push r13    //24
    push r14    //16
    push r15    //8
	mov rbp,rsp //0

    sub rsp, 100h
    and rsp, 0fffffffffffffff0h
    vmovups [rsp], xmm0
    vmovups [rsp+10h], xmm1
    vmovups [rsp+20h], xmm2
    vmovups [rsp+30h], xmm3
    vmovups [rsp+40h], xmm4
    vmovups [rsp+50h], xmm5
    vmovups [rsp+60h], xmm6
    vmovups [rsp+70h], xmm7
    vmovups [rsp+80h], xmm8
    vmovups [rsp+90h], xmm9
    vmovups [rsp+0a0h], xmm10
    vmovups [rsp+0b0h], xmm11
    vmovups [rsp+0c0h], xmm12
    vmovups [rsp+0d0h], xmm13
    vmovups [rsp+0e0h], xmm14
    vmovups [rsp+0f0h], xmm15

    





    //get interrupter
    mov rax, [rbp + 120]
    and rsp, 0fffffffffffffff0h
    //make buffer zone for c++
	sub rsp,20h
    //call void interrupter()
    call rax
    //restore stack
    mov rsp,rbp
    //restore everything (flags + registers)
    and rsp, 0fffffffffffffff0h

    vmovups xmm0, [rsp]
    vmovups xmm1, [rsp+40h]
    vmovups xmm2, [rsp+80h]
    vmovups xmm3, [rsp+0c0h]
    vmovups xmm4, [rsp+100h]
    vmovups xmm5, [rsp+140h]
    vmovups xmm6, [rsp+180h]
    vmovups xmm7, [rsp+1c0h]
    vmovups xmm8, [rsp+200h]
    vmovups xmm9, [rsp+240h]
    vmovups xmm10, [rsp+280h]
    vmovups xmm11, [rsp+2c0h]
    vmovups xmm12, [rsp+300h]
    vmovups xmm13, [rsp+340h]
    vmovups xmm14, [rsp+380h]
    vmovups xmm15, [rsp+3c0h]
    mov rsp,rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq
    pop rbp
    //pop interrupter
    lea rsp, [rsp+8]
    ret
.size   signal_interrupter_asm_xmm, .-signal_interrupter_asm_xmm
signal_interrupter_asm_xmm_small:
    //push return address
    //push interrupter
    push rbp

    //save flags
    pushfq

    //save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
	mov rbp,rsp

    sub rsp, 80h
    and rsp, 0fffffffffffffff0h
    vmovups [rsp], xmm0
    vmovups [rsp+10h], xmm1
    vmovups [rsp+20h], xmm2
    vmovups [rsp+30h], xmm3
    vmovups [rsp+40h], xmm4
    vmovups [rsp+50h], xmm5
    vmovups [rsp+60h], xmm6
    vmovups [rsp+70h], xmm7


    





    //get interrupter
    mov rax, [rbp + 120]
    and rsp, 0fffffffffffffff0h
    //make buffer zone for c++
	sub rsp,20h
    //call void interrupter()
    call rax
    //restore stack
    mov rsp,rbp
    //restore everything (flags + registers)
    and rsp, 0fffffffffffffff0h

    vmovups xmm0, [rsp]
    vmovups xmm1, [rsp+40h]
    vmovups xmm2, [rsp+80h]
    vmovups xmm3, [rsp+0c0h]
    vmovups xmm4, [rsp+100h]
    vmovups xmm5, [rsp+140h]
    vmovups xmm6, [rsp+180h]
    vmovups xmm7, [rsp+1c0h]
    mov rsp,rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq
    pop rbp
    //pop interrupter
    lea rsp, [rsp+8]
    ret
.size   signal_interrupter_asm_xmm_small, .-signal_interrupter_asm_xmm_small
