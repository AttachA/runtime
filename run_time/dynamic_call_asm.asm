ifdef rsp
.code
ArgumentsPrepareCallForFastcall PROC FRAME
	push rbp
	.pushreg rbp
	mov rbp,rsp
    .setframe rbp, 0h
	.endprolog

	mov rax ,qword ptr [rbp+38h];argc
	mov r10 ,qword ptr [rbp+40h];args
	shl rax,3
prepeart:
	cmp rax,0  				  
	jne not_end  			  
	jmp end_proc 			  
not_end:					  
	sub rax, 8
	push qword ptr [r10+rax];
	jmp prepeart
end_proc:
	xor r10,r10
	sub rsp,20h
	call qword ptr [rbp+30h];func
	mov rsp,rbp
	pop rbp
	ret  					  
ArgumentsPrepareCallForFastcall endp







ArgumentsPrepareCallFor_V_AMD64 PROC FRAME
	push rbp
	.pushreg rbp
	mov rbp,rsp
    .setframe rbp, 0h
	.endprolog

	mov rax ,qword ptr [rbp+38h];argc
	mov r10 ,qword ptr [rbp+40h];args
	shl rax,3
prepeart:
	cmp rax,0  				  
	jne not_end  			  
	jmp end_proc 			  
not_end:					  
	sub rax, 8
	push qword ptr [r10+rax];
	jmp prepeart
end_proc:
	xor r10,r10
	sub rsp,148h
	call qword ptr [rbp+30h];func
	mov rsp,rbp
	pop rbp
	ret  					  
ArgumentsPrepareCallFor_V_AMD64 endp





justJump proc
	mov [rsp+8], rcx
	ret
justJump endp











endif




END