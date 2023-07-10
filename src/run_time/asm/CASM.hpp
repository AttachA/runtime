// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef RUN_TIME_CASM
#define RUN_TIME_CASM
#include <asmjit/asmjit.h>
#include <vector>
#include <cassert>
#include <unordered_map>
#include "../library/exceptions.hpp"
#include "../cxxException.hpp"
#include "../attacha_abi_structs.hpp"
namespace art{
	using asmjit::CodeHolder;
	using asmjit::Error;
	using asmjit::Label;
	using asmjit::x86::Assembler;

	using creg = asmjit::x86::Gp;
	using creg128 = asmjit::x86::Xmm;
	using creg64 = asmjit::x86::Gpq;
	using creg32 = asmjit::x86::Gpd;
	using creg16 = asmjit::x86::Gpw;
	using creg8 = asmjit::x86::Gpb;
#ifdef _WIN64
	constexpr creg8 resr_8l = asmjit::x86::al;
	constexpr creg8 argr0_8l = asmjit::x86::cl;
	constexpr creg8 argr1_8l = asmjit::x86::dl;
	constexpr creg8 argr2_8l = asmjit::x86::r8b;
	constexpr creg8 argr3_8l = asmjit::x86::r9b;
	constexpr creg8 resr_8h = asmjit::x86::ah;
	constexpr creg8 argr0_8h = asmjit::x86::ch;
	constexpr creg8 argr1_8h = asmjit::x86::dh;

	constexpr creg16 resr_16 = asmjit::x86::ax;
	constexpr creg16 argr0_16 = asmjit::x86::cx;
	constexpr creg16 argr1_16 = asmjit::x86::dx;
	constexpr creg16 argr2_16 = asmjit::x86::r8w;
	constexpr creg16 argr3_16 = asmjit::x86::r9w;

	constexpr creg32 resr_32 = asmjit::x86::eax;
	constexpr creg32 argr0_32 = asmjit::x86::ecx;
	constexpr creg32 argr1_32 = asmjit::x86::edx;
	constexpr creg32 argr2_32 = asmjit::x86::r8d;
	constexpr creg32 argr3_32 = asmjit::x86::r9d;

	constexpr creg64 resr = asmjit::x86::rax;
	constexpr creg64 argr0 = asmjit::x86::rcx;
	constexpr creg64 argr1 = asmjit::x86::rdx;
	constexpr creg64 argr2 = asmjit::x86::r8;
	constexpr creg64 argr3 = asmjit::x86::r9;

	constexpr creg64 enviro_ptr = asmjit::x86::r12;
	constexpr creg64 arg_ptr = asmjit::x86::r13;
	constexpr creg32 arg_len_32 = asmjit::x86::r14d;
	constexpr creg64 arg_len = asmjit::x86::r14;


	constexpr creg64 stack_ptr = asmjit::x86::rsp;
	constexpr creg64 frame_ptr = asmjit::x86::rbp;



	constexpr creg128 vec0 = asmjit::x86::xmm0;
	constexpr creg128 vec1 = asmjit::x86::xmm1;
	constexpr creg128 vec2 = asmjit::x86::xmm2;
	constexpr creg128 vec3 = asmjit::x86::xmm3;
	constexpr creg128 vec4 = asmjit::x86::xmm4;
	constexpr creg128 vec5 = asmjit::x86::xmm5;
	constexpr creg128 vec6 = asmjit::x86::xmm6;
	constexpr creg128 vec7 = asmjit::x86::xmm7;
	constexpr creg128 vec8 = asmjit::x86::xmm8;
	constexpr creg128 vec9 = asmjit::x86::xmm9;
	constexpr creg128 vec10 = asmjit::x86::xmm10;
	constexpr creg128 vec11 = asmjit::x86::xmm11;
	constexpr creg128 vec12 = asmjit::x86::xmm12;
	constexpr creg128 vec13 = asmjit::x86::xmm13;
	constexpr creg128 vec14 = asmjit::x86::xmm14;
	constexpr creg128 vec15 = asmjit::x86::xmm15;

#else
	constexpr creg8 resr_8h = asmjit::x86::ah;
	constexpr creg8 resr1_8h = asmjit::x86::dh;
	constexpr creg8 argr0_8h = asmjit::x86::dh;
	constexpr creg8 argr1_8h = asmjit::x86::sh;
	constexpr creg8 argr2_8h = asmjit::x86::dh;
	constexpr creg8 argr3_8h = asmjit::x86::ch;

	constexpr creg8 resr_8l = asmjit::x86::al;
	constexpr creg8 resr1_8l = asmjit::x86::dl;
	constexpr creg8 argr0_8l = asmjit::x86::dl;
	constexpr creg8 argr1_8l = asmjit::x86::sl;
	constexpr creg8 argr2_8l = asmjit::x86::dl;
	constexpr creg8 argr3_8l = asmjit::x86::cl;
	constexpr creg8 argr4_8l = asmjit::x86::r8b;
	constexpr creg8 argr5_8l = asmjit::x86::r9b;


	constexpr creg16 resr_16 = asmjit::x86::ax;
	constexpr creg16 resr0_16 = asmjit::x86::dx;
	constexpr creg16 argr0_16 = asmjit::x86::di;
	constexpr creg16 argr1_16 = asmjit::x86::si;
	constexpr creg16 argr2_16 = asmjit::x86::dx;
	constexpr creg16 argr3_16 = asmjit::x86::cx;
	constexpr creg16 argr4_16 = asmjit::x86::r8w;
	constexpr creg16 argr5_16 = asmjit::x86::r9w;

	constexpr creg32 resr_32 = asmjit::x86::eax;
	constexpr creg32 resr1_32 = asmjit::x86::edx;
	constexpr creg32 argr0_32 = asmjit::x86::edi;
	constexpr creg32 argr1_32 = asmjit::x86::esi;
	constexpr creg32 argr2_32 = asmjit::x86::edx;
	constexpr creg32 argr3_32 = asmjit::x86::ecx;
	constexpr creg32 argr4_32 = asmjit::x86::r8d;
	constexpr creg32 argr5_32 = asmjit::x86::r9d;

	constexpr creg64 resr = asmjit::x86::rax;
	constexpr creg64 resr1 = asmjit::x86::rdx;
	constexpr creg64 argr0 = asmjit::x86::rdi;
	constexpr creg64 argr1 = asmjit::x86::rsi;
	constexpr creg64 argr2 = asmjit::x86::rdx;
	constexpr creg64 argr3 = asmjit::x86::rcx;
	constexpr creg64 argr4 = asmjit::x86::r8;
	constexpr creg64 argr5 = asmjit::x86::r9;

	constexpr creg64 enviro_ptr = asmjit::x86::r13;

	constexpr creg64 arg_ptr = asmjit::x86::r12;
	constexpr creg64 temp_ptr = asmjit::x86::r14;
	constexpr creg64 mut_temp_ptr = asmjit::x86::r11;


	constexpr creg64 stack_ptr = asmjit::x86::rsp;
	constexpr creg64 frame_ptr = asmjit::x86::rbp;



	constexpr creg128 vec0 = asmjit::x86::xmm0;
	constexpr creg128 vec1 = asmjit::x86::xmm1;
	constexpr creg128 vec2 = asmjit::x86::xmm2;
	constexpr creg128 vec3 = asmjit::x86::xmm3;
	constexpr creg128 vec4 = asmjit::x86::xmm4;
	constexpr creg128 vec5 = asmjit::x86::xmm5;
	constexpr creg128 vec6 = asmjit::x86::xmm6;
	constexpr creg128 vec7 = asmjit::x86::xmm7;
	constexpr creg128 vec8 = asmjit::x86::xmm8;
	constexpr creg128 vec9 = asmjit::x86::xmm9;
	constexpr creg128 vec10 = asmjit::x86::xmm10;
	constexpr creg128 vec11 = asmjit::x86::xmm11;
	constexpr creg128 vec12 = asmjit::x86::xmm12;
	constexpr creg128 vec13 = asmjit::x86::xmm13;
	constexpr creg128 vec14 = asmjit::x86::xmm14;
	constexpr creg128 vec15 = asmjit::x86::xmm15;
#endif
#if defined(_M_X64) || defined(__x86_64__)
#define CASM_X64
#endif

#ifdef CASM_X64
#define CASM_REDZONE_SIZE 0x20

#define CASM_DEBUG
#ifdef CASM_DEBUG
#define casm_stack_align_check_v size_t stack_align_check = 8//call opcode use 8 bytes
#define casm_stack_align_check_dynamic assert(false && "In debug mode dynamic stack allocation disabled")
#define casm_stack_align_check_add(add) stack_align_check += add
#define casm_stack_align_check_rem(rem) stack_align_check -= rem
#define casm_stack_align_check_flush stack_align_check = 8
#define casm_stack_align_check_align if(stack_align_check & 15){stack_align_check &= -16; stack_align_check+=16;}
#define casm_stack_align_check assert(!(stack_align_check & 15) && "Align check failed")
#else
#define casm_value_align_check_v
#define casm_stack_align_check_dynamic
#define casm_stack_align_check_add(add)
#define casm_stack_align_check_rem(rem)
#define casm_stack_align_check_flush
#define casm_stack_align_check_align
#define casm_stack_align_check
#endif
#define casm_asmjit_direct_proxy_jump(function_, actual_function_)\
		void function_(asmjit::Label op){\
			a. actual_function_(op);\
		}

	struct ValueIndexContext{
		const std::vector<ValueItem*>& static_map;
		list_array<ValueItem>& values_pool;
	};
	class CASM {
		asmjit::x86::Assembler a;
		asmjit::Section* text;
		asmjit::Section* data = nullptr;
		casm_stack_align_check_v;
	public:
		bool resr_used = false;
		CASM(asmjit::CodeHolder& holder) :a(&holder) {
			text = holder.textSection();
			Error err = holder.newSection(&data, ".data",SIZE_MAX, asmjit::SectionFlags::kNone, 8);
			if (err)
				throw CompileTimeException("Failed to create data section due: " + std::string(asmjit::DebugUtils::errorAsString(err)));
		}

		static uint32_t enviroValueOffset(uint16_t off) {
			return (int32_t(off) << 1) * 8;
		}
		static uint32_t enviroMetaOffset(uint16_t off) {
			return ((int32_t(off) << 1) | 1) * 8;
		}
		static uint32_t enviroMetaSizeOffset(uint16_t off) {
			int32_t val_off = (int32_t(off) << 1) * 8;
			val_off += 12;
			return val_off;
		}



		void lea_valindex(const ValueIndexContext& context, ValueIndexPos value_index, creg64 res){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				lea(res, enviro_ptr, enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				lea(res, arg_ptr, enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(res, context.static_map[value_index.index]);
				break;
			case ValuePos::in_constants:
				mov(res, &context.values_pool[value_index.index + context.static_map.size()]);
			default:
				break;
			}
		}
		void lea_valindex_meta(const ValueIndexContext& context, ValueIndexPos value_index, creg64 res){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				lea(res, enviro_ptr, enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				lea(res, arg_ptr, enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(res, &context.static_map[value_index.index]->meta);
				break;
			case ValuePos::in_constants:
				mov(res, &context.values_pool[value_index.index + context.static_map.size()].meta);
			default:
				break;
			}
		}
		void lea_valindex_meta_size(const ValueIndexContext& context, ValueIndexPos value_index, creg64 res){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				lea(res, enviro_ptr, enviroMetaSizeOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				lea(res, arg_ptr, enviroMetaSizeOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(res, &context.static_map[value_index.index]->meta);
				break;
			case ValuePos::in_constants:
				mov(res, &context.values_pool[value_index.index + context.static_map.size()].meta.val_len);
			default:
				break;
			}
		}
		void mov_valindex(const ValueIndexContext& context, creg64 res, ValueIndexPos value_index, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov_long(res, enviro_ptr, enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				mov_long(res, arg_ptr, enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->val);
				mov_long(res, cache, 0);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].val);
				mov_long(res, cache, 0);
			default:
				break;
			}
		}
		void mov_valindex_meta(const ValueIndexContext& context, creg64 res, ValueIndexPos value_index, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov_long(res, enviro_ptr, enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				mov_long(res, arg_ptr, enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta);
				mov_long(res, cache, 0);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta);
				mov_long(res, cache, 0);
			default:
				break;
			}
		}
		void mov_valindex(const ValueIndexContext& context, ValueIndexPos value_index, creg64 set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroValueOffset(value_index.index), set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroValueOffset(value_index.index), set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]);
				mov(cache, 0, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()]);
				mov(cache, 0, set);
			default:
				break;
			}
		}
		void mov_valindex_meta(const ValueIndexContext& context, ValueIndexPos value_index, creg64 set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroMetaOffset(value_index.index), set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroMetaOffset(value_index.index), set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta);
				mov(cache, 0, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta);
				mov(cache, 0, set);
			default:
				break;
			}
		}
		void mov_valindex(const ValueIndexContext& context, ValueIndexPos value_index, const asmjit::Imm& set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroValueOffset(value_index.index), 8, set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroValueOffset(value_index.index), 8, set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->val);
				mov(cache, 0, 8, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].val);
				mov(cache, 0, 8, set);
			default:
				break;
			}
		}
		void mov_valindex_meta(const ValueIndexContext& context, ValueIndexPos value_index, const asmjit::Imm& set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroMetaOffset(value_index.index), 8, set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroMetaOffset(value_index.index), 8, set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta);
				mov(cache, 0, 8, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta);
				mov(cache, 0, 8, set);
			default:
				break;
			}
		}
		void mov_valindex_meta_size(const ValueIndexContext& context, ValueIndexPos value_index, const asmjit::Imm& set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroMetaSizeOffset(value_index.index), 4, set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroMetaSizeOffset(value_index.index), 4, set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta.val_len);
				mov(cache, 0, 4, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta.val_len);
				mov(cache, 0, 4, set);
			default:
				break;
			}
		}
		void mov_valindex_meta_size(const ValueIndexContext& context, ValueIndexPos value_index, creg32 set, creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, enviroMetaSizeOffset(value_index.index), 4, set);
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, enviroMetaSizeOffset(value_index.index), 4, set);
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta.val_len);
				mov(cache, 0, 4, set);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta.val_len);
				mov(cache, 0, 4, set);
			default:
				break;
			}
		}
		void mov_valindex_meta_size(const ValueIndexContext& context, creg32 res, ValueIndexPos value_index,  creg64 cache = resr){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov_int(res, enviro_ptr, enviroMetaSizeOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				mov_int(res, arg_ptr, enviroMetaSizeOffset(value_index.index));
				break;
			case ValuePos::in_static:
				mov(cache, &context.static_map[value_index.index]->meta.val_len);
				mov_int(res, cache, 0);
				break;
			case ValuePos::in_constants:
				mov(cache, &context.values_pool[value_index.index + context.static_map.size()].meta.val_len);
				mov_int(res, cache, 0);
			default:
				break;
			}
		}







		static size_t alignStackBytes(size_t bytes_count) {
			return (bytes_count + 15) & -16;
		}
		void stackAlloc(size_t bytes_count) {
			a.mov(resr, stack_ptr);
			a.sub(stack_ptr, bytes_count);
			casm_stack_align_check_add(bytes_count);
		}
		void stackAlloc(creg64 bytes_count) {
			a.mov(resr, stack_ptr);
			a.sub(stack_ptr, bytes_count);
			casm_stack_align_check_dynamic;
		}
		void stackIncrease(size_t bytes_count) {
			a.sub(stack_ptr, bytes_count);
			casm_stack_align_check_add(bytes_count);
		}
		void stackIncrease(creg64 bytes_count) {
			a.sub(stack_ptr, bytes_count);
			casm_stack_align_check_dynamic;
		}
		void stackReduce(size_t bytes_count) {
			a.add(stack_ptr, bytes_count);
			casm_stack_align_check_rem(bytes_count);
		}
		void stackReduce(creg64 bytes_count) {
			a.add(stack_ptr, bytes_count);
			casm_stack_align_check_dynamic;
		}
		void stackAlign() {
			a.and_(stack_ptr,-16);
			casm_stack_align_check_align;
		}


		void getEnviro(creg64 c0) {
			a.mov(c0, enviro_ptr);
		}
#pragma region mov reg reg
		void movA(creg c0, creg c1) {
			a.mov(c0, c1);
		}
		void mov(creg64 c0, creg64 c1) {
			a.mov(c0, c1);
		}
		void mov(creg32 c0, creg32 c1) {
			a.mov(c0, c1);
		}
		void mov(creg16 c0, creg16 c1) {
			a.mov(c0, c1);
		}
		void mov(creg8 c0, creg8 c1) {
			a.mov(c0, c1);
		}
#pragma endregion
#pragma region mov reg imm
		void mov(creg128 res, const asmjit::Imm& set) {
			if (resr_used)
				a.push(resr);

			a.mov(resr, set);
			casm_stack_align_check;
			a.vmovq(res, resr);

			if (resr_used)
				a.pop(resr);
		}
		void mov(creg res, const asmjit::Imm& set) {
			if (res.isVec())
				throw CompileTimeException("Invalid operation");
			a.mov(res, set);
		}
#pragma endregion
#pragma region mov reg (base:reg,off:int32_t)
		void mov_byte(creg8 res, creg64 get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_8(get, get_off));
		}
		void mov_short(creg16 res, creg64 get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_16(get, get_off));
		}
		void mov_int(creg32 res, creg64 get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_32(get, get_off));
		}
		void mov_long(creg64 res, creg64 get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_64(get, get_off));
		}
		void mov_vector(creg128 res, creg64 get, int32_t get_off) {
			a.movdqu(res, asmjit::x86::ptr_128(get, get_off));
		}
		void mov_default(creg res, creg64 get, int32_t get_off, uint32_t v_siz) {
			a.mov(res, asmjit::x86::ptr(get, get_off, v_siz));
		}
#pragma endregion
#pragma region mov reg (base:label,off:int32_t)
		void mov_byte(creg8 res, asmjit::Label get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_8(get, get_off));
		}
		void mov_short(creg16 res, asmjit::Label get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_16(get, get_off));
		}
		void mov_int(creg32 res, asmjit::Label get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_32(get, get_off));
		}
		void mov_long(creg64 res, asmjit::Label get, int32_t get_off) {
			a.mov(res, asmjit::x86::ptr_64(get, get_off));
		}
		void mov_vector(creg128 res, asmjit::Label get, int32_t get_off) {
			a.movdqu(res, asmjit::x86::ptr_128(get, get_off));
		}
		void mov_default(creg res, asmjit::Label get, int32_t get_off, uint32_t v_siz) {
			a.mov(res, asmjit::x86::ptr(get, get_off, v_siz));
		}
#pragma endregion
#pragma region mov reg (base:label,off:reg)
		void mov_byte(creg8 res, asmjit::Label get, creg8 get_off) {
			a.mov(res, asmjit::x86::ptr_8(get, get_off));
		}
		void mov_short(creg16 res, asmjit::Label get, creg16 get_off) {
			a.mov(res, asmjit::x86::ptr_16(get, get_off));
		}
		void mov_int(creg32 res, asmjit::Label get, creg32 get_off) {
			a.mov(res, asmjit::x86::ptr_32(get, get_off));
		}
		void mov_long(creg64 res, asmjit::Label get, creg64 get_off) {
			a.mov(res, asmjit::x86::ptr_64(get, get_off));
		}
		void mov_default(creg res, asmjit::Label get, creg get_index, uint32_t get_index_shift = 0, uint32_t offset = 0, uint32_t v_siz = 0) {
			a.mov(res, asmjit::x86::ptr(get, get_index, get_index_shift, v_siz));
		}
#pragma endregion
#pragma region mov reg (base:reg,off:reg)
		void mov_byte(creg8 res, asmjit::Label get, creg get_off) {
			a.mov(res, asmjit::x86::ptr_8(get, get_off));
		}
		void mov_short(creg16 res, asmjit::Label get, creg get_off) {
			a.mov(res, asmjit::x86::ptr_16(get, get_off));
		}
		void mov_int(creg32 res, asmjit::Label get, creg get_off) {
			a.mov(res, asmjit::x86::ptr_32(get, get_off));
		}
		void mov_long(creg64 res, asmjit::Label get, creg get_off) {
			a.mov(res, asmjit::x86::ptr_64(get, get_off));
		}
		void mov_vector(creg128 res, asmjit::Label get, creg get_off) {
			a.movdqu(res, asmjit::x86::ptr_128(get, get_off));
		}
		void mov_default(creg res, asmjit::Label get, creg get_off, uint32_t v_siz) {
			a.mov(res, asmjit::x86::ptr(get, get_off,0,0, v_siz));
		}
#pragma endregion

#pragma region mov (res:reg,res_off:int32_t) set(reg|imm)
		void mov(creg64 res, int32_t res_off, creg128 set) {
			a.movdqu(asmjit::x86::ptr(res, res_off, 16), set);
		}
		void mov(creg64 res, int32_t res_off, creg64 set) {
			a.mov(asmjit::x86::ptr(res, res_off, 8), set);
		}
		void mov(creg64 res, int32_t res_off, creg32 set) {
			a.mov(asmjit::x86::ptr(res, res_off, 4), set);
		}
		void mov(creg64 res, int32_t res_off, creg16 set) {
			a.mov(asmjit::x86::ptr(res, res_off, 2), set);
		}
		void mov(creg64 res, int32_t res_off, creg8 set) {
			a.mov(asmjit::x86::ptr(res, res_off, 1), set);
		}
		void mov(creg64 res, int32_t res_off, int32_t set_size, creg set) {
			a.mov(asmjit::x86::ptr(res, res_off, set_size), set);
		}
		void mov(creg64 res, int32_t res_off, int32_t set_size, const asmjit::Imm& set) {
			a.mov(asmjit::x86::ptr(res, res_off, set_size), set);
		}
#pragma endregion

		void mov(creg res, creg base, creg index, uint8_t shift, int32_t offset, uint32_t v_siz) {
			a.mov(res, asmjit::x86::ptr(base, index, shift, offset, v_siz));
		}
		void shift_left(creg64 c0, creg64 c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg32 c0, creg32 c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg16 c0, creg16 c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg8 c0, creg8 c1) {
			a.shl(c0, c1);
		}
		void shift_right(creg64 c0, creg64 c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg32 c0, creg32 c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg16 c0, creg16 c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg8 c0, creg8 c1) {
			a.shr(c0, c1);
		}

		void shift_left(creg64 c0, int8_t c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg32 c0, int8_t c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg16 c0, int8_t c1) {
			a.shl(c0, c1);
		}
		void shift_left(creg8 c0, int8_t c1) {
			a.shl(c0, c1);
		}
		void shift_right(creg64 c0, int8_t c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg32 c0, int8_t c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg16 c0, int8_t c1) {
			a.shr(c0, c1);
		}
		void shift_right(creg8 c0, int8_t c1) {
			a.shr(c0, c1);
		}

		void test(creg64 c0, creg64 c1) {
			a.test(c0, c1);
		}
		void test(creg32 c0, creg32 c1) {
			a.test(c0, c1);
		}
		void test(creg16 c0, creg16 c1) {
			a.test(c0, c1);
		}
		void test(creg8 c0, creg8 c1) {
			a.test(c0, c1);
		}
		void test(creg reg, const asmjit::Imm& v) {
			if (reg.isVec())
				throw CompileTimeException("Invalid operation");
			a.test(reg, v);
		}
		void test(creg64 res, int32_t res_off, int32_t vsize, const asmjit::Imm& v) {
			a.test(asmjit::x86::ptr(res, res_off, vsize), v);
		}

		void cmp(creg64 c0, creg64 c1) {
			a.cmp(c0, c1);
		}
		void cmp(creg32 c0, creg32 c1) {
			a.cmp(c0, c1);
		}
		void cmp(creg16 c0, creg16 c1) {
			a.cmp(c0, c1);
		}
		void cmp(creg8 c0, creg8 c1) {
			a.cmp(c0, c1);
		}
		void cmp(creg reg, const asmjit::Imm& v) {
			if (reg.isVec())
				throw CompileTimeException("Invalid operation");
			a.cmp(reg, v);
		}
		void cmp(creg64 res, int32_t res_off, int32_t set_size, const asmjit::Imm& set) {
			a.cmp(asmjit::x86::ptr(res, res_off, set_size), set);
		}


		void lea(creg64 res, creg64 set, int32_t set_off = 0, uint8_t set_size = 0) {
			a.lea(res, asmjit::x86::Mem(set, set_off, set_size));
		}
		void lea(creg64 res, asmjit::Label set) {
			a.lea(res, asmjit::x86::ptr(set));
		}
		void lea(creg64 res, asmjit::Label set, creg64 set_off) {
			a.lea(res, asmjit::x86::ptr(set, set_off,3));
		}

		void push(const asmjit::Imm& val) {
			casm_stack_align_check_add(8);
			a.push(val);
		}
		void push(creg val) {
			casm_stack_align_check_add(val.size());
			a.push(val);
		}
		void pop(creg res) {
			casm_stack_align_check_rem(res.size());
			a.pop(res);
		}
		void pop() {
			stackReduce(8);
		}

		template<class FUNC = asmjit::Label>
		void call(asmjit::Label label) {
			casm_stack_align_check;
			a.call(label);
		}
		template<class FUNC>
		void call(FUNC fun) {
			casm_stack_align_check;
			a.call((*(void**)(&fun)));
		}
		void jmp(const asmjit::Imm& pos) {
			a.jmp(pos);
		}
		void jmp(creg64 pos) {
			a.jmp(pos);
		}
		void jmp(asmjit::Label label) {
			a.jmp((const asmjit::Label&)label);
		}
		void jmp_in_label(asmjit::Label label) {
			a.jmp(asmjit::x86::ptr(label));
		}
		void jmp_equal(asmjit::Label op) {
			a.je(op);
		}


		void jmp_not_equal(asmjit::Label op) {
			a.jne(op);
		}

		
		void jmp_unsigned_more(asmjit::Label op) {
			a.ja(op);
		}
		void jmp_signed_more(asmjit::Label op) {
			a.jg(op);
		}
		void jmp_unsigned_lower(asmjit::Label op) {
			a.jb(op);
		}
		void jmp_signed_lower(asmjit::Label op) {
			a.jl(op);
		}

		void jmp_unsigned_more_or_eq(asmjit::Label op) {
			a.jae(op);
		}
		void jmp_signed_more_or_eq(asmjit::Label op) {
			a.jge(op);
		}
		void jmp_unsigned_lower_or_eq(asmjit::Label op) {
			a.jbe(op);
		}
		void jmp_signed_lower_or_eq(asmjit::Label op) {
			a.jle(op);
		}

		void jmp_zero(asmjit::Label op) {
			a.jz(op);
		}
		void jmp_not_zero(asmjit::Label op) {
			a.jnz(op);
		}

		void push_flags() {
			casm_stack_align_check_add(2);
			a.pushf();
		}
		void pop_flags() {
			casm_stack_align_check_rem(2);
			a.popf();
		}
		void push_dflags() {
			casm_stack_align_check_add(4);
			a.pushfd();
		}
		void pop_dflags() {
			casm_stack_align_check_rem(4);
			a.popfd();
		}
		void load_flag8h() {
			a.lahf();
		}
		void store_flag8h() {
			a.sahf();
		}

		void int3() {
			a.int3();
		}



		void xor_(creg64 c0, creg64 c1) {
			a.xor_(c0, c1);
		}
		void xor_(creg32 c0, creg32 c1) {
			a.xor_(c0, c1);
		}
		void xor_(creg16 c0, creg16 c1) {
			a.xor_(c0, c1);
		}
		void xor_(creg8 c0, creg8 c1) {
			a.xor_(c0, c1);
		}
		void xor_byte(creg8 res, creg64 base, int32_t off) {
			a.xor_(res, asmjit::x86::ptr_8(base, off));
		}
		void xor_short(creg16 res, creg64 base, int32_t off) {
			a.xor_(res, asmjit::x86::ptr_16(base, off));
		}
		void xor_int(creg32 res, creg64 base, int32_t off) {
			a.xor_(res, asmjit::x86::ptr_32(base, off));
		}
		void xor_long(creg64 res, creg64 base, int32_t off) {
			a.xor_(res, asmjit::x86::ptr_64(base, off));
		}
		void xor_(creg64 res, creg64 base, int32_t off) {
			a.xor_(res, asmjit::x86::ptr_64(base, off));
		}
		void xor_(creg res, const asmjit::Imm& v) {
			a.xor_(res, v);
		}
		void xor_(creg64 res, int32_t res_off, int32_t vsize, const asmjit::Imm& v) {
			a.xor_(asmjit::x86::ptr(res, res_off, vsize), v);
		}


		void or_(creg64 c0, creg64 c1) {
			a.or_(c0, c1);
		}
		void or_(creg32 c0, creg32 c1) {
			a.or_(c0, c1);
		}
		void or_(creg16 c0, creg16 c1) {
			a.or_(c0, c1);
		}
		void or_(creg8 c0, creg8 c1) {
			a.or_(c0, c1);
		}
		void or_byte(creg8 res, creg64 base, int32_t off) {
			a.or_(res, asmjit::x86::ptr_8(base, off));
		}
		void or_short(creg16 res, creg64 base, int32_t off) {
			a.or_(res, asmjit::x86::ptr_16(base, off));
		}
		void or_int(creg32 res, creg64 base, int32_t off) {
			a.or_(res, asmjit::x86::ptr_32(base, off));
		}
		void or_long(creg64 res, creg64 base, int32_t off) {
			a.or_(res, asmjit::x86::ptr_64(base, off));
		}
		void or_(creg64 res, creg64 base, int32_t off) {
			a.or_(res, asmjit::x86::ptr_64(base, off));
		}
		void or_(creg res, const asmjit::Imm& v) {
			a.or_(res, v);
		}
		void or_(creg64 res, int32_t res_off, int32_t vsize, const asmjit::Imm& v) {
			a.or_(asmjit::x86::ptr(res, res_off, vsize), v);
		}



		void and_(creg64 c0, creg64 c1) {
			a.and_(c0, c1);
		}
		void and_(creg32 c0, creg32 c1) {
			a.and_(c0, c1);
		}
		void and_(creg16 c0, creg16 c1) {
			a.and_(c0, c1);
		}
		void and_(creg8 c0, creg8 c1) {
			a.and_(c0, c1);
		}
		void and_byte(creg8 res, creg64 base, int32_t off) {
			a.and_(res, asmjit::x86::ptr_8(res, off));
		}
		void and_short(creg16 res, creg64 base, int32_t off) {
			a.and_(res, asmjit::x86::ptr_16(res, off));
		}
		void and_int(creg32 res, creg64 base, int32_t off) {
			a.and_(res, asmjit::x86::ptr_32(res, off));
		}
		void and_long(creg64 res, creg64 base, int32_t off) {
			a.and_(res, asmjit::x86::ptr_64(res, off));
		}
		void and_(creg64 res, creg64 base, int32_t off) {
			a.and_(res, asmjit::x86::ptr_64(res, off));
		}
		void and_(creg res, const asmjit::Imm& v) {
			a.and_(res, v);
		}
		void and_(creg64 res, int32_t res_off, int32_t vsize, const asmjit::Imm& v) {
			a.and_(asmjit::x86::ptr(res, res_off, vsize), v);
		}




		void ret() {
			a.ret();
		}

		void label_bind(const asmjit::Label& label) {
			a.bind(label);
		}

		void noting() {
			a.nop();
		}
		asmjit::Label newLabel() {
			return a.newLabel();
		}
		void offsettable(asmjit::Label table, creg index, creg result) {
			a.mov(result,asmjit::x86::ptr(table, index,0,0,result.size()));
		}

		asmjit::CodeHolder* code() { return a.code(); };
		asmjit::Label add_data(char* bytes, size_t size) {
			a.section(data);
			asmjit::Label label = a.newLabel();
			a.bind(label);
			a.embed(bytes, size);
			a.section(text);
			return label;
		}
		asmjit::Label add_label_ptr(asmjit::Label l) {
			a.section(data);
			asmjit::Label label = a.newLabel();
			a.bind(label);
			a.embedLabel(l);
			a.section(text);
			return label;
		}
		asmjit::Label add_table(const std::vector<asmjit::Label>& labels) {
			a.section(data);
			asmjit::Label label = a.newLabel();
			a.bind(label);
			for (auto& l : labels)
				a.embedLabel(l);
			a.section(text);
			return label;
		}
		void bind_data(asmjit::Label label,char* bytes, size_t size) {
			a.section(data);
			a.bind(label);
			a.embed(bytes, size);
			a.section(text);
		}
		void bind_label_ptr(asmjit::Label label, asmjit::Label l) {
			a.section(data);
			a.bind(label);
			a.embedLabel(l);
			a.section(text);
		}
		void bind_table(asmjit::Label label, const std::vector<asmjit::Label>& labels) {
			a.section(data);
			a.bind(label);
			for (auto& l : labels)
				a.embedLabel(l);
			a.section(text);
		}


		size_t offset() {
			return a.offset();
		}

		void sub(creg res, creg val) {
			a.sub(res, val);
		}
		void sub(creg res, uint64_t val) {
			a.sub(res, val);
		}
		void sub(creg64 res, int32_t off, creg val, uint8_t vsize = 0) {
			a.sub(asmjit::x86::ptr(res, off, vsize), val);
		}
		void sub(creg64 res, int32_t off, uint64_t val, uint8_t vsize = 0) {
			a.sub(asmjit::x86::ptr(res, off, vsize), val);
		}

		void sub(creg res, creg64 val, int32_t off, uint8_t vsize = 0) {
			a.sub(asmjit::x86::ptr(res, off, vsize), val);
		}

		void add(creg res, creg val) {
			a.add(res, val);
		}
		void add(creg res, uint64_t val) {
			a.add(res, val);
		}
		void add(creg64 res, int32_t off, creg val, uint8_t vsize = 0) {
			a.add(asmjit::x86::ptr(res, off, vsize), val);
		}
		void add(creg64 res, int32_t off, uint64_t val, uint8_t vsize = 0) {
			a.add(asmjit::x86::ptr(res, off, vsize), val);
		}

		void mul(creg res, creg val, creg val2) {
			a.mul(res, val, val2);
		}
		void imul(creg res, creg val, creg val2) {
			a.imul(res, val, val2);
		}
		void div(creg res, creg val, creg val2) {
			a.div(res, val, val2);
		}
		void idiv(creg res, creg val, creg val2) {
			a.idiv(res, val, val2);
		}


		void add(creg res, creg64 val, int32_t off, uint8_t vsize = 0) {
			a.add(asmjit::x86::ptr(res, off, vsize), val);
		}
		void atomic_increase(void* value_ptr){
			a.lock().inc(asmjit::x86::ptr(uint64_t(value_ptr),8));
		}
		void atomic_decrease(void* value_ptr){
			a.lock().dec(asmjit::x86::ptr(uint64_t(value_ptr),8));
		}
		void atomic_fetch_add(void* value_ptr, creg64 old){
			a.lock().xadd(asmjit::x86::ptr(uint64_t(value_ptr),8), old);
		}

		void insertNative(uint8_t* opcodes,uint32_t len){
			a.embed(opcodes, len);
		}
		void finalize(){
			a.section(data);
		}
		static size_t alocate_and_prepare_code(size_t additional_size_begin, uint8_t*& res, CodeHolder* code, asmjit::JitAllocator* alloc, size_t additional_size_end);
		static void relase_code(uint8_t* res, asmjit::JitAllocator* alloc);
		
	};


	namespace {
		union UWCODE {
			struct {
				uint8_t offset;
				uint8_t op : 4;
				uint8_t info : 4;
			};
			uint16_t solid = 0;
			UWCODE() = default;
			UWCODE(const UWCODE& copy) = default;
			UWCODE(uint8_t off, uint8_t oper, uint8_t inf) { offset = off; op = oper; info = inf; }
		};
		struct UWINFO_head {
			uint8_t Version : 3 = 1;
			uint8_t Flags : 5 = 0;
			uint8_t SizeOfProlog = 0;
			uint8_t CountOfUnwindCodes = 0;
			uint8_t FrameRegister : 4 = 5;
			uint8_t FrameOffset : 4 = 0;
		};
	}
	struct StackTraceItem {
		std::string fn_name;
		std::string file_path;
		size_t line;
		constexpr static size_t nline = -1;
	};

	struct ScopeAction{
		enum class Action : uint8_t{
			destruct_stack,
			destruct_register,
			filter,
			converter,
			finally,

			not_action = (uint8_t)-1
		} action : 8;
		union{
			void(*destruct)(void**);
			void(*destruct_register)(void*);
			//exception names
			bool(*filter)(CXXExInfo& info, void** handle_adress, void* filter_data, size_t len, void* rsp);
			//current exception in args
			void(*converter)(void*, size_t len, void* rsp);
			void(*finally)(void*, size_t len, void* rsp);
		};
		union{
			uint64_t stack_offset : 52;
			uint32_t register_value;
		};
		union{
			void* filter_data = 0;
			void* converter_data;
		};
		union{
			size_t filter_data_len = 0;
			size_t converter_data_len;
		};
		void (*cleanup_filter_data)(ScopeAction*) = nullptr;


		size_t function_begin_off;
		size_t function_end_off;


		ScopeAction(){}
		ScopeAction(const ScopeAction& copy) {
			*this = copy;
		}
		ScopeAction(ScopeAction&& move) {
			*this = std::move(move);
		}
		ScopeAction& operator=(const ScopeAction& copy) {
			action = copy.action;
			destruct = copy.destruct;
			stack_offset = copy.stack_offset;
			filter_data = copy.filter_data;
			filter_data_len = copy.filter_data_len;
			function_begin_off = copy.function_begin_off;
			function_end_off = copy.function_end_off;
			return *this;
		}
		ScopeAction& operator=(ScopeAction&& move) {
			action = move.action;
			destruct = move.destruct;
			stack_offset = move.stack_offset;
			filter_data = move.filter_data;
			filter_data_len = move.filter_data_len;
			function_begin_off = move.function_begin_off;
			function_end_off = move.function_end_off;

			move.action = Action::destruct_stack;
			move.destruct = nullptr;
			move.stack_offset = 0;
			move.filter_data = nullptr;
			move.filter_data_len = 0;
			move.function_begin_off = 0;
			move.function_end_off = 0;
			return *this;
		}
		~ScopeAction(){
			if(cleanup_filter_data)
				cleanup_filter_data(this);
		}


	};
	struct FrameResult {
		std::vector<uint16_t> prolog;
		list_array<ScopeAction> scope_actions;
		UWINFO_head head;
		uint32_t exHandleOff = 0;
		bool use_handle = false;

		//return uwind_info_ptr
		void* init(uint8_t*& frame, CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name="AttachA unnamed_symbol", const char* file_path ="");
		static bool deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime);
		static std::vector<void*> JitCaptureStackChainTrace(uint32_t framesToSkip = 0, bool includeNativeFrames = true, uint32_t max_frames = 32);
		static std::vector<StackTraceItem> JitCaptureStackTrace(uint32_t framesToSkip = 0, bool includeNativeFrames = true, uint32_t max_frames = 32);
		static std::vector<StackTraceItem> JitCaptureExternStackTrace(void* rip, uint32_t framesToSkip = 0, bool includeNativeFrames = true, uint32_t max_frames = 32);
		static std::vector<void*> JitCaptureExternStackChainTrace(void* rip, uint32_t framesToSkip = 0, bool includeNativeFrames = true, uint32_t max_frames = 32);
		static StackTraceItem JitResolveFrame(void* rip, bool include_native = true);
	};


	class BuildCall {
		CASM& csm;
		size_t arg_c = 0;
		size_t pushed = 0;
		size_t total_arguments = 0;
		bool aligned = false;
#ifdef _WIN64
	//#define callStart()
		void callStart() {
			if (!aligned) {
				if (total_arguments & 1 && total_arguments > 4) {
					csm.push(0);
					pushed += 8;
				}
				aligned = true;
			}
			if (total_arguments == 0) {
				if (arg_c == 4)
					throw CompileTimeException("Fail add argument, too much aguments");
			}else if (arg_c == total_arguments)
				throw CompileTimeException("Fail add argument, too much aguments");
		}
#else
#define callStart()
#endif // _WIN64
	public:
		BuildCall(CASM& a, size_t arguments) : csm(a), total_arguments(arguments){}
		~BuildCall() noexcept(false) {
			if (arg_c)
				throw InvalidOperation("Build call is incomplete, need finalization");
		}
		void setArguments(size_t arguments) {
			if (!aligned)
				total_arguments = arguments;
		}
		void needAlign() {
			if(!pushed)
				csm.stackAlign();
		}
		void addArg(creg64 reg) {
			callStart();
			switch (arg_c++) {
			case 0:
				if (argr0 != reg)
					csm.movA(argr0, reg);
				break;
			case 1:
				if (argr1 != reg)
					csm.movA(argr1, reg);
				break;
			case 2:
				if (argr2 != reg)
					csm.movA(argr2, reg);
				break;
			case 3:
				if (argr3 != reg)
					csm.movA(argr3, reg);
				break;
#ifndef _WIN64
			case 4:
				if (argr4 != reg)
					csm.movA(argr4, reg);
				break;
			case 5:
				if (argr5 != reg)
					csm.movA(argr5, reg);
				break;
#endif
			default:
				csm.push(reg);
				pushed += 8;
			}
		}
		void addArg(creg32 reg) {
			callStart();
			switch (arg_c++) {
			case 0:
				if (argr0_32 != reg)
					csm.movA(argr0_32, reg);
				break;
			case 1:
				if (argr1_32 != reg)
					csm.movA(argr1_32, reg);
				break;
			case 2:
				if (argr2_32 != reg)
					csm.movA(argr2_32, reg);
				break;
			case 3:
				if (argr3_32 != reg)
					csm.movA(argr3_32, reg);
				break;
#ifndef _WIN64
			case 4:
				if (argr4_32 != reg)
					csm.movA(argr4_32, reg);
				break;
			case 5:
				if (argr5_32 != reg)
					csm.movA(argr5_32, reg);
				break;
#endif
			default:
				csm.push(creg::fromTypeAndId(resr.type(), reg.id()));
				pushed += 8;
			}
		}
		void addArg(creg16 reg) {
			callStart();
			switch (arg_c++) {
			case 0:
				if (argr0_16 != reg)
					csm.movA(argr0_16, reg);
				break;
			case 1:
				if (argr1_16 != reg)
					csm.movA(argr1_16, reg);
				break;
			case 2:
				if (argr2_16 != reg)
					csm.movA(argr2_16, reg);
				break;
			case 3:
				if (argr3_16 != reg)
					csm.movA(argr3_16, reg);
				break;
#ifndef _WIN64
			case 4:
				if (argr4_16 != reg)
					csm.movA(argr4_16, reg);
				break;
			case 5:
				if (argr5_16 != reg)
					csm.movA(argr5_16, reg);
				break;
#endif
			default:
				csm.push(creg::fromTypeAndId(resr.type(), reg.id()));
				pushed += 8;
			}
		}
		void addArg(creg8 reg) {
			callStart();
			switch (arg_c++) {
			case 0:
				if (argr0_8l != reg)
					csm.movA(argr0_8l, reg);
				break;
			case 1:
				if (argr1_8l != reg)
					csm.movA(argr1_8l, reg);
				break;
			case 2:
				if (argr2_8l != reg)
					csm.movA(argr2_8l, reg);
				break;
			case 3:
				if (argr3_8l != reg)
					csm.movA(argr3_8l, reg);
				break;
#ifndef _WIN64
			case 4:
				if (argr4_8l != reg)
					csm.movA(argr4_8l, reg);
				break;
			case 5:
				if (argr5_8l != reg)
					csm.movA(argr5_8l, reg);
				break;
#endif
			default:
				csm.push(creg::fromTypeAndId(resr.type(), reg.id()));
				pushed += 8;
			}
		}
		void addArg(const asmjit::Imm& val,uint8_t val_size = 8) {
			callStart();
			switch (arg_c++) {
			case 0:
				csm.mov(argr0, val);
				break;
			case 1:
				csm.mov(argr1, val);
				break;
			case 2:
				csm.mov(argr2, val);
				break;
			case 3:
				csm.mov(argr3, val);
				break;
#ifndef _WIN64
			case 4:
				csm.mov(argr4, val);
				break;
			case 5:
				csm.mov(argr5, val);
				break;
#endif
			default:
				csm.push(val);
				pushed += val_size;
			}
		}
		inline void addArg(bool val) { addArg(val, 8); }
		inline void addArg(int8_t val) { addArg(val, 8); }
		inline void addArg(uint8_t val) { addArg(val, 8); }
		inline void addArg(int16_t val) { addArg(val, 8); }
		inline void addArg(uint16_t val) { addArg(val, 8); }
		inline void addArg(int32_t val) { addArg(val, 8); }
		inline void addArg(uint32_t val) { addArg(val, 8); }
		inline void addArg(int64_t val) { addArg(val, 8); }
		inline void addArg(uint64_t val) { addArg(val, 8); }
		inline void addArg(float val) { addArg(val, 8); }
		inline void addArg(double val) { addArg(val, 8); }
		inline void addArg(void* val) { addArg(val, 8); }

		void lea(creg64 reg, int32_t off, bool allow_use_resr = true) {
			callStart();
			switch (arg_c++) {
			case 0:
				csm.lea(argr0, reg, off);
				break;
			case 1:
				csm.lea(argr1, reg, off);
				break;
			case 2:
				csm.lea(argr2, reg, off);
				break;
			case 3:
				csm.lea(argr3, reg, off);
				break;
#ifndef _WIN64
			case 4:
				csm.lea(argr4, reg, off);
				break;
			case 5:
				csm.lea(argr5, reg, off);
				break;
#endif
			default:
				if (off) {
					if (allow_use_resr) {
						csm.lea(resr, reg, off);
						csm.push(resr);
					}
					else {
						csm.lea(reg, reg, off);
						csm.push(reg);
						csm.lea(reg, reg, -off);
					}
				} else csm.push(reg);
				pushed += 8;
			}
		}
		void mov(creg64 reg, int32_t off, bool allow_use_resr = true) {
			callStart();
			switch (arg_c++) {
			case 0:
				csm.mov_long(argr0, reg, off);
				break;
			case 1:
				csm.mov_long(argr1, reg, off);
				break;
			case 2:
				csm.mov_long(argr2, reg, off);
				break;
			case 3:
				csm.mov_long(argr3, reg, off);
				break;
#ifndef _WIN64
			case 4:
				csm.mov_long(argr4, reg, off);
				break;
			case 5:
				csm.mov_long(argr5, reg, off);
				break;
#endif
			default:
				if (off) {
					if (allow_use_resr) {
						csm.mov_long(resr, reg, off);
						csm.push(resr);
					}
					else {
						csm.stackIncrease(8);
						csm.push(reg);
						csm.mov_long(reg, reg, off);
						csm.mov(stack_ptr,-2, reg);
						csm.pop(reg);
					}
				}
				else 
					csm.push(reg);
				pushed += 8;
			}
		}

		
		void lea_valindex(const ValueIndexContext& context, ValueIndexPos value_index){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				lea(enviro_ptr,CASM::enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				lea(arg_ptr, CASM::enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_static:
				addArg(context.static_map[value_index.index]);
				break;
			case ValuePos::in_constants:
				addArg(&context.values_pool[value_index.index + context.static_map.size()]);
				break;
			default:
				break;
			}
		}
		void lea_valindex_meta(const ValueIndexContext& context, ValueIndexPos value_index){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				lea(enviro_ptr,CASM::enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				lea(arg_ptr, CASM::enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_static:
				addArg(&context.static_map[value_index.index]->meta);
				break;
			case ValuePos::in_constants:
				addArg(&context.values_pool[value_index.index + context.static_map.size()].meta);
				break;
			default:
				break;
			}
		}
		

		void mov_valindex(const ValueIndexContext& context, ValueIndexPos value_index){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, CASM::enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, CASM::enviroValueOffset(value_index.index));
				break;
			case ValuePos::in_static:
				addArg(context.static_map[value_index.index]->val);
				break;
			case ValuePos::in_constants:
				addArg(context.values_pool[value_index.index + context.static_map.size()].val);
				break;
			default:
				break;
			}
		}
		void mov_valindex_meta(const ValueIndexContext& context, ValueIndexPos value_index){
			switch (value_index.pos) {
			case ValuePos::in_enviro:
				mov(enviro_ptr, CASM::enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_arguments:
				mov(arg_ptr, CASM::enviroMetaOffset(value_index.index));
				break;
			case ValuePos::in_static:
				addArg(context.static_map[value_index.index]->meta.encoded);
				break;
			case ValuePos::in_constants:
				addArg(context.values_pool[value_index.index + context.static_map.size()].meta.encoded);
				break;
			default:
				break;
			}
		}
		void skip() {
			callStart();
			switch (arg_c++) {
			case 0:
			case 1:
			case 2:
			case 3:
#ifndef _WIN64
			case 4:
			case 5:
#endif
				break;
			default:
				csm.push(0ull);
				pushed += 8;
			}
		}

		template<class F>
		void finalize(F func) {
			//vector call calle impl
			csm.stackIncrease(CASM_REDZONE_SIZE);
			csm.call(func);
			csm.stackReduce(CASM_REDZONE_SIZE + pushed);
			pushed = 0;
			arg_c = 0;
			aligned = false;
		}
	};

	void _______dbgOut(const char*);
	class BuildProlog {
		FrameResult res;
		std::vector<std::pair<uint16_t, creg>> pushes;
		std::vector<std::pair<uint16_t, uint32_t>> stack_alloc;
		std::vector<std::pair<uint16_t, uint16_t>> set_frame;
		std::vector<std::pair<uint16_t, std::pair<creg, int32_t>>> save_to_stack;


		CASM& csm;
		enum UWC {
			UWOP_PUSH_NONVOL = 0,
			UWOP_ALLOC_LARGE = 1,
			UWOP_ALLOC_SMALL = 2,
			UWOP_SET_FPREG = 3,
			UWOP_SAVE_NONVOL = 4,
			UWOP_SAVE_NONVOL_FAR = 5,
			UWOP_SAVE_XMM128 = 8,
			UWOP_SAVE_XMM128_FAR = 9,
			UWOP_PUSH_MACHFRAME = 10,
		};
		size_t stack_align = 0;
		uint16_t cur_op = 0;
		bool frame_inited : 1 = false;
		bool prolog_preEnd : 1 = false;
	public:
		BuildProlog(CASM& a) : csm(a) {}
		~BuildProlog() {
			pushes.clear();
			stack_alloc.clear();
			set_frame.clear();
			save_to_stack.clear();
			if (frame_inited)
				return; 
			_______dbgOut("Frame not initalized!");
			::abort();
		}
		void pushReg(creg reg) {
			csm.push(reg.fromTypeAndId(asmjit::RegType::kGp64, reg.id()));
			if ((uint8_t)csm.offset() != csm.offset())
				throw CompileTimeException("prolog too large");
			res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_PUSH_NONVOL, reg.id()).solid);
			pushes.push_back({ cur_op++,reg });
			stack_align += 8;
		}
		void stackAlloc(uint32_t size) {
			if (size == 0)
				return;
			//align stack
			size = (size / 8 + ((size % 8) ? 1 : 0)) * 8;
			csm.sub(stack_ptr, size);
			if ((uint8_t)csm.offset() != csm.offset())
				throw CompileTimeException("prolog too large");
			if (size <= 128) 
				res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_SMALL, size / 8 - 1).solid);
			else if (size <= 524280) {
				//512K - 8
				res.prolog.push_back(size / 8);
				res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_LARGE, 0).solid);
			}
			else if(size <= 4294967288) {
				//4gb - 8
				res.prolog.push_back((uint16_t)(size >> 16));
				res.prolog.push_back((uint16_t)size);
				res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_LARGE, 1).solid);
			}
			else 
				throw CompileTimeException("Invalid uwind code, too large stack alocation");
			stack_alloc.push_back({ cur_op++, size });
			stack_align += size;
		}
		void setFrame(uint16_t stack_offset = 0) {
			if(frame_inited)
				throw CompileTimeException("Frame already inited");
			if (stack_offset % 16)
				throw CompileTimeException("Invalid frame offset, it must be aligned by 16");
			if(uint8_t(stack_offset / 16) != stack_offset / 16)
				throw CompileTimeException("frameoffset too large");
			csm.lea(frame_ptr, stack_ptr, stack_offset);
			if ((uint8_t)csm.offset() != csm.offset())
				throw CompileTimeException("prolog too large");
			res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SET_FPREG, 0).solid);
			set_frame.push_back({ cur_op++, stack_offset });
			res.head.FrameOffset = stack_offset / 16;
			frame_inited = true;
		}
		void saveToStack(creg reg, int32_t stack_back_offset) {
			if (reg.isVec()) {
				if (reg.type() == asmjit::RegType::kVec128) {
					if (INT32_MAX > stack_back_offset)
						throw CompileTimeException("Overflow, fail convert 64 point to 32 point");
					if (UINT16_MAX > stack_back_offset || stack_back_offset % 16) {
						csm.mov(stack_ptr, stack_back_offset, reg.as<creg128>());
						if ((uint8_t)csm.offset() != csm.offset())
							throw CompileTimeException("prolog too large");
						res.prolog.push_back(stack_back_offset & (UINT32_MAX ^ UINT16_MAX));
						res.prolog.push_back(stack_back_offset & UINT16_MAX);
						res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_XMM128_FAR, reg.id()).solid);
					}
					else {
						csm.mov(stack_ptr, stack_back_offset, reg.as<creg128>());
						if ((uint8_t)csm.offset() != csm.offset())
							throw CompileTimeException("prolog too large");
						res.prolog.push_back(uint16_t(stack_back_offset / 16));
						res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_XMM128, reg.id()).solid);
					}
				}
				else
					throw CompileTimeException("Supported only 128 bit vector register");
			}
			else {
				csm.mov(stack_ptr, stack_back_offset, reg.size(), reg);
				if ((uint8_t)csm.offset() != csm.offset())
					throw CompileTimeException("prolog too large");
				if (stack_back_offset % 8) {
					res.prolog.push_back(stack_back_offset & (UINT32_MAX ^ UINT16_MAX));
					res.prolog.push_back(stack_back_offset & UINT16_MAX);
					res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_NONVOL_FAR, reg.id()).solid);
				}
				else {
					res.prolog.push_back(uint16_t(stack_back_offset / 8));
					res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_NONVOL, reg.id()).solid);
				}
			}
			save_to_stack.push_back({ cur_op++, {reg,stack_back_offset} });
		}
		void alignPush() {
			if (stack_align & 0xF)
				stackAlloc(8);
		}
		size_t cur_stack_offset() {
			return stack_align;
		}
		void end_prolog() {
			if (prolog_preEnd)
				throw CompileTimeException("end_prolog will be used only once");
			if (stack_align & 0xF)
				stackAlloc(8);
			prolog_preEnd = true;
			if ((uint8_t)csm.offset() != csm.offset())
				throw CompileTimeException("prolog too large");
			res.head.SizeOfProlog = (uint8_t)csm.offset();
			if ((uint8_t)res.prolog.size() != res.prolog.size())
				throw CompileTimeException("prolog too large");
			res.head.CountOfUnwindCodes = (uint8_t)res.prolog.size();

			if (res.head.CountOfUnwindCodes & 1)
				res.prolog.push_back(0);
		}
		inline size_t offset(){
			return csm.offset();
		}


		ScopeAction* create_destruct(void(*func)(void**), uint64_t stack_offset = 0) {
			ScopeAction action;
			action.action = ScopeAction::Action::destruct_stack;
			action.destruct = func;
			action.stack_offset = stack_offset;
			res.scope_actions.push_back(action);
			return &res.scope_actions.back();
		}
		ScopeAction* create_destruct(void(*func)(void*), creg64 reg) {
			ScopeAction action;
			action.action = ScopeAction::Action::destruct_register;
			action.destruct_register = func;
			action.register_value = reg.id();
			res.scope_actions.push_back(action);
			return &res.scope_actions.back();
		}
		ScopeAction* create_filter(bool(*func)(CXXExInfo&, void**, void*, size_t, void*)) {
			ScopeAction action;
			action.action = ScopeAction::Action::filter;
			action.filter = func;
			res.scope_actions.push_back(action);
			return &res.scope_actions.back();
		}
		ScopeAction* create_converter(void(*func)(void*, size_t, void* rsp)) {
			ScopeAction action;
			action.action = ScopeAction::Action::converter;
			action.converter = func;
			res.scope_actions.push_back(action);
			return &res.scope_actions.back();
		}
		ScopeAction* create_null_action() {
			ScopeAction action;
			action.action = ScopeAction::Action::not_action;
			res.scope_actions.push_back(action);
			return &res.scope_actions.back();
		}

		inline void setScopeBegin(ScopeAction* action) {
			action->function_begin_off = csm.offset();
		}
		inline void setScopeEnd(ScopeAction* action) {
			action->function_end_off = csm.offset();
		}
		FrameResult& finalize_epilog() {
			while (cur_op--) {
				if (pushes.size()) {
					if (pushes.back().first == cur_op) {
						csm.pop(pushes.back().second);
						pushes.pop_back();
						continue;
					}
				}
				if (stack_alloc.size()) {
					if (stack_alloc.back().first == cur_op) {
						csm.stackReduce(stack_alloc.back().second);
						stack_alloc.pop_back();
						continue;
					}
				}
				if (set_frame.size()) {
					if (set_frame.back().first == cur_op) {
						csm.lea(stack_ptr, frame_ptr, -set_frame.back().second);
						set_frame.pop_back();
						continue;
					}
				}
				if (save_to_stack.size()) {
					if (save_to_stack.back().first == cur_op) {
						auto& reg = save_to_stack.back().second;
						if (reg.first.isVec())
							csm.mov_vector(reg.first.as<creg128>(), stack_ptr, reg.second);
						else
							csm.mov_default(reg.first, stack_ptr, reg.second, reg.first.size());
						save_to_stack.pop_back();
						continue;
					}
				}
				throw CompileTimeException("fail build prolog");
			}
			for(auto& i : res.scope_actions)
				if (i.function_end_off == 0)
					throw CompileTimeException("scope not finalized");
			return res;
		}
	};

	class ScopeManager{
		BuildProlog& csm;
		struct ExceptionScopes {
			std::vector<ScopeAction*> actions;
			size_t begin_off;
		};
		std::unordered_map<size_t, ExceptionScopes> scope_actions;
		std::unordered_map<size_t, ScopeAction*> value_lifetime;

		size_t ex_scopes=0;
		size_t value_lifetime_scopes=0;
	public:
		ScopeManager(BuildProlog& csm) :csm(csm) {}
		~ScopeManager() noexcept(false) {
			for (auto& i : scope_actions) 
				for(auto& j : i.second.actions)
					if (j->function_end_off == 0)
						throw CompileTimeException("scope not finalized");
			
			for(auto& i : value_lifetime)
				if (i.second->function_end_off == 0)
					throw CompileTimeException("scope not finalized");
		}
		size_t createExceptionScope() {
			size_t id = ex_scopes++;
			scope_actions[id] = ExceptionScopes({}, csm.offset());
			return id;
		}
		ScopeAction* setExceptionHandle(size_t id, bool(*filter_fun)(CXXExInfo&, void**, void*, size_t, void* rsp)){
			auto it = scope_actions.find(id);
			if (it == scope_actions.end())
				throw CompileTimeException("invalid exception scope");
			ScopeAction* action = csm.create_filter(filter_fun);
			action->function_begin_off = it->second.begin_off;
			it->second.actions.push_back(action);
			return action;
		}
		ScopeAction* setExceptionConverter(size_t id, void(*converter_fun)(void*, size_t len, void* rsp)) {
			auto it = scope_actions.find(id);
			if (it == scope_actions.end())
				throw CompileTimeException("invalid exception scope");
			ScopeAction* action = csm.create_converter(converter_fun);
			action->function_begin_off = it->second.begin_off;
			it->second.actions.push_back(action);
			return action;
		}
		size_t createValueLifetimeScope(void(*destructor)(void**), size_t stack_off) {
			ScopeAction* action = csm.create_destruct(destructor, stack_off);
			csm.setScopeBegin(action);
			size_t id = value_lifetime_scopes++;
			value_lifetime[id++] = action;
			return id;
		}
		size_t createValueLifetimeScope(void(*destructor)(void*), creg64 reg) {
			ScopeAction* action = csm.create_destruct(destructor, reg);
			csm.setScopeBegin(action);
			size_t id = value_lifetime_scopes++;
			value_lifetime[id] = action;
			return id;
		}
		void endValueLifetime(size_t id) {
			auto it = value_lifetime.find(id);
			if (it == value_lifetime.end())
				throw CompileTimeException("invalid value lifetime scope");
			csm.setScopeEnd(it->second);
			value_lifetime.erase(it);
		}
		void endExceptionScope(size_t id) {
			auto it = scope_actions.find(id);
			if (it == scope_actions.end())
				throw CompileTimeException("invalid exception scope");
			for(auto& i : it->second.actions)
				csm.setScopeEnd(i);
			scope_actions.erase(it);
		}
	};


#elif defined(__aarch64__) || defined(_M_ARM64)

#else
#error INVALID BUILD ARCHITECTURE, supported only x64 or aarch64 archetectures
#endif
}
#endif