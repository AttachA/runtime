// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CASM.hpp"
#ifdef _WIN64
#include <Windows.h>
#include <DbgHelp.h>
#include <dbgeng.h>
namespace art{
	void _______dbgOut(const char* str) {
		OutputDebugStringA(str);
	}
}
#elif defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#include <cxxabi.h>
#endif
#include <string>
#include <unordered_map>
#include <cassert>
#include "../threading.hpp"
namespace art{
#define CONV_ASMJIT(to_conv) { if(auto tmp = (to_conv)) throw CompileTimeException("Fail create func asmjit err code: " + std::to_string(tmp)); }
	//return offset from allocated to additional size or code size
	size_t CASM::allocate_and_prepare_code(size_t additional_size_begin, uint8_t*& res, CodeHolder* code, asmjit::JitAllocator* alloc, size_t additional_size_end) {
		res = 0;
		CONV_ASMJIT(code->flatten());
		if(code->hasUnresolvedLinks()){
			CONV_ASMJIT(code->resolveUnresolvedLinks());
		}

		size_t estimatedCodeSize = code->codeSize();
		if (ASMJIT_UNLIKELY(estimatedCodeSize == 0))
			throw CompileTimeException("Code is empty");

		uint8_t* rx;
		uint8_t* rw;
		CONV_ASMJIT(alloc->alloc((void**)&rx, (void**)&rw, additional_size_begin + estimatedCodeSize + additional_size_end));
		rw += additional_size_begin;
		// Relocate the code.
		Error err = code->relocateToBase(uintptr_t((void*)rx) + additional_size_begin);
		if (ASMJIT_UNLIKELY(err)) {
			alloc->release(rx);
			return err;
		}

		// Recalculate the final code size and shrink the memory we allocated for it
		// in case that some relocations didn't require records in an address table.
		size_t codeSize = code->codeSize();

		

		for (asmjit::Section* section : code->_sections) {
			size_t offset = size_t(section->offset());
			size_t bufferSize = size_t(section->bufferSize());
			size_t virtualSize = size_t(section->virtualSize());

			assert(offset + bufferSize <= codeSize);
			memcpy(rw + offset, section->data(), bufferSize);

			if (virtualSize > bufferSize) {
				assert(offset + virtualSize <= codeSize);
				memset(rw + offset + bufferSize, 0, virtualSize - bufferSize);
			}
		}
		if (codeSize < estimatedCodeSize)
			alloc->shrink(rx, additional_size_begin + codeSize + additional_size_end);

#if defined(_M_X64) || defined(__x86_64__)
#if defined(__GNUC__)
	uint8_t* start = rx;
	uint8_t* end = start + codeSize + additional_size_begin + additional_size_end;
	__builtin___clear_cache(start, end);
#endif
#else
# if defined(_WIN32)
	// Windows has a built-in support in `kernel32.dll`.
	::FlushInstructionCache(::GetCurrentProcess(), rx, codeSize + additional_size);
# elif defined(__GNUC__)
	char* start = static_cast<char*>(const_cast<void*>(rx));
	char* end = start + codeSize + additional_size;
	__builtin___clear_cache(start, end);
# else
	DebugUtils::unused(p, size);
# endif
#endif
		res = rx;
		return codeSize;
	}
	void CASM::release_code(uint8_t* code, asmjit::JitAllocator* alloc) {
		alloc->release(code);
	}

	struct frame_info {
		std::string name;
		std::string file;
		size_t fun_size = 0;
	};
	std::unordered_map<uint8_t*, frame_info> frame_symbols;
#ifdef _WIN64
#pragma comment(lib,"Dbghelp.lib")
	mutex DbgHelp_lock;





	struct NativeSymbolResolver {
		std::unordered_map<void*, StackTraceItem> memoized;
		NativeSymbolResolver() {
			SymInitialize(GetCurrentProcess(), nullptr, true);
		}
		~NativeSymbolResolver() {
			SymCleanup(GetCurrentProcess());
		}

