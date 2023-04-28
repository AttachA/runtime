// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "dynamic_call.hpp"

#ifdef _WIN64
//https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160
//fastcall x64
extern "C" uint64_t __fastcall ArgumentsPrepareCallForFastcall(void* rcx, void* rdx, void* r8, void* r9, DynamicCall::PROC proc, size_t values_count, void* args);
extern "C" uint64_t CallTypeCall(DynamicCall::PROC proc, list_array<DynamicCall::ArgumentsHolder::ArgumentItem>& arguments, size_t struct_size, bool used_this, void* this_pointer) {
	if (used_this)
		arguments.push_front(DynamicCall::ArgumentsHolder::ArgumentItem(this_pointer));
	
	
	void* register_rcx = nullptr; //fir arg
	void* register_rdx = nullptr; //sec arg
	void* register_r8 = nullptr; //thr arg
	void* register_r9 = nullptr; //fou arg



	char i = 0;
	if (struct_size) {
		register_rcx = new char[struct_size];
		i = 1;
	}
	for (; i < 4; i++) {
		if (!arguments.size())
			break;
		switch (i) {
		case 0:
			register_rcx = arguments.front().ptr;
			break;
		case 1:
			register_rdx = arguments.front().ptr;
			break;
		case 2:
			register_r8 = arguments.front().ptr;
			break;
		case 3:
			register_r9 = arguments.front().ptr;
			break;
		}
		arguments.pop_front();
	}
	if (arguments.size()) {
		if (!(arguments.size() & 1))
			arguments.push_back(0);
		arguments.commit();
		return ArgumentsPrepareCallForFastcall(
			register_rcx,
			register_rdx,
			register_r8,
			register_r9,
			proc,
			arguments.size(),
			&arguments[0]
		);
	}
	else {
		return ArgumentsPrepareCallForFastcall(
			register_rcx,
			register_rdx,
			register_r8,
			register_r9,
			proc,
			0,
			nullptr
		);
	}
}
#elif defined(LINUX) && defined(__x86_64__)
extern "C" uint64_t __fastcall ArgumentsPrepareCallFor_V_AMD64(void* rsi, void* rdi, void* rcx, void* rcx, void* rdx, void* r8, void* r9, DynamicCall::PROC proc, size_t values_count, void* args);
extern "C" uint64_t CallTypeCall(DynamicCall::PROC proc, list_array<DynamicCall::ArgumentsHolder::ArgumentItem>&arguments, size_t struct_size, bool used_this, void* this_pointer) {
	if (used_this)
		arguments.push_front(DynamicCall::ArgumentsHolder::ArgumentItem(this_pointer));


	void* register_rsi = nullptr; //fir arg
	void* register_rdi = nullptr; //sec arg
	void* register_rcx = nullptr; //thr arg
	void* register_rdx = nullptr; //fou arg
	void* register_r8 = nullptr; //fiv arg
	void* register_r9 = nullptr; //six arg



	char i = 0;
	if (struct_size) {
		register_rcx = new char[struct_size];
		i = 1;
	}
	for (; i < 4; i++) {
		if (!arguments.size())
			break;
		switch (i) {
		case 0:
			register_rsi = arguments.front().ptr;
			break;
		case 1:
			register_rdi = arguments.front().ptr;
			break;
		case 2:
			register_rcx = arguments.front().ptr;
			break;
		case 3:
			register_rdx = arguments.front().ptr;
			break;
		case 4:
			register_r8 = arguments.front().ptr;
			break;
		case 5:
			register_r9 = arguments.front().ptr;
			break;
		}
		arguments.pop_front();
	}
	if (arguments.size()) {
		if (!(arguments.size() & 1))
			arguments.push_back(0);
		arguments.commit();
		return ArgumentsPrepareCallFor_V_AMD64(
			register_rsi,
			register_rdi,
			register_rcx,
			register_rdx,
			register_r8,
			register_r9,
			proc,
			arguments.size(),
			&arguments[0]
		);
	}
	else {
		return ArgumentsPrepareCallFor_V_AMD64(
			register_rsi,
			register_rdi,
			register_rcx,
			register_rdx,
			register_r8,
			register_r9,
			proc,
			0,
			nullptr
		);
	}
}
#else
#ifdef _WIN32
//cdecl
uint64_t FunctionCall(DynamicCall::PROC proc, void** args, int max_i, void* struct_mem) {
	size_t i = 0;
	void* arg;
	__asm {
		mov ebx, max_i
		repeart :
		cmp ebx, 0;
		jne not_end
			jmp end
			not_end :
		sub ebx, 1
	}
	arg = args[i++];
	__asm {
		push arg;
		jmp repeart
			end :
		cmp struct_mem, 0;
		je just_call
			push struct_mem
			just_call :
		call proc
	}
}
#else
//System V i386 ABI
uint64_t FunctionCall(DynamicCall::PROC proc, void** args, int max_i, void* struct_mem) {
	size_t i = 0;
	void* arg;
	__asm__ volatile("mov %0, %%edx"::"r"(max_i) : "%edx");
repeart:
	__asm__ volatile(
		"cmp 0, %edx\n\t"
		"jne not_end\n\t"
		"jmp end"
	);
not_end:
	__asm__ volatile("push %0"::"r"(arg):);
	arg = args[i++];
	goto repeart;
	end:
	__asm__ volatile(
		"cmp 0, %0\n\t"
		"je just_call"
		::"r"(struct_mem):
	);
	__asm__ volatile("push %0"::"r"(struct_mem):);
just_call:
	__asm__ volatile("call %0"::"r"(proc):);
}
#endif

uint64_t CallTypeCall(DynamicCall::PROC proc, list_array<DynamicCall::ArgumentsHolder::ArgumentItem>& arguments, size_t struct_size, bool used_this, void* this_pointer) {

	list_array<void*> arguments_res;
	if (used_this)
		arguments_res.push_front(this_pointer);
	for (size_t i = 0; i < arguments.size(); i++) {
		auto& it = arguments[i];
		if (it.is_8bit)
		{
			union {
				uint64_t full;
				void* part[2];
			} tmp;
			tmp.full = *(uint64_t*)it.ptr;
			arguments_res.push_back((void*)*(uint64_t*)it.ptr);
			arguments_res.push_back(tmp.part[1]);
		}
		else if (size_t struct_len = it.value_size) {
			struct_len = struct_len / 8 + ((struct_len % 8) ? true : false);
			size_t* tmp = (size_t*)it.ptr;
			for (size_t i = 0; i < struct_len; i++)
				arguments_res.push_back((void*)tmp[i]);

		}
		else
			arguments_res.push_back(it.ptr);
	}
	arguments_res.flip();

	void* struct_mem = nullptr;
	if (struct_size)
		struct_mem = new char[struct_size];

	return FunctionCall(
		proc,
		&arguments_res[0],
		arguments_res.size(),
		struct_mem
	);
}
#endif


namespace DynamicCall {
	namespace Calls {
		uint64_t call(PROC proc, ArgumentsHolder& ah, bool used_this, void* this_pointer) {
			return CallTypeCall(proc, ah.GetArguments(), 0, used_this, this_pointer);
		}
		void callNR(PROC proc, ArgumentsHolder& ah, bool used_this, void* this_pointer) {
			CallTypeCall(proc, ah.GetArguments(), 0, used_this, this_pointer);
		}
		void* callPTR(PROC proc, ArgumentsHolder& ah, bool used_this, void* this_pointer) {
			return (void*)CallTypeCall(proc, ah.GetArguments(), 0, used_this, this_pointer);
		}
	}
}