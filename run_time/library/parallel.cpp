#pragma once
#include "../AttachA_CXX.hpp"
namespace parallel {
	ProxyClassDefine define_ConditionVariable;
	ProxyClassDefine define_Mutex;
	ProxyClassDefine define_Semaphore;
	ProxyClassDefine define_ConcurentFile;
	ProxyClassDefine define_EventSystem;
	ProxyClassDefine define_TaskLimiter;
	ProxyClassDefine define_TaskQuery;
	ProxyClassDefine define_ValueMonitor;
	ProxyClassDefine define_ValueChangeMonitor;

	template<class Class_>
	inline typed_lgr<Class_> getClass(ValueItem* vals) {
		vals->getAsync();
		if (vals->meta.vtype == VType::proxy)
			return (*(typed_lgr<Class_>*)(((ProxyClass*)vals->getSourcePtr()))->class_ptr);
		else
			throw InvalidOperation("That function used only in proxy class");
	}


#pragma region ConditionVariable
	ValueItem* funs_ConditionVariable_wait(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 1: {
					std::mutex mt;
					MutexUnify unif(mt);
					std::unique_lock lock(unif);
					getClass<TaskConditionVariable>(vals)->wait(lock);
					return nullptr;
				}
				case 2: {
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);
							MutexUnify unif(*tmp.getPtr());
							std::unique_lock lock(unif);
							getClass<TaskConditionVariable>(vals)->wait(lock);
							return nullptr;
						}
						else
							throw  InvalidArguments("That function recuive [class ptr] and optional [mutex]");
					}
					else {
						std::mutex mt;
						MutexUnify unif(mt);
						std::unique_lock lock(unif);
						return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_for(lock,(size_t)vals[1]));
					}
				}
				case 3:
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);
							MutexUnify unif(*tmp.getPtr());
							std::unique_lock lock(unif);
							return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_for(lock, (size_t)vals[2]));
						}
						else throw InvalidArguments("That function recuive [class ptr], optional [mutex] and optional [milliseconds to timeout]");
					}
					else throw InvalidArguments("That function recuive [class ptr], optional [mutex] and optional [milliseconds to timeout]");
				default:
					throw InvalidArguments("That function recuive [class ptr] and optional [milliseconds to timeout]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_wait_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 2: {
					std::mutex mt;
					MutexUnify unif(mt);
					std::unique_lock lock(unif);
					return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_until(lock, std::chrono::high_resolution_clock::time_point((std::chrono::nanoseconds)(uint64_t)vals[1])));
				}
				case 3:
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);
							MutexUnify unif(*tmp.getPtr());
							std::unique_lock lock(unif);
							return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_until(lock, std::chrono::high_resolution_clock::time_point((std::chrono::nanoseconds)(uint64_t)vals[2])));
						}
						else
							throw  InvalidArguments("That function recuive [class ptr], optional [mutex] and optional  [milliseconds to timeout] ");
					}
				default:
					throw InvalidArguments("That function recuive only [class ptr], [mutex] and [time point value in nanoseconds] or [class ptr] and [time point value in nanoseconds]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_notify_one(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskConditionVariable>(vals)->notify_one();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_notify_all(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskConditionVariable>(vals)->notify_one();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_ConditionVariable() {
		define_ConditionVariable.copy = AttachA::Interface::special::proxyCopy<TaskConditionVariable, true>;
		define_ConditionVariable.destructor = AttachA::Interface::special::proxyDestruct<TaskConditionVariable, true>;
		define_ConditionVariable.name = "condition_variable";
		define_ConditionVariable.funs["wait"] = { new FuncEnviropment(funs_ConditionVariable_wait,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["wait_until"] = { new FuncEnviropment(funs_ConditionVariable_wait_until,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["notify_one"] = { new FuncEnviropment(funs_ConditionVariable_notify_one,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["notify_all"] = { new FuncEnviropment(funs_ConditionVariable_notify_all,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region Mutex
	ValueItem* funs_Mutex_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskMutex>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_unlock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskMutex>(vals)->unlock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if(len == 1)
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock());
				else
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock_for((size_t)vals[1]));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock_until(std::chrono::high_resolution_clock::time_point((std::chrono::nanoseconds)(uint64_t)vals[1])));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem((uint8_t)getClass<TaskMutex>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}
	void init_Mutex() {
		define_Mutex.copy = AttachA::Interface::special::proxyCopy<TaskMutex, true>;
		define_Mutex.destructor = AttachA::Interface::special::proxyDestruct<TaskMutex, true>;
		define_Mutex.name = "mutex";
		define_Mutex.funs["lock"] = { new FuncEnviropment(funs_Mutex_lock,false),false,ClassAccess::pub };
		define_Mutex.funs["unlock"] = { new FuncEnviropment(funs_Mutex_unlock,false),false,ClassAccess::pub };
		define_Mutex.funs["try_lock"] = { new FuncEnviropment(funs_Mutex_try_lock,false),false,ClassAccess::pub };
		define_Mutex.funs["try_lock_until"] = { new FuncEnviropment(funs_Mutex_try_lock_until,false),false,ClassAccess::pub };
		define_Mutex.funs["is_locked"] = { new FuncEnviropment(funs_Mutex_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region Semaphore
	ValueItem* funs_Semaphore_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_release(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->release();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_release_all(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->release_all();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if(len==1)
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock());
				else
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock_for((size_t)vals[1]));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock_until(std::chrono::high_resolution_clock::time_point((std::chrono::nanoseconds)(uint64_t)vals[1])));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem((uint8_t)getClass<TaskSemaphore>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_Semaphore() {
		define_Semaphore.copy = AttachA::Interface::special::proxyCopy<TaskSemaphore, true>;
		define_Semaphore.destructor = AttachA::Interface::special::proxyDestruct<TaskSemaphore, true>;
		define_Semaphore.name = "semaphore";
		define_Semaphore.funs["lock"] = { new FuncEnviropment(funs_Semaphore_lock,false),false,ClassAccess::pub };
		define_Semaphore.funs["release"] = { new FuncEnviropment(funs_Semaphore_release,false),false,ClassAccess::pub };
		define_Semaphore.funs["release_all"] = { new FuncEnviropment(funs_Semaphore_release_all,false),false,ClassAccess::pub };
		define_Semaphore.funs["try_lock"] = { new FuncEnviropment(funs_Semaphore_try_lock,false),false,ClassAccess::pub };
		define_Semaphore.funs["try_lock_until"] = { new FuncEnviropment(funs_Semaphore_try_lock_until,false),false,ClassAccess::pub };
		define_Semaphore.funs["is_locked"] = { new FuncEnviropment(funs_Semaphore_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region ConcurentFile
	ValueItem* funs_ConcurentFile_read(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				ValueItem res;
				switch (len) {
				case 2: {
					auto tmp = getClass<ConcurentFile>(vals);
					res = ConcurentFile::read(tmp, (uint32_t)vals[1]);
					break;
				}
				case 3: {
					auto tmp = getClass<ConcurentFile>(vals);
					res = ConcurentFile::read(tmp, (uint32_t)vals[1], (uint64_t)vals[2]);
					break;
				}
				default:
					throw InvalidArguments("Invalid arguments, argumets [class],[ui32 len],[optional ui64 pos]");
				}
				return new ValueItem(std::move(res));
			}
			throw InvalidOperation("That function used only in proxy class");
		}
		return nullptr;
	}
	template<uint8_t val_len>
	ValueItem ___funs_ConcurentFile_write_val(typed_lgr<ConcurentFile>& class_, ValueItem& item, uint64_t pos) {
		union {
			void* value;
			char proxy[val_len];
		};
		value = item.getSourcePtr();
		char* clone = new char[val_len];
		memcpy(clone, proxy, val_len);
		return ConcurentFile::write(class_, clone, val_len, pos);
	}
	template<uint8_t val_len>
	ValueItem ___funs_ConcurentFile_write_array(typed_lgr<ConcurentFile>& class_, ValueItem& item, uint64_t pos) {
		char* clone = new char[val_len * item.meta.val_len];
		memcpy(clone, item.getSourcePtr(), val_len * item.meta.val_len);
		return ConcurentFile::write(class_, clone, val_len * item.meta.val_len, pos);
	}
	ValueItem ___funs_ConcurentFile_write_string(typed_lgr<ConcurentFile>& class_,const std::string& ref, uint64_t pos) {
		if (ref.size() <= UINT32_MAX) {
			char* clone = new char[ref.size()];
			memcpy(clone, ref.c_str(), ref.size());
			return ConcurentFile::write(class_, clone, (uint32_t)ref.size(), pos);
		}
		else
			return ConcurentFile::write_long(class_, new list_array<uint8_t>(ref.begin(), ref.end(), ref.size()), pos);
	}
	ValueItem ___funs_ConcurentFile_write(typed_lgr<ConcurentFile>& class_, ValueItem& item, uint64_t pos) {
		item.getAsync();
		switch (item.meta.vtype) {
		case VType::noting:
			return nullptr;
		case VType::i8:
		case VType::ui8:
		case VType::type_identifier:
			return ___funs_ConcurentFile_write_val<1>(class_, item, pos);
		case VType::i16:
		case VType::ui16:
			return ___funs_ConcurentFile_write_val<2>(class_, item, pos);
		case VType::i32:
		case VType::ui32:
		case VType::flo:
			return ___funs_ConcurentFile_write_val<4>(class_, item, pos);
		case VType::i64:
		case VType::ui64:
		case VType::doub:
		case VType::undefined_ptr:
			return ___funs_ConcurentFile_write_val<8>(class_, item, pos);
		case VType::raw_arr_i8:
		case VType::raw_arr_ui8:
			return ___funs_ConcurentFile_write_array<1>(class_, item, pos);
		case VType::raw_arr_i16:
		case VType::raw_arr_ui16:
			return ___funs_ConcurentFile_write_array<2>(class_, item, pos);
		case VType::raw_arr_i32:
		case VType::raw_arr_ui32:
		case VType::raw_arr_flo:
			return ___funs_ConcurentFile_write_array<4>(class_, item, pos);
		case VType::raw_arr_i64:
		case VType::raw_arr_ui64:
		case VType::raw_arr_doub:
			return ___funs_ConcurentFile_write_array<8>(class_, item, pos);
		case VType::string:
			return ___funs_ConcurentFile_write_string(class_, *(std::string*)item.getSourcePtr(), pos);
		case VType::except_value:
			throw UnsupportedOperation("Fail to write except_value: cause that cannont be converted to ui8[] or string", (std::exception_ptr)item);
		case VType::class_:
		case VType::morph:
		case VType::proxy:
			if (AttachA::Interface::hasImplement(item, "to_u8[]")) {
				ValueItem tmp = AttachA::Interface::makeCall(ClassAccess::pub, item, "to_u8[]");
				return ___funs_ConcurentFile_write_array<1>(class_, item, pos);
			}
			else if (AttachA::Interface::hasImplement(item, "to_string")) 
				return ___funs_ConcurentFile_write_string(class_, (std::string)AttachA::Interface::makeCall(ClassAccess::pub, item, "to_string"), pos);
			else
				throw UnsupportedOperation("Fail to write unsupported interface: " + AttachA::Interface::name(item) + " cause that cannont be converted to ui8[] or string");
		case VType::faarr:
		case VType::saarr:
		case VType::uarr:
			throw NotImplementedException();
		default:
			throw UnsupportedOperation("Fail to write unsupported type: " + enum_to_string(item.meta.vtype) + " cause that cannont be converted to ui8[] or string");
		}
	}
	ValueItem* funs_ConcurentFile_write(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len == 1)
					throw InvalidArguments("Invalid arguments, argumets [class],[any],[optional ui64 pos],[optional any...]");
				auto class_ = getClass<ConcurentFile>(vals);

				ValueItem res;
				switch (len) {
				case 2:
					res = ___funs_ConcurentFile_write(
						class_,
						vals[1],
						-1
					);
					break;
				case 3:
					res = ___funs_ConcurentFile_write(
						class_,
						vals[1],
						(uint64_t)vals[2]
					);
					break;
				default: {
					uint64_t pos = (uint64_t)vals[2];
					len -= 3;
					ValueItem* faarr = new ValueItem[len]{};
					faarr[0] = ___funs_ConcurentFile_write(
						class_,
						vals[1],
						pos
					);
					++vals;
					++vals;
					++vals;
					uint32_t i = 1;
					while (--len) {
						faarr[i++] = ___funs_ConcurentFile_write(
							class_,
							*vals,
							pos
						);
						++vals;
					}
					res = ValueItem(faarr, i);
				}
				}
				return new ValueItem(std::move(res));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConcurentFile_append(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len == 1)
					throw InvalidArguments("Invalid arguments, argumets [class],[any...]");
				else if (len == 2) {
					auto class_ = getClass<ConcurentFile>(vals);
					return new ValueItem(___funs_ConcurentFile_write(class_, vals[1], -1));
				}
				else {
					auto class_ = getClass<ConcurentFile>(vals);
					len -= 2;
					ValueItem* faarr = new ValueItem[len];
					faarr[0] = ___funs_ConcurentFile_write(
						class_,
						vals[1],
						-1
					);
					++vals;
					++vals;
					uint32_t i = 1;
					while (--len) {
						faarr[i++] = ___funs_ConcurentFile_write(
							class_,
							*vals,
							-1
						);
						++vals;
					}
					return new ValueItem(faarr, i);
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConcurentFile_read_long(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				ValueItem res;
				switch (len) {
				case 2: {
					auto tmp = getClass<ConcurentFile>(vals);
					res = ConcurentFile::read_long(tmp, (uint64_t)vals[1]);
					break;
				}
				case 3: {
					auto tmp = getClass<ConcurentFile>(vals);
					res = ConcurentFile::read_long(tmp, (uint64_t)vals[1], (uint64_t)vals[2]);
					break;
				}
				default:
					throw InvalidArguments("Invalid arguments, argumets [class],[ui64 len],[optional ui64 pos]");
				}
				return new ValueItem(std::move(res));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem __funs_ConcurentFile_write_long_simple(typed_lgr<ConcurentFile>& class_, ValueItem& val, uint64_t pos = -1) {
		if (val.meta.vtype != VType::uarr)
			throw InvalidArguments("Function ConcurentFile write_long_simple used only for uarr cause them allow can store more items than '*arr*'");
		list_array<ValueItem>& to_pack = *(list_array<ValueItem>*)val.getSourcePtr();
		list_array<uint8_t> to_write;
		for (auto& it : to_pack) {
			union {
				void* val;
				uint8_t proxy[8];
			};
			switch (it.meta.vtype) {
			case VType::i8:
			case VType::ui8:
			case VType::type_identifier:
				val = it.getSourcePtr();
				to_write.push_back(proxy[0]);
				break;
			case VType::i16:
			case VType::ui16:
				val = it.getSourcePtr();
				to_write.push_back(proxy[0]);
				to_write.push_back(proxy[1]);
				break;
			case VType::i32:
			case VType::ui32:
			case VType::flo:
				val = it.getSourcePtr();
				to_write.push_back(proxy[0]);
				to_write.push_back(proxy[1]);
				to_write.push_back(proxy[2]);
				to_write.push_back(proxy[3]);
				break;
			case VType::i64:
			case VType::ui64:
			case VType::doub:
			case VType::undefined_ptr:
				val = it.getSourcePtr();
				to_write.push_back(proxy[0]);
				to_write.push_back(proxy[1]);
				to_write.push_back(proxy[2]);
				to_write.push_back(proxy[3]);
				to_write.push_back(proxy[4]);
				to_write.push_back(proxy[5]);
				to_write.push_back(proxy[6]);
				to_write.push_back(proxy[7]);
				break;
			default:
				throw InvalidArguments("Function ConcurentFile write_long_simple used only for uarr with simple types");
			}
		}
		return ConcurentFile::write_long(class_, new list_array<uint8_t>(std::move(to_write)), pos);
	}
	ValueItem* funs_ConcurentFile_write_long_simple(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				ValueItem res;
				switch (len) {
				case 1:
					throw InvalidArguments("Invalid arguments, argumets [class],[uarr<ints|floats|type_id|undefined_ptr>],[optional ui64 pos],[optional uarr<ints|floats|type_id|undefined_ptr>...]");
				case 2: {
					auto class_ = getClass<ConcurentFile>(vals);
					res = __funs_ConcurentFile_write_long_simple(class_, vals[1]);
					break;
				}
				case 3: {
					auto class_ = getClass<ConcurentFile>(vals);
					res = __funs_ConcurentFile_write_long_simple(class_, vals[1], (uint64_t)vals[2]);
					break;
				}
				default: {
					auto class_ = getClass<ConcurentFile>(vals);
					uint64_t pos = (uint64_t)vals[2];
					len -= 3;
					ValueItem* faarr = new ValueItem[len]{};
					faarr[0] = ___funs_ConcurentFile_write(
						class_,
						vals[1],
						pos
					);
					++vals;
					++vals;
					++vals;
					uint32_t i = 1;
					while (--len) {
						faarr[i++] = ___funs_ConcurentFile_write(
							class_,
							*vals,
							pos
						);
						++vals;
					}
					res = ValueItem(faarr, i);
				}
				}
				return new ValueItem(std::move(res));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConcurentFile_append_long_simple(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				ValueItem res;
				switch (len) {
				case 1:
					throw InvalidArguments("Invalid arguments, argumets [class],[uarr<ints|floats|type_id|undefined_ptr>...]");
				case 2: {
					auto class_ = getClass<ConcurentFile>(vals);
					res = __funs_ConcurentFile_write_long_simple(class_, vals[1]);
					break;
				}
				default: {
					auto class_ = getClass<ConcurentFile>(vals);
					len -= 2;
					ValueItem* faarr = new ValueItem[len]{};
					faarr[0] = __funs_ConcurentFile_write_long_simple(class_, vals[1]);
					++vals;
					++vals;
					uint32_t i = 1;
					while (--len) {
						faarr[i++] = __funs_ConcurentFile_write_long_simple(class_, *vals);
						++vals;
					}
					res = ValueItem(faarr, i);
				}
				}
				return new ValueItem(std::move(res));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConcurentFile_is_open(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				typed_lgr<ConcurentFile> class_ = getClass<ConcurentFile>(vals);
				return new ValueItem((uint8_t)ConcurentFile::is_open(class_));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConcurentFile_close(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				typed_lgr<ConcurentFile> class_ = getClass<ConcurentFile>(vals);
				return new ValueItem((uint8_t)ConcurentFile::is_open(class_));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_ConcurentFile() {
		define_ConcurentFile.copy = AttachA::Interface::special::proxyCopy<ConcurentFile, true>;;
		define_ConcurentFile.destructor = AttachA::Interface::special::proxyDestruct<ConcurentFile, true>;;
		define_ConcurentFile.name = "concurent_file";
		define_ConcurentFile.funs["read"] = { new FuncEnviropment(funs_ConcurentFile_read,false),false,ClassAccess::pub };
		define_ConcurentFile.funs["read_long"] = { new FuncEnviropment(funs_ConcurentFile_read_long,false),false,ClassAccess::pub };

		define_ConcurentFile.funs["write"] = { new FuncEnviropment(funs_ConcurentFile_write,false),false,ClassAccess::pub };
		define_ConcurentFile.funs["append"] = { new FuncEnviropment(funs_ConcurentFile_append,false),false,ClassAccess::pub };

		define_ConcurentFile.funs["write_long_simple"] = { new FuncEnviropment(funs_ConcurentFile_write_long_simple,false),false,ClassAccess::pub };
		define_ConcurentFile.funs["append_long_simple"] = { new FuncEnviropment(funs_ConcurentFile_append_long_simple,false),false,ClassAccess::pub };
		define_ConcurentFile.funs["is_open"] = { new FuncEnviropment(funs_ConcurentFile_is_open,false),false,ClassAccess::pub };
		define_ConcurentFile.funs["close"] = { new FuncEnviropment(funs_ConcurentFile_close,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region EventSystem
	ValueItem* funs_EventSystem_operator_add(ValueItem* vals, uint32_t len) {
		if(len < 2)
			throw InvalidArguments("That function recuive only [class ptr] and [function ptr]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			if (!fun) 
				throw InvalidArguments("That function recuive only [class ptr] and [function ptr]");
			getClass<EventSystem>(vals)->operator+=(*fun);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_join(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			bool as_async = len > 2 ? (bool)vals[2] : false;
			EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)vals[3] : EventSystem::Priorithy::avg;
			if (!fun)
				throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
			getClass<EventSystem>(vals)->join(*fun, as_async, priorithy);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_leave(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			bool as_async = len > 2 ? (bool)vals[2] : false;
			EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)vals[3] : EventSystem::Priorithy::avg;
			if (!fun)
				throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
			getClass<EventSystem>(vals)->leave(*fun, as_async, priorithy);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	ValueItem __funs_EventSystem_get_values0(ValueItem* vals, uint32_t len) {
		ValueItem values;
		if (len > 2) {
			size_t size = len - 1;
			ValueItem* args = new ValueItem[size]{};
			for (uint32_t i = 0; i < size; i++)
				args[i] = vals[i + 1];
			values = ValueItem(args, size, no_copy);
		}
		return values;
	}
	ValueItem* funs_EventSystem_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_sync_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->sync_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_await_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->await_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_async_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->async_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_EventSystem() {
		define_EventSystem.copy = AttachA::Interface::special::proxyCopy<EventSystem, true>;
		define_EventSystem.destructor = AttachA::Interface::special::proxyDestruct<EventSystem, true>;
		define_EventSystem.name = "event_system";

		define_EventSystem.funs["operator +"] = { new FuncEnviropment(funs_EventSystem_operator_add,false),false,ClassAccess::pub };

		define_EventSystem.funs["join"] = { new FuncEnviropment(funs_EventSystem_join,false),false,ClassAccess::pub };
		define_EventSystem.funs["leave"] = { new FuncEnviropment(funs_EventSystem_leave,false),false,ClassAccess::pub };

		define_EventSystem.funs["notify"] = { new FuncEnviropment(funs_EventSystem_notify,false),false,ClassAccess::pub };
		define_EventSystem.funs["sync_notify"] = { new FuncEnviropment(funs_EventSystem_sync_notify,false),false,ClassAccess::pub };

		define_EventSystem.funs["await_notify"] = { new FuncEnviropment(funs_EventSystem_await_notify,false),false,ClassAccess::pub };
		define_EventSystem.funs["async_notify"] = { new FuncEnviropment(funs_EventSystem_async_notify,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region TaskLimiter
	ValueItem* funs_TaskLimiter_set_max_treeshold(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [count]");
		if (vals->meta.vtype == VType::proxy) {
			getClass<TaskLimiter>(vals)->set_max_treeshold((uint64_t)vals[1]);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskLimiter>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_unlock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskLimiter>(vals)->unlock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 1:
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock());
				case 2:
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock_for((size_t)vals[1]));
				default:
					throw InvalidArguments("That function recuive only [class ptr] and optional [milliseconds to timeout]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock_until(std::chrono::high_resolution_clock::time_point((std::chrono::nanoseconds)(uint64_t)vals[1])));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem(getClass<TaskLimiter>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}
	void init_TaskLimiter() {
		define_TaskLimiter.copy = AttachA::Interface::special::proxyCopy<TaskLimiter, true>;
		define_TaskLimiter.destructor = AttachA::Interface::special::proxyDestruct<TaskLimiter, true>;
		define_TaskLimiter.name = "task_limiter";
		define_TaskLimiter.funs["set_max_treeshold"] = { new FuncEnviropment(funs_TaskLimiter_set_max_treeshold,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["lock"] = { new FuncEnviropment(funs_TaskLimiter_lock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["unlock"] = { new FuncEnviropment(funs_TaskLimiter_unlock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["try_lock"] = { new FuncEnviropment(funs_TaskLimiter_try_lock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["try_lock_until"] = { new FuncEnviropment(funs_TaskLimiter_try_lock_until,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["is_locked"] = { new FuncEnviropment(funs_TaskLimiter_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region TaskQuery
	ValueItem* funs_TaskQuery_add_task(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");
		if (vals->meta.vtype == VType::proxy) {
			ValueItem& val = vals[1];
			typed_lgr<FuncEnviropment> func;
			typed_lgr<FuncEnviropment> fault_func;
			std::chrono::steady_clock::time_point timeout = std::chrono::steady_clock::time_point::min();
			bool used_task_local = false;
			val.getAsync();
			if(val.meta.vtype == VType::faarr || val.meta.vtype == VType::saarr) {
				auto arr = (ValueItem*)val.getSourcePtr();
				if (arr->meta.vtype == VType::function)
					func = *arr->funPtr();
				else
					throw InvalidArguments("That function recuive [class ptr] [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");

				if(val.meta.val_len > 1 && arr[1].meta.vtype == VType::function) 
					fault_func = *arr[1].funPtr();
				if(val.meta.val_len > 2 && arr[1].meta.vtype == VType::time_point)
					timeout = (std::chrono::steady_clock::time_point)arr[2];
				if(val.meta.val_len > 3)
					used_task_local = (bool)arr[3];
			}
			ValueItem args = (len == 3) ? vals[2] : ValueItem();
			return new ValueItem(new typed_lgr(getClass<TaskQuery>(vals)->add_task(func, args,used_task_local,fault_func, timeout)), VType::async_res, no_copy);
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	ValueItem* funs_TaskQuery_enable(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->enable();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_disable(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->disable();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_in_query(ValueItem* vals, uint32_t len) {
		if (len == 2)
			if (vals->meta.vtype == VType::proxy){
				if(vals[1].meta.vtype != VType::async_res)
					throw InvalidArguments("That function recuive [class ptr] and [async result (task)]");
				return new ValueItem(getClass<TaskQuery>(vals)->in_query(*(typed_lgr<Task>*)vals[1].getSourcePtr()));
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_set_max_at_execution(ValueItem* vals, uint32_t len) {
		if (len == 2)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->set_max_at_execution((size_t)vals[1]);
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_get_max_at_execution(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->get_max_at_execution();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_TaskQuery() {
		define_TaskQuery.copy = AttachA::Interface::special::proxyCopy<TaskQuery, true>;
		define_TaskQuery.destructor = AttachA::Interface::special::proxyDestruct<TaskQuery, true>;
		define_TaskQuery.name = "task_query";
		define_TaskQuery.funs["add_task"] = { new FuncEnviropment(funs_TaskQuery_add_task,false),false,ClassAccess::pub };
		define_TaskQuery.funs["enable"] = { new FuncEnviropment(funs_TaskQuery_enable,false),false,ClassAccess::pub };
		define_TaskQuery.funs["disable"] = { new FuncEnviropment(funs_TaskQuery_disable,false),false,ClassAccess::pub };
		define_TaskQuery.funs["in_query"] = { new FuncEnviropment(funs_TaskQuery_in_query,false),false,ClassAccess::pub };
		define_TaskQuery.funs["set_max_at_execution"] = { new FuncEnviropment(funs_TaskQuery_set_max_at_execution,false),false,ClassAccess::pub };
		define_TaskQuery.funs["get_max_at_execution"] = { new FuncEnviropment(funs_TaskQuery_get_max_at_execution,false),false,ClassAccess::pub };
	}
#pragma endregion



	void init() {
		init_ConditionVariable();
		init_Mutex();
		init_Semaphore();
		init_ConcurentFile();
		init_EventSystem();
		init_TaskLimiter();
		init_TaskQuery();
	}





	namespace constructor {
		ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskConditionVariable()), &define_ConditionVariable), VType::proxy, no_copy);
		}
		ValueItem* createProxy_Mutex(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskMutex()), &define_ConditionVariable), VType::proxy, no_copy);
		}
		ValueItem* createProxy_Semaphore(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskSemaphore()), &define_Semaphore), VType::proxy, no_copy);
		}

		ValueItem* createProxy_ConcurentFile(ValueItem* val, uint32_t len) {
			if (len) {
				return new ValueItem(new ProxyClass(new typed_lgr(new ConcurentFile(((std::string)*val).c_str())), &define_ConcurentFile), VType::proxy, no_copy);
			}
			else
				throw InvalidArguments("Invalid arguments, argumets [string]");
		}

		ValueItem* createProxy_EventSystem(ValueItem* val, uint32_t len) {
			return new ValueItem(new ProxyClass(new typed_lgr(new EventSystem()), &define_EventSystem), VType::proxy, no_copy);
		}

		ValueItem* createProxy_TaskLimiter(ValueItem* val, uint32_t len) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskLimiter()), &define_TaskLimiter), VType::proxy, no_copy);
		}

		ValueItem* createProxy_TaskQuery(ValueItem* val, uint32_t len){
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskQuery(len ? (size_t)*val : 0)), &define_TaskQuery), VType::proxy, no_copy);
		}

		//ProxyClass createProxy_ValueMonitor() {
		//
		//}
		//ProxyClass createProxy_ValueChangeMonitor() {
		//
		//}
	}
	
	ValueItem* createThread(ValueItem* vals, uint32_t len) {
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");

		if (len != 1) {
			typed_lgr<FuncEnviropment> func = *vals->funPtr();
			ValueItem* copyArgs = new ValueItem[len - 1];
			vals++;
			len--;
			uint32_t i = 0;
			while (len--)
				copyArgs[i++] = *vals++;
			std::thread([](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
				auto tmp = FuncEnviropment::sync_call(func, args, len);
				if (tmp)
					delete tmp;
				delete[] args;
			}, func, copyArgs, i).detach();
		}
		else {
			std::thread([](typed_lgr<FuncEnviropment> func) {
				auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
				if (tmp)
					delete tmp;
			}, *vals->funPtr()).detach();
		}
		return nullptr;
	}
	ValueItem* createThreadAndWait(ValueItem* vals, uint32_t len) {
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");

		TaskConditionVariable cv;
		TaskMutex mtx;
		MutexUnify unif(mtx);
		std::unique_lock ul(unif);
		bool end = false;
		ValueItem* res = nullptr;
		typed_lgr<FuncEnviropment> func = *vals->funPtr();
		if (len != 1) {
			std::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
				try{
					auto tmp = FuncEnviropment::sync_call(func, args, len);
					std::unique_lock ul(mtx);
					res = tmp;
					end = true;
					cv.notify_all();
				}catch(...){
					std::unique_lock ul(mtx);
					try{
						res = new ValueItem(std::current_exception());
					}catch(...){
						end = true;
						cv.notify_all();
					}
					end = true;
					cv.notify_all();
				}
			}, func, vals, len).detach();

		}
		else {
			std::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnviropment> func) {
				try{
					auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
					std::unique_lock ul(mtx);
					res = tmp;
					end = true;
					cv.notify_all();
				}catch(...){
					std::unique_lock ul(mtx);
					try{
						res = new ValueItem(std::current_exception());
					}catch(...){
						end = true;
						cv.notify_all();
					}
					end = true;
					cv.notify_all();
				}
			}, func).detach();
		}
		while(!end)
			cv.wait(ul);
		return res;
	}

	struct _createAsyncThread_awaiter_struct {
		TaskConditionVariable cv;
		TaskMutex mtx;
		MutexUnify unif;
		bool end = false;
		ValueItem* res = nullptr;
		_createAsyncThread_awaiter_struct(){
			unif = MutexUnify(mtx);
		}
		~_createAsyncThread_awaiter_struct(){
			if(!end){
				std::unique_lock ul(mtx);
				end = true;
				cv.notify_all();
			}
		}
	};
	
	ValueItem* _createAsyncThread__Awaiter(ValueItem* val, uint32_t len){
		_createAsyncThread_awaiter_struct* awaiter = (_createAsyncThread_awaiter_struct*)val->getSourcePtr();
		std::unique_lock<MutexUnify> ul(awaiter->unif);
		try{
			while(!awaiter->end)
				awaiter->cv.wait(ul);
		}catch(...){
			ul.unlock();
			delete awaiter;
			throw;
		}
		ul.unlock();
		delete awaiter;
		return awaiter->res;
	}
	typed_lgr<FuncEnviropment> __createAsyncThread__Awaiter = new FuncEnviropment(_createAsyncThread__Awaiter, false);


	ValueItem* createAsyncThread(ValueItem* vals, uint32_t len){
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");
		typed_lgr<FuncEnviropment> func = *vals->funPtr();
		_createAsyncThread_awaiter_struct* awaiter = nullptr;
		try{
			awaiter = new _createAsyncThread_awaiter_struct();

			if (len != 1) {
				ValueItem* copyArgs = new ValueItem[len - 1];
				vals++;
				len--;
				uint32_t i = 0;
				while (len--)
					copyArgs[i++] = *vals++;

				std::thread([awaiter](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
					try{
						auto tmp = FuncEnviropment::sync_call(func, args, len);
						std::unique_lock ul(awaiter->mtx);
						awaiter->res = tmp;
						awaiter->end = true;
						awaiter->cv.notify_all();
					}catch(...){
						std::unique_lock ul(awaiter->mtx);
						try{
							awaiter->res = new ValueItem(std::current_exception());
						}catch(...){
							awaiter->end = true;
							awaiter->cv.notify_all();
						}
						awaiter->end = true;
						awaiter->cv.notify_all();
					}
				}, func, vals, len).detach();

			}
			else {
				std::thread([awaiter](typed_lgr<FuncEnviropment> func) {
					try{
						auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
						std::unique_lock ul(awaiter->mtx);
						awaiter->res = tmp;
						awaiter->end = true;
						awaiter->cv.notify_all();
					}catch(...){
						std::unique_lock ul(awaiter->mtx);
						try{
							awaiter->res = new ValueItem(std::current_exception());
						}catch(...){
							awaiter->end = true;
							awaiter->cv.notify_all();
						}
						awaiter->end = true;
						awaiter->cv.notify_all();
					}
				}, func).detach();
			}
		}
		catch(...) {
			if(awaiter)
				delete awaiter;
			throw;
		}
		ValueItem awaiter_args(awaiter);
		return new ValueItem(new typed_lgr(new Task(__createAsyncThread__Awaiter, awaiter_args)), VType::async_res, no_copy);
	}

	ValueItem* createTask(ValueItem* vals, uint32_t len){
		typed_lgr<FuncEnviropment> func;
		typed_lgr<FuncEnviropment> fault_func;
		std::chrono::steady_clock::time_point timeout = std::chrono::steady_clock::time_point::min();
		bool used_task_local = false;
		auto arr = (ValueItem*)vals->getSourcePtr();
		if (arr->meta.vtype == VType::function)
			func = *arr->funPtr();
		else
			throw InvalidArguments("That function recuive [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");

		if(arr->meta.val_len > 1 && arr[1].meta.vtype == VType::function) 
			fault_func = *arr[1].funPtr();
		if(arr->meta.val_len > 2 && arr[2].meta.vtype == VType::time_point)
			timeout = (std::chrono::steady_clock::time_point)arr[2];
		if(arr->meta.val_len > 3)
			used_task_local = (bool)arr[3];
			
		ValueItem args = (len == 3) ? vals[2] : ValueItem();
		return new ValueItem(new typed_lgr(new Task(func, args, used_task_local, fault_func, timeout)), VType::async_res, no_copy);
	}
}