		StackTraceItem GetName(void* frame) {
			{
				auto it = memoized.find(frame);
				if (it != memoized.end())
					return it->second;
			}
			unsigned char buffer[sizeof(SYMBOL_INFO) + 128];
			PSYMBOL_INFO symbol64 = reinterpret_cast<SYMBOL_INFO*>(buffer);
			memset(symbol64, 0, sizeof(SYMBOL_INFO) + 128);
			symbol64->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol64->MaxNameLen = 128;

			DWORD64 displacement = 0;
			BOOL result = SymFromAddr(GetCurrentProcess(), (DWORD64)frame, &displacement, symbol64);
			if (result) {
				IMAGEHLP_LINE64 line64;
				DWORD displacement = 0;
				memset(&line64, 0, sizeof(IMAGEHLP_LINE64));
				line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				result = SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)frame, &displacement, &line64);
				if(memoized.size() > 1000)
					memoized.erase(memoized.begin());
				if (result)
					return memoized[frame] = { symbol64->Name, line64.FileName, line64.LineNumber };
				else
					return memoized[frame] = { symbol64->Name, line64.FileName ? line64.FileName: "UNDEFINED" , SIZE_MAX};
			}
			return { "UNDEFINED","UNDEFINED", SIZE_MAX };
		}
	};

	uint32_t CaptureStackTrace___(uint32_t max_frames, void** out_frames, CONTEXT& context) {
		memset(out_frames, 0, sizeof(void*) * max_frames);

		// RtlCaptureStackBackTrace doesn't support RtlAddFunctionTable..
		//return RtlCaptureStackBackTrace(0, max_frames, out_frames, nullptr);
		
		//if you wanna port to x32 use RtlCaptureStackBackTrace
		UNWIND_HISTORY_TABLE history;
		memset(&history, 0, sizeof(UNWIND_HISTORY_TABLE));
		
		ULONG64 establisher_frame = 0;
		PVOID handler_data = nullptr;
		
		uint32_t frame;
		for (frame = 0; frame < max_frames; frame++) {
			ULONG64 image_base;
			PRUNTIME_FUNCTION rt_func = RtlLookupFunctionEntry(context.Rip, &image_base, &history);
		
			KNONVOLATILE_CONTEXT_POINTERS nv_context;
			memset(&nv_context, 0, sizeof(KNONVOLATILE_CONTEXT_POINTERS));
			if (!rt_func) {
				context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
				context.Rsp += 8;
			}
			else
				RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, context.Rip, rt_func, &context, &handler_data, &establisher_frame, &nv_context);
			
		
			if (!context.Rip)
				break;
		
			out_frames[frame] = (void*)context.Rip;
		}
		return frame;
	}

	uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
		CONTEXT context;
		RtlCaptureContext(&context);
		return CaptureStackTrace___(max_frames, out_frames, context);
	}
	uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip_frame) {
		CONTEXT context{0};
		context.Rip = (DWORD64)rip_frame;
		return CaptureStackTrace___(max_frames, out_frames, context);
	}


	size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
		//int PCIndex = int(pc - ((uint8_t*)(info->start)));
		//if (info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
		//for (unsigned i = 1; i < info->LineInfo.Size(); i++)
		//{
		//	if (info->LineInfo[i].InstructionIndex >= PCIndex)
		//	{
		//		return info->LineInfo[i - 1].LineNumber;
		//	}
		//}
		return SIZE_MAX;
	}




	template<typename Vec, typename T>
	void pushInVectorAsValue(std::vector<Vec>& vec, T value) {
		size_t _add = sizeof(T) / sizeof(Vec);
		size_t add = _add ? _add : sizeof(Vec) / sizeof(T);
		size_t source_pos = vec.size() * sizeof(Vec);
		vec.resize(vec.size() + add);
		*(T*)(((char*)vec.data()) + source_pos) = value;
	}
	template<typename Vec, typename T>
	void pushInVectorAsArray(std::vector<Vec>& vec, T* value, size_t size) {
		size_t _add = sizeof(T) / sizeof(Vec) * size;
		size_t add = _add ? _add : sizeof(Vec) / sizeof(T) * size;
		size_t source_pos = vec.size() * sizeof(Vec);
		vec.resize(vec.size() + add);
		T* ptr = (T*)(((char*)vec.data()) + source_pos);
		for(size_t i = 0; i < size; i++)
			ptr[i] = value[i];
	}

	//convert FrameResult struct to native unwindInfo
	std::vector<uint16_t> convert(FrameResult& frame) {
		auto& codes = frame.prolog;

		std::vector<uint16_t> info((sizeof(UWINFO_head) >> 1));
		info.reserve(codes.size() + codes.size() & 1);

		frame.head.Flags = frame.use_handle ? UNW_FLAG_EHANDLER : 0;
		*(UWINFO_head*)(info.data()) = frame.head;

		for (size_t i = codes.size(); i > 0; i--)
			info.push_back(codes[i - 1]);

		if (codes.size() & 1)
			info.push_back(0);

		if (frame.use_handle) {
			pushInVectorAsValue(info, frame.exHandleOff);
			std::vector<uint8_t> handler_info;
			for(size_t i = frame.scope_actions.size(); i > 0; i--){
				auto& action = frame.scope_actions[i - 1];
				handler_info.push_back((uint8_t)action.action);
				pushInVectorAsValue(handler_info, action.function_begin_off);
				pushInVectorAsValue(handler_info, action.function_end_off);
				switch (action.action) {
				case ScopeAction::Action::destruct_stack:
					pushInVectorAsValue(handler_info, action.destruct);
					pushInVectorAsValue(handler_info, action.stack_offset);
					break;
				case ScopeAction::Action::destruct_register:
					pushInVectorAsValue(handler_info, action.destruct_register);
					pushInVectorAsValue(handler_info, action.register_value);
					break;
				case ScopeAction::Action::filter:
					pushInVectorAsValue(handler_info, action.filter);
					pushInVectorAsValue(handler_info, action.filter_data_len);
					pushInVectorAsArray(handler_info, (char*)action.filter_data, action.filter_data_len);
					break;
				case ScopeAction::Action::finally:
					pushInVectorAsValue(handler_info, action.finally);
					pushInVectorAsValue(handler_info, action.finally_data_len);
					pushInVectorAsArray(handler_info, (char*)action.finally_data, action.finally_data_len);
					break;
				default:
					break;
				}
			}
			pushInVectorAsValue(handler_info, ScopeAction::Action::not_action);
			if(handler_info.size() & 1){
				handler_info.push_back(0xFFui8);
				pushInVectorAsArray(info, handler_info.data(), handler_info.size());
			}else{
				pushInVectorAsArray(info, handler_info.data(), handler_info.size());
				info.push_back(0xFFFF);
			}
		}
		return info;
	}
	void* FrameResult::init(uint8_t*& frame,CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name, const char* file_path) {
		std::vector<uint16_t> unwindInfo = convert(*this);
		size_t unwindInfoSize = unwindInfo.size() * sizeof(uint16_t);

		uint8_t* baseaddr;
		size_t fun_size = CASM::allocate_and_prepare_code(0, baseaddr, code, runtime.allocator(), unwindInfoSize + sizeof(RUNTIME_FUNCTION));
		if(!baseaddr){
			const char* err = asmjit::DebugUtils::errorAsString(asmjit::Error(fun_size));
			throw CompileTimeException(err);
		}
		
		uint8_t* startaddr = baseaddr;
		uint8_t* unwindptr = baseaddr + (((fun_size + 15) >> 4) << 4);
		memcpy(unwindptr, unwindInfo.data(), unwindInfoSize);

		RUNTIME_FUNCTION* table = (RUNTIME_FUNCTION*)(unwindptr + unwindInfoSize);
		frame = (uint8_t*)table;
		table[0].BeginAddress = (DWORD)(ptrdiff_t)(startaddr - baseaddr);
		table[0].EndAddress = (DWORD)(ptrdiff_t)(use_handle ? exHandleOff : fun_size);
		table[0].UnwindData = (DWORD)(ptrdiff_t)(unwindptr - baseaddr);
		BOOLEAN result = RtlAddFunctionTable(table, 1, (DWORD64)baseaddr);

		if (result == 0) {
			runtime.allocator()->release(baseaddr);
			throw CompileTimeException("RtlAddFunctionTable failed");
		}
		auto& tmp = frame_symbols[baseaddr];
		tmp.fun_size = fun_size;
		tmp.name = symbol_name;
		tmp.file = file_path;
		return baseaddr;
	}
	bool FrameResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime){
		if (frame) {
			BOOLEAN result = RtlDeleteFunctionTable((RUNTIME_FUNCTION*)frame);
			auto tmp = runtime.allocator()->release(funct);
			frame_symbols.erase((uint8_t*)funct);
			return !(result == FALSE || tmp);
		}
		return false;
	};
#else

	void _______dbgOut(const char*){
		//TODO
	}
	struct NativeSymbolResolver {
		StackTraceItem GetName(void* frame) {
			char** strings;
			void* frames[1] = { frame };
			strings = backtrace_symbols(frames, 1);

			char* ptr = strings[0];
			char* filename = ptr;
			const char* function = "";

			while (*ptr) {
				if (*ptr == '(') {
					*(ptr++) = 0;
					function = ptr;
					break;
				}
				ptr++;
			}

			if (function[0]) {
				while (*ptr) {
					if (*ptr == '+' || *ptr == ')') {
						*(ptr++) = 0;
						break;
					}
					ptr++;
				}
			}

			int status;
			char* new_function = abi::__cxa_demangle(function, nullptr, nullptr, &status);
			if (new_function)
				function = new_function;
			

			std::string s = "Called from " + std::string(function) + " at " + filename + '\n';
			if (new_function)
				free(new_function);
			free(strings);
			return { s,"UNDEFINED",SIZE_MAX };
		}
	};


	size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
		//int PCIndex = int(pc - ((uint8_t*)(info->start)));
		//if (info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
		//for (unsigned i = 1; i < info->LineInfo.Size(); i++)
		//{
		//	if (info->LineInfo[i].InstructionIndex >= PCIndex)
		//	{
		//		return info->LineInfo[i - 1].LineNumber;
		//	}
		//}
		return SIZE_MAX;
	}
	uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
		if(max_frames != (int32_t)max_frames)
			return 0;
		return (uint32_t)backtrace(out_frames, (int32_t)max_frames);
	}
	uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip) {
		return 0;
	}


	void* FrameResult::init(uint8_t*& frame, CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name, const char* file_path){
		//std::vector<uint16_t> unwindInfo = convert(*this);
		//size_t unwindInfoSize = unwindInfo.size() * sizeof(uint16_t);

		uint8_t* baseaddr;
		size_t fun_size = CASM::allocate_and_prepare_code(0, baseaddr, code, runtime.allocator(), 0/*unwindInfoSize + sizeof(RUNTIME_FUNCTION)*/);
		if(!baseaddr){
			const char* err = asmjit::DebugUtils::errorAsString(asmjit::Error(fun_size));
			throw CompileTimeException(err);
		}
		/*
		uint8_t* startaddr = baseaddr;
		uint8_t* unwindptr = baseaddr + (((fun_size + 15) >> 4) << 4);
		memcpy(unwindptr, unwindInfo.data(), unwindInfoSize);

		RUNTIME_FUNCTION* table = (RUNTIME_FUNCTION*)(unwindptr + unwindInfoSize);
		frame = (uint8_t*)table;
		table[0].BeginAddress = (DWORD)(ptrdiff_t)(startaddr - baseaddr);
		table[0].EndAddress = (DWORD)(ptrdiff_t)(use_handle ? exHandleOff : fun_size);
		table[0].UnwindData = (DWORD)(ptrdiff_t)(unwindptr - baseaddr);
		bool result = RtlAddFunctionTable(table, 1, (DWORD64)baseaddr);

		if (result == 0) {
			runtime.allocator()->release(baseaddr);
			throw CompileTimeException("RtlAddFunctionTable failed");
		}
		auto& tmp = frame_symbols[baseaddr];
		tmp.fun_size = fun_size;
		tmp.name = symbol_name;
		tmp.file = file_path;
		*/
		return baseaddr;
	}
	bool FrameResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime) {
		return !(runtime._release(funct));
	};
#endif


	StackTraceItem JitGetStackFrameName(NativeSymbolResolver* nativeSymbols, void* pc) {
		for (auto& it : frame_symbols) {
			auto& info = it.second;
			if (pc >= it.first && pc < (it.first + info.fun_size))
				return { info.name,  info.file, JITPCToLine((uint8_t*)pc, &info) };
		}
		return nativeSymbols ? nativeSymbols->GetName(pc) : StackTraceItem("UNDEFINED", "UNDEFINED", SIZE_MAX);
	}



	std::vector<StackTraceItem> FrameResult::JitCaptureStackTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
		std::lock_guard lg(DbgHelp_lock);//in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
		if (max_frames + 1 == 0)
			throw std::bad_array_new_length();
		max_frames += 1;
		std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
		void** frame = frames_buffer.get();
		uint32_t num_frames = CaptureStackTrace(max_frames, frame);

		std::unique_ptr<NativeSymbolResolver> nativeSymbols;
		if (includeNativeFrames)
			nativeSymbols.reset(new NativeSymbolResolver());

		std::vector<StackTraceItem> stack_trace;
		for (uint32_t i = framesToSkip + 1; i < num_frames; i++)
			stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
		return stack_trace;
	}

	std::vector<void*> FrameResult::JitCaptureStackChainTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
		std::lock_guard lg(DbgHelp_lock);//in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
		if (max_frames + 1 == 0)
			throw std::bad_array_new_length();
		max_frames += 1;
		std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
		void** frame = frames_buffer.get();
		uint32_t num_frames = CaptureStackTrace(max_frames, frame);
		if (framesToSkip >= num_frames)
			return {};
		else 
			return std::vector<void*>( frame + framesToSkip, frame + num_frames);
	}

	std::vector<StackTraceItem> FrameResult::JitCaptureExternStackTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
		std::lock_guard lg(DbgHelp_lock);//in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
		if (max_frames + 1 == 0)
			throw std::bad_array_new_length();
		max_frames += 1;
		std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
		void** frame = frames_buffer.get();
		uint32_t num_frames = CaptureStackTrace(max_frames, frame, rip);

		std::unique_ptr<NativeSymbolResolver> nativeSymbols;
		if (includeNativeFrames)
			nativeSymbols.reset(new NativeSymbolResolver());

		std::vector<StackTraceItem> stack_trace;
		for (uint32_t i = framesToSkip + 1; i < num_frames; i++)
			stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
		return stack_trace;
	}

	std::vector<void*> FrameResult::JitCaptureExternStackChainTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
		lock_guard lg(DbgHelp_lock);//in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
		if (max_frames + 1 == 0)
			throw std::bad_array_new_length();
		max_frames += 1;
		std::unique_ptr<void*,std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
		void** frame = frames_buffer.get();
		uint32_t num_frames = CaptureStackTrace(max_frames, frame, rip);
		if (framesToSkip >= num_frames)
			return {};
		else 
			return std::vector<void*>(frame + framesToSkip, frame + num_frames);
	}
	StackTraceItem FrameResult::JitResolveFrame(void* rip, bool include_native){
		if(include_native){
			#ifdef _WIN64
					lock_guard lg(DbgHelp_lock);//in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
			#endif
			std::unique_ptr<NativeSymbolResolver> nativeSymbols;
			nativeSymbols.reset(new NativeSymbolResolver());
			return JitGetStackFrameName(nativeSymbols.get(), rip);
		}else return JitGetStackFrameName(nullptr, rip);
	}
}