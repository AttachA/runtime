#include <run_time/asm/dynamic_call_proxy.hpp>
#include <run_time/attacha_abi.hpp>
namespace art{
    namespace __attacha___{
		void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg) {
			if (!meta.allow_edit && meta.vtype == VType::string) {
				switch (meta.vtype) {
				case VType::noting:
					call.AddValueArgument((void*)0);
					break;
				case VType::i8:
					call.AddValueArgument(*(int8_t*)&arg);
					break;
				case VType::i16:
					call.AddValueArgument(*(int16_t*)&arg);
					break;
				case VType::i32:
					call.AddValueArgument(*(int32_t*)&arg);
					break;
				case VType::i64:
					call.AddValueArgument(*(int64_t*)&arg);
					break;
				case VType::ui8:
					call.AddValueArgument(*(uint8_t*)&arg);
					break;
				case VType::ui16:
					call.AddValueArgument(*(int16_t*)&arg);
					break;
				case VType::ui32:
					call.AddValueArgument(*(uint32_t*)&arg);
					break;
				case VType::ui64:
					call.AddValueArgument(*(uint64_t*)&arg);
					break;
				case VType::flo:
					call.AddValueArgument(*(float*)&arg);
					break;
				case VType::doub:
					call.AddValueArgument(*(double*)&arg);
					break;
				case VType::raw_arr_i8:
					call.AddArray((int8_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_i16:
					call.AddArray((int16_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_i32:
					call.AddArray((int32_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_i64:
					call.AddArray((int64_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_ui8:
					call.AddArray((uint8_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_ui16:
					call.AddArray((int16_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_ui32:
					call.AddArray((uint32_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_ui64:
					call.AddArray((uint64_t*)arg, meta.val_len);
					break;
				case VType::raw_arr_flo:
					call.AddArray((float*)arg, meta.val_len);
					break;
				case VType::raw_arr_doub:
					call.AddArray((double*)arg, meta.val_len);
					break;
				case VType::uarr:
					call.AddPtrArgument((const list_array<ValueItem>*)arg);
					break;
				case VType::string:
					call.AddArray(((art::ustring*)arg)->data(), ((art::ustring*)arg)->size());
					break;
				case VType::undefined_ptr:
					call.AddPtrArgument(arg);
					break;
				default:
					throw NotImplementedException();
				}
			}
			else {
				switch (meta.vtype) {
				case VType::noting:
					call.AddValueArgument((void*)0);
					break;
				case VType::i8:
					call.AddValueArgument(*(int8_t*)&arg);
					break;
				case VType::i16:
					call.AddValueArgument(*(int16_t*)&arg);
					break;
				case VType::i32:
					call.AddValueArgument(*(int32_t*)&arg);
					break;
				case VType::i64:
					call.AddValueArgument(*(int64_t*)&arg);
					break;
				case VType::ui8:
					call.AddValueArgument(*(uint8_t*)&arg);
					break;
				case VType::ui16:
					call.AddValueArgument(*(int16_t*)&arg);
					break;
				case VType::ui32:
					call.AddValueArgument(*(uint32_t*)&arg);
					break;
				case VType::ui64:
					call.AddValueArgument((uint64_t)arg);
					break;
				case VType::flo:
					call.AddValueArgument(*(float*)&arg);
					break;
				case VType::doub:
					call.AddValueArgument(*(double*)&arg);
					break;
				case VType::raw_arr_i8:
					call.AddPtrArgument((int8_t*)arg);
					break;
				case VType::raw_arr_i16:
					call.AddPtrArgument((int16_t*)arg);
					break;
				case VType::raw_arr_i32:
					call.AddPtrArgument((int32_t*)arg);
					break;
				case VType::raw_arr_i64:
					call.AddPtrArgument((int64_t*)arg);
					break;
				case VType::raw_arr_ui8:
					call.AddPtrArgument((uint8_t*)arg);
					break;
				case VType::raw_arr_ui16:
					call.AddPtrArgument((int16_t*)arg);
					break;
				case VType::raw_arr_ui32:
					call.AddPtrArgument((uint32_t*)arg);
					break;
				case VType::raw_arr_ui64:
					call.AddPtrArgument((uint64_t*)arg);
					break;
				case VType::raw_arr_flo:
					call.AddPtrArgument((float*)arg);
					break;
				case VType::raw_arr_doub:
					call.AddPtrArgument((double*)arg);
					break;
				case VType::uarr:
					call.AddPtrArgument((list_array<ValueItem>*)arg);
					break;
				case VType::string:
					call.AddPtrArgument(((art::ustring*)arg)->data());
					break;
				case VType::undefined_ptr:
					call.AddPtrArgument(arg);
					break;
				default:
					throw NotImplementedException();
				}
			}
		}
		ValueItem* NativeProxy_DynamicToStatic(DynamicCall::FunctionCall& call, DynamicCall::FunctionTemplate& nat_templ, ValueItem* arguments, uint32_t arguments_size) {
			using namespace DynamicCall;
			if (arguments) {
				while (arguments_size--) {
					ValueItem& it = *arguments++;
					auto to_add = call.ToAddArgument();
					if (to_add.is_void()) {
						if (call.is_variadic()) {
							void*& val = getValue(it.val, it.meta);
							NativeProxy_DynamicToStatic_addValue(call, it.meta, val);
							continue;
						}
						else
							break;
					}
					void*& arg = getValue(it.val, it.meta);
					ValueMeta meta = it.meta;
					switch (to_add.vtype) {
					case FunctionTemplate::ValueT::ValueType::integer:
					case FunctionTemplate::ValueT::ValueType::signed_integer:
					case FunctionTemplate::ValueT::ValueType::floating:
						if (to_add.ptype == FunctionTemplate::ValueT::PlaceType::as_value) {
							switch (meta.vtype) {
							case VType::i8:
								call.AddValueArgument(*(int8_t*)&arg);
								break;
							case VType::i16:
								call.AddValueArgument(*(int16_t*)&arg);
								break;
							case VType::i32:
								call.AddValueArgument(*(int32_t*)&arg);
								break;
							case VType::i64:
								call.AddValueArgument(*(int64_t*)&arg);
								break;
							case VType::ui8:
								call.AddValueArgument(*(uint8_t*)&arg);
								break;
							case VType::ui16:
								call.AddValueArgument(*(int16_t*)&arg);
								break;
							case VType::ui32:
								call.AddValueArgument(*(uint32_t*)&arg);
								break;
							case VType::ui64:
								call.AddValueArgument((uint64_t)arg);
								break;
							case VType::flo:
								call.AddValueArgument(*(float*)&arg);
								break;
							case VType::doub:
								call.AddValueArgument(*(double*)&arg);
								break;
							default:
								throw InvalidType("Required integer or floating family type but received another");
							}
						}
						else {
							switch (meta.vtype) {
							case VType::i8:
								call.AddValueArgument((int8_t*)&arg);
								break;
							case VType::i16:
								call.AddValueArgument((int16_t*)&arg);
								break;
							case VType::i32:
								call.AddValueArgument((int32_t*)&arg);
								break;
							case VType::i64:
								call.AddValueArgument((int64_t*)&arg);
								break;
							case VType::ui8:
								call.AddValueArgument((uint8_t*)&arg);
								break;
							case VType::ui16:
								call.AddValueArgument((uint16_t*)&arg);
								break;
							case VType::ui32:
								call.AddValueArgument((uint32_t*)&arg);
								break;
							case VType::ui64:
								call.AddValueArgument((uint64_t*)&arg);
								break;
							case VType::flo:
								call.AddValueArgument((float*)&arg);
								break;
							case VType::doub:
								call.AddValueArgument((double*)&arg);
								break;
							case VType::raw_arr_i8:
								call.AddValueArgument((int8_t*)arg);
								break;
							case VType::raw_arr_i16:
								call.AddValueArgument((int16_t*)arg);
								break;
							case VType::raw_arr_i32:
								call.AddValueArgument((int32_t*)arg);
								break;
							case VType::raw_arr_i64:
								call.AddValueArgument((int64_t*)arg);
								break;
							case VType::raw_arr_ui8:
								call.AddValueArgument((uint8_t*)arg);
								break;
							case VType::raw_arr_ui16:
								call.AddValueArgument((uint16_t*)arg);
								break;
							case VType::raw_arr_ui32:
								call.AddValueArgument((uint32_t*)arg);
								break;
							case VType::raw_arr_ui64:
								call.AddValueArgument((uint64_t*)arg);
								break;
							case VType::raw_arr_flo:
								call.AddValueArgument((float*)arg);
								break;
							case VType::raw_arr_doub:
								call.AddValueArgument((double*)arg);
								break;
							case VType::string:
								call.AddValueArgument(((art::ustring*)arg)->data());
								break;
							default:
								throw InvalidType("Required integer or floating family type but received another");
							}
						}
						break;
					case FunctionTemplate::ValueT::ValueType::pointer:
						switch (meta.vtype) {
						case VType::raw_arr_i8:
							call.AddValueArgument((int8_t*)arg);
							break;
						case VType::raw_arr_i16:
							call.AddValueArgument((int16_t*)arg);
							break;
						case VType::raw_arr_i32:
							call.AddValueArgument((int32_t*)arg);
							break;
						case VType::raw_arr_i64:
							call.AddValueArgument((int64_t*)arg);
							break;
						case VType::raw_arr_ui8:
							call.AddValueArgument((uint8_t*)arg);
							break;
						case VType::raw_arr_ui16:
							call.AddValueArgument((uint16_t*)arg);
							break;
						case VType::raw_arr_ui32:
							call.AddValueArgument((uint32_t*)arg);
							break;
						case VType::raw_arr_ui64:
							call.AddValueArgument((uint64_t*)arg);
							break;
						case VType::raw_arr_flo:
							call.AddValueArgument((float*)arg);
							break;
						case VType::raw_arr_doub:
							call.AddValueArgument((double*)arg);
							break;
						case VType::string:
							if (to_add.vsize == 1) {
								if (to_add.is_modifiable)
									call.AddPtrArgument(((art::ustring*)arg)->data());
								else
									call.AddArray(((art::ustring*)arg)->c_str(), ((art::ustring*)arg)->size());
							}
							else if (to_add.vsize == 2) {


							}
							else if (to_add.vsize == sizeof(art::ustring))
								call.AddPtrArgument((art::ustring*)arg);
							break;
						case VType::undefined_ptr:
							call.AddPtrArgument(arg);
							break;
						default:
							throw InvalidType("Required pointer family type but received another");
						}
						break;
					case FunctionTemplate::ValueT::ValueType::_class:
					default:
						break;
					}
				}
			}
			void* res = nullptr;
			try {
				res = call.Call();
			}
			catch (...) {
				if(!need_restore_stack_fault())
					throw;
			}
			if (restore_stack_fault())
				throw StackOverflowException();

			if (nat_templ.result.is_void())
				return nullptr;
			if (nat_templ.result.ptype == DynamicCall::FunctionTemplate::ValueT::PlaceType::as_ptr)
				return new ValueItem(res, VType::undefined_ptr);
			switch (nat_templ.result.vtype) {
			case DynamicCall::FunctionTemplate::ValueT::ValueType::integer:
				switch (nat_templ.result.vsize) {
				case 1:
					return new ValueItem(res, VType::ui8);
				case 2:
					return new ValueItem(res, VType::ui16);
				case 4:
					return new ValueItem(res, VType::ui32);
				case 8:
					return new ValueItem(res, VType::ui64);
				default:
					throw InvalidCast("Invalid type for convert");
				}
			case DynamicCall::FunctionTemplate::ValueT::ValueType::signed_integer:
				switch (nat_templ.result.vsize) {
				case 1:
					return new ValueItem(res, VType::i8);
				case 2:
					return new ValueItem(res, VType::i16);
				case 4:
					return new ValueItem(res, VType::i32);
				case 8:
					return new ValueItem(res, VType::i64);
				default:
					throw InvalidCast("Invalid type for convert");
				}
			case DynamicCall::FunctionTemplate::ValueT::ValueType::floating:
				switch (nat_templ.result.vsize) {
				case 1:
					return new ValueItem(res, VType::ui8);
				case 2:
					return new ValueItem(res, VType::ui16);
				case 4:
					return new ValueItem(res, VType::flo);
				case 8:
					return new ValueItem(res, VType::doub);
				default:
					throw InvalidCast("Invalid type for convert");
				}
			case DynamicCall::FunctionTemplate::ValueT::ValueType::pointer:
				return new ValueItem(res, VType::undefined_ptr);
			case DynamicCall::FunctionTemplate::ValueT::ValueType::_class:
			default:
				throw NotImplementedException();
			}
		}
	}
}