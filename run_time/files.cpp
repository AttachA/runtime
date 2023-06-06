#include "files.hpp"
#include "library/exceptions.hpp"
#include "tasks_util/native_workers_singleton.hpp"
#include <Windows.h>
#include <io.h>
#include "../configuration/compatibility.hpp"
#if CONFIGURATION_COMPATIBILITY_ENABLE_FSTREAM_FROM_BLOCKINGFILEHANDLE
#include <fstream>
#endif
#include <utf8cpp/utf8.h>
namespace files {
    void io_error_to_exception(io_errors error){
        switch(error){
            case io_errors::eof: throw AException("FileException", "EOF");
            case io_errors::no_enough_memory: throw AException("FileException", "No enough memory");
            case io_errors::invalid_user_buffer: throw AException("FileException", "Invalid user buffer");
            case io_errors::no_enough_quota: throw AException("FileException", "No enough quota");
            case io_errors::unknown_error: throw AException("FileException", "Unknown error");
            default: throw AException("FileException", "Unknown error");
        }
    }
    class File_ : public NativeWorkerHandle {
        TaskConditionVariable awaiters;
        TaskMutex mutex;
        void* handle;
        char* buffer = nullptr;
        bool fullifed = false;
        bool is_canceled = false;
    public:
        typed_lgr<Task> awaiter;
        uint32_t fullifed_bytes = 0;
        const uint32_t buffer_size;
        const uint64_t offset;
        const bool is_read;
        const bool required_full;
        File_(NativeWorkerManager* manager, void* handle, char* buffer, uint32_t buffer_size, uint64_t offset) : NativeWorkerHandle(manager), handle(handle), is_read(false), buffer_size(buffer_size), offset(offset), required_full(true){
            SecureZeroMemory(&overlapped, sizeof(overlapped));
            overlapped.Offset = offset & 0xFFFFFFFF;
            overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
            this->buffer = new char[buffer_size];
            memcpy(this->buffer, buffer, buffer_size);
        }
        
        File_(NativeWorkerManager* manager, void* handle, uint32_t buffer_size, uint64_t offset, bool required_full = true) : NativeWorkerHandle(manager), handle(handle), is_read(true), buffer_size(buffer_size), offset(offset), required_full(required_full){
            SecureZeroMemory(&overlapped, sizeof(overlapped));
            overlapped.Offset = offset & 0xFFFFFFFF;
            overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
            this->buffer = new char[buffer_size];
        }
        ~File_(){
            if(buffer)
                delete[] buffer;
        }
        void cancel(){
            if(buffer){
                if(CancelIoEx(handle, &overlapped))
                    return;
                delete[] buffer;
                buffer = nullptr;
                MutexUnify unify(mutex);
                std::unique_lock<MutexUnify> lock(unify);
                fullifed = true;
                if(awaiter){
                    if(is_read && !required_full)
                        awaiter->fres.finalResult(ValueItem(), lock);
                    else
                        awaiter->fres.finalResult(ValueItem(), lock);
                }
                awaiters.notify_all();
            }
            delete this;
        }
        void await(){
            MutexUnify unify(mutex);
            std::unique_lock<MutexUnify> lock(unify);
            while(!fullifed)
                awaiters.wait(lock);
        }
        void now_fullifed(){
            MutexUnify unify(mutex);
            std::unique_lock<MutexUnify> lock(unify);
            fullifed = true;
            if(awaiter) {
                if(is_read)
                    awaiter->fres.finalResult(ValueItem((uint8_t*)buffer, buffer_size), lock);
                else 
                    awaiter->fres.finalResult(fullifed_bytes, lock);
            }
            awaiters.notify_all();
        }
        void exception(io_errors e){
            MutexUnify unify(mutex);
            std::unique_lock<MutexUnify> lock(unify);
            fullifed = true;
            if(awaiter) {
                if(fullifed_bytes){
                    if(is_read)
                        awaiter->fres.yieldResult(ValueItem((uint8_t*)buffer, fullifed_bytes), lock);
                    else 
                        awaiter->fres.yieldResult(fullifed_bytes, lock);
                }
                awaiter->fres.finalResult((uint8_t)e, lock);
            }
            awaiters.notify_all();
            delete this;
        }
        void readed(uint32_t len){
            if(is_read){
                fullifed_bytes += len;
                if(buffer_size > fullifed_bytes){
                    uint64_t new_offset = offset + fullifed_bytes;
                    overlapped.Offset = new_offset & 0xFFFFFFFF;
                    overlapped.OffsetHigh = (new_offset >> 32) & 0xFFFFFFFF;
                    if(!ReadFile(handle, buffer + fullifed_bytes, buffer_size - fullifed_bytes, nullptr, &overlapped)) error_filter();
                }else now_fullifed();
            }
        }
        void writen(uint32_t len){
            if(!is_read){
                fullifed_bytes += len;
                if(buffer_size > fullifed_bytes){
                    uint64_t new_offset = offset + fullifed_bytes;
                    overlapped.Offset = new_offset & 0xFFFFFFFF;
                    overlapped.OffsetHigh = (new_offset >> 32) & 0xFFFFFFFF;
                    if(!WriteFile(handle, buffer + fullifed_bytes, buffer_size - fullifed_bytes, nullptr, &overlapped)) error_filter();
                }
            }else now_fullifed();
        }
        void operation_fullifed(uint32_t len){
            if(buffer_size <= fullifed_bytes + len){
                now_fullifed();
                return;
            }
            if(is_read)
                readed(len);
            else
                writen(len);
        }
    
        void start(){
            if(is_read){
                if(!ReadFile(handle, buffer, buffer_size, NULL, &overlapped)){
                    auto err = GetLastError();
                    switch (err) {
                        case ERROR_IO_PENDING: return;
                        case ERROR_HANDLE_EOF: throw AException("FileException", "End of file");
                        case ERROR_NOT_ENOUGH_MEMORY: throw AException("FileException", "Not enough memory");
                        case ERROR_INVALID_USER_BUFFER: throw AException("FileException", "Invalid user buffer");
                        case ERROR_NOT_ENOUGH_QUOTA: throw AException("FileException", "Not enough quota");
                        default: throw AException("FileException", "Unknown error");
                    }
                }
            }
            else{
                if(!WriteFile(handle, buffer, buffer_size, NULL, &overlapped)){
                    auto err = GetLastError();
                    switch (err) {
                        case ERROR_IO_PENDING: return;
                        case ERROR_HANDLE_EOF: throw AException("FileException", "End of file");
                        case ERROR_NOT_ENOUGH_MEMORY: throw AException("FileException", "Not enough memory");
                        case ERROR_INVALID_USER_BUFFER: throw AException("FileException", "Invalid user buffer");
                        case ERROR_NOT_ENOUGH_QUOTA: throw AException("FileException", "Not enough quota");
                        default: throw AException("FileException", "Unknown error");
                    }
                }
            }
        }
    
        bool error_filter(){
            switch (GetLastError()) {
                case ERROR_IO_PENDING: return false;
                case ERROR_HANDLE_EOF: {
                    if(is_read && !required_full){
                        now_fullifed();
                        return false;
                    }else{
                        exception(io_errors::eof);
                        return true;
                    }
                }
                case ERROR_NOT_ENOUGH_MEMORY: exception(io_errors::no_enough_memory); return true;
                case ERROR_INVALID_USER_BUFFER: exception(io_errors::invalid_user_buffer); return true;
                case ERROR_NOT_ENOUGH_QUOTA: exception(io_errors::no_enough_quota); return true;
                default: exception(io_errors::unknown_error); return true;
            }
        }
    };
    void file_overlapped_on_await(ValueItem& it){
        ((File_*)(void*)it)->await();
    }
    void file_overlapped_on_cancel(ValueItem& it){
        ((File_*)(void*)it)->cancel();
    }


    class FileManager : public NativeWorkerManager {
        void* _handle = nullptr;
        uint64_t write_pointer;
        uint64_t read_pointer;
        pointer_mode pointer_mode;
        friend class File_;
        
        uint64_t _file_size(){
            uint64_t size = 0;
            FILE_STANDARD_INFO finfo = {0};
            if(GetFileInformationByHandleEx(_handle, FileStandardInfo, &finfo, sizeof(finfo))){
                if(finfo.EndOfFile.QuadPart > 0)
                    size = finfo.EndOfFile.QuadPart;
                else
                    size = -1;
            }else
                size = -1;
            return size;
        }
    public:
        FileManager(const char* path, size_t path_len, open_mode open, on_open_action action, share_mode share, _sync_flags flags, enum class pointer_mode pointer_mode) noexcept(false) : pointer_mode(pointer_mode) {
            read_pointer = 0;
            write_pointer = 0;
            std::u16string wpath;
            utf8::utf8to16(path, path + path_len, std::back_inserter(wpath));
            DWORD wshare_mode = 0;
            if(share.read)
                wshare_mode |= FILE_SHARE_READ;
            if(share.write)
                wshare_mode |= FILE_SHARE_WRITE;
            if(share._delete)
                wshare_mode |= FILE_SHARE_DELETE;
            
            DWORD wflags = FILE_FLAG_OVERLAPPED;
            if(flags.delete_on_close)
                wflags |= FILE_FLAG_DELETE_ON_CLOSE;
            if(flags.no_buffering)
                wflags |= FILE_FLAG_NO_BUFFERING;
            if(flags.posix_semantics)
                wflags |= FILE_FLAG_POSIX_SEMANTICS;
            if(flags.random_access)
                wflags |= FILE_FLAG_RANDOM_ACCESS;
            if(flags.sequential_scan)
                wflags |= FILE_FLAG_SEQUENTIAL_SCAN;
            if(flags.write_through)
                wflags |= FILE_FLAG_WRITE_THROUGH;

            
            DWORD wopen = 0;
            switch(open){
                case open_mode::read:
                    wopen = GENERIC_READ;
                    break;
                case open_mode::write:
                    wopen = GENERIC_WRITE;
                    break;
                case open_mode::append:
                    wopen = FILE_APPEND_DATA;
                    break;
                case open_mode::read_write:
                    wopen = GENERIC_READ | GENERIC_WRITE;
                    break;
                default:
                    throw InvalidArguments("Invalid open mode, excepted read, write, read_write or append, but got " + std::to_string((int)open));
            }
            DWORD creation_mode = 0;
            switch (action) {
                case on_open_action::open:
                    creation_mode = OPEN_ALWAYS;
                    break;
                case on_open_action::always_new:
                    creation_mode = CREATE_ALWAYS;
                    break;
                case on_open_action::create_new:
                    creation_mode = CREATE_NEW;
                    break;
                case on_open_action::open_exists:
                    creation_mode = OPEN_EXISTING;
                    break;
                case on_open_action::truncate_exists:
                    creation_mode = TRUNCATE_EXISTING;
                    break;
                default:
                    throw InvalidArguments("Invalid on open action, excepted open, always_new, create_new, open_exists or truncate_exists, but got " + std::to_string((int)open));
            }
            
            _handle = CreateFileW((wchar_t*)wpath.c_str(), wopen, wshare_mode, NULL, creation_mode , wflags, NULL);
            if(_handle == INVALID_HANDLE_VALUE){
                    _handle = nullptr;
                    switch(GetLastError()){
                        case ERROR_FILE_NOT_FOUND:
                            throw AException("FileException", "File not found");
                        case ERROR_ACCESS_DENIED:
                            throw AException("FileException", "Access denied");
                        case ERROR_FILE_EXISTS:
                        case ERROR_ALREADY_EXISTS:
                            throw AException("FileException", "File exists");
                        case ERROR_FILE_INVALID:
                            throw AException("FileException", "File invalid");
                        case ERROR_FILE_TOO_LARGE:
                            throw AException("FileException", "File too large");
                        case ERROR_INVALID_PARAMETER:
                            throw AException("FileException", "Invalid parameter");
                        case ERROR_SHARING_VIOLATION:
                            throw AException("FileException", "Sharing violation");
                        default:
                            throw AException("FileException", "Unknown error");
                    }
                }
            NativeWorkersSingleton::register_handle(_handle, this);
        }
        ~FileManager(){
            if(_handle != nullptr)
                CloseHandle(_handle);
        }
        ValueItem read(uint32_t size, bool require_all = true){
            File_* file = new File_(this, _handle, size, read_pointer, require_all);
            switch(pointer_mode){
                case pointer_mode::seprated:
                    read_pointer += size;
                    break;
                case pointer_mode::combined:
                    write_pointer = read_pointer = read_pointer + size;
                    break;
            }
            ValueItem args((void*)file);
            try{
                file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr);
                file->start();
            }catch(...){
                delete file;
                throw;
            }
            return file->awaiter;
        }
        ValueItem read(uint8_t* data, uint32_t size, bool require_all = true) {
            File_* file = new File_(this, _handle, size, read_pointer, require_all);
            switch(pointer_mode){
                case pointer_mode::seprated:
                    read_pointer += size;
                    break;
                case pointer_mode::combined:
                    write_pointer = read_pointer = read_pointer + size;
                    break;
            }
            ValueItem args((void*)file);
            try{
                file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr);
                file->start();
                auto res = Task::get_result(file->awaiter);
                if(res->meta.vtype == VType::ui8) {io_error_to_exception((io_errors)(uint8_t)(size_t)res->val); return nullptr;}
                else if (res->meta.vtype == VType::raw_arr_ui8){
                    auto arr = (uint8_t*)res->getSourcePtr();
                    uint32_t len = res->meta.val_len;
                    memcpy(data, arr, len);
                    delete res;
                    return len; 
                }
                else if(res->meta.vtype == VType::noting){
                    delete res;
                    return 0;
                }
                else{
                    delete res;
                    throw InternalException("Caught invalid value type, excepted raw_arr_ui8, noting or except_value, but got " + std::to_string((int)res->meta.vtype));
                }
            }catch(...){
                delete file;
                throw;
            }
        }
        
        ValueItem write(uint8_t* data, uint32_t size) {
            File_* file = new File_(this, _handle, (char*)data, size, write_pointer);
            switch(pointer_mode){
                case pointer_mode::seprated:
                    write_pointer += size;
                    break;
                case pointer_mode::combined:
                    write_pointer = read_pointer = write_pointer + size;
                    break;
            }
            ValueItem args((void*)file);
            try{
                file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr);
                file->start();
            }catch(...){
                delete file;
                throw;
            }
            return file->awaiter;
        }
        ValueItem append(uint8_t* data, uint32_t size) {
            File_* file = new File_(this, _handle, (char*)data, size, (uint64_t)-1);
            ValueItem args((void*)file);
            try{
                file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr);
                file->start();
            }catch(...){
                delete file;
                throw;
            }
            return file->awaiter;
        }
        ValueItem seek_pos(uint64_t offset, pointer_offset pointer_offset, pointer pointer){
            switch (pointer_offset) {
            case pointer_offset::begin:
                switch(pointer_mode){
                    case pointer_mode::seprated:
                       switch(pointer){
                            case pointer::read:
                                read_pointer = offset;
                                break;
                            case pointer::write:
                                write_pointer = offset;
                                break;
                        }
                        break;
                    case pointer_mode::combined:
                        read_pointer = write_pointer = offset;
                        break;
                }
                break;
            case pointer_offset::current:
                switch(pointer_mode){
                    case pointer_mode::seprated:
                        switch(pointer){
                            case pointer::read:
                                read_pointer += offset;
                                break;
                            case pointer::write:
                                write_pointer += offset;
                                break;
                        }
                        break;
                    case pointer_mode::combined:
                        read_pointer = write_pointer += offset;
                        break;
                }
                break;
            case pointer_offset::end:{
                auto size = _file_size();
                if(size != -1){
                        switch(pointer_mode){
                            case pointer_mode::seprated:
                                switch(pointer){
                                    case pointer::read:
                                        read_pointer = size + offset;
                                        break;
                                    case pointer::write:
                                        write_pointer = size + offset;
                                        break;
                                }
                                break;
                            case pointer_mode::combined:
                                read_pointer = write_pointer = size + offset;
                                break;
                        }
                    }
                else return false;
                break;
            }
            default:
                break;
            }
            return true;
        }
        ValueItem seek_pos(uint64_t offset, pointer_offset pointer_offset) {
            switch (pointer_offset) {
            case pointer_offset::begin:
                read_pointer = write_pointer = offset;
                break;
            case pointer_offset::current:
                read_pointer = write_pointer += offset;
                break;
            case pointer_offset::end:{
                auto size = _file_size();
                if(size != -1)
                    read_pointer = write_pointer = size + offset;
                else return false;
                break;
            }
            default:
                break;
            }
            return true;
        }
        ValueItem tell_pos(pointer pointer){
            switch (pointer) {
            case pointer::read:
                return read_pointer;
            case pointer::write:
                return write_pointer;
            default:
                return nullptr;
            }
        }
        
        ValueItem flush(){
            return (bool)FlushFileBuffers(_handle);
        }
        ValueItem file_size(){
            auto res = _file_size();
            if(res == -1)
                return nullptr;
            else
                return res;
        }
        void handle(void* data, class NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status){
            auto file = (File_*)overlapped;
            if(!status)
                file->error_filter();
            else
                file->operation_fullifed(dwBytesTransferred);
        }
        void* get_handle(){
            return _handle;
        }
    };
    

    FileHandle::FileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _async_flags flags, share_mode share, pointer_mode pointer_mode) noexcept(false){
        _sync_flags sync_flags;
        sync_flags.delete_on_close = flags.delete_on_close;
        sync_flags.posix_semantics = flags.posix_semantics;
        sync_flags.random_access = flags.random_access;
        sync_flags.sequential_scan = flags.sequential_scan;
        try{
            handle = new FileManager(path, path_len, open, action, share, sync_flags, pointer_mode);
        }catch(...){
            delete handle;
            throw;
        }
    }
    FileHandle::FileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _sync_flags flags, share_mode share, pointer_mode pointer_mode) noexcept(false){
        try{
            handle = new FileManager(path, path_len, open, action, share, flags, pointer_mode);
            mimic_non_async.make_construct();
        }catch(...){
            delete handle;
            throw;
        }
    }
    FileHandle::~FileHandle(){
        delete handle;
    }
    ValueItem FileHandle::read(uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            ValueItem res = handle->read(size, false);
            res.getAsync();
            if(res.meta.vtype == VType::ui8){ io_error_to_exception((io_errors)(uint8_t)(size_t)res.val); return nullptr; }
            else return res;
        }else return handle->read(size, false);
    }
    uint32_t FileHandle::read(uint8_t* data, uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return (uint32_t)handle->read(data, size, false);
        }else return (uint32_t)handle->read(data, size, false);
    }
    ValueItem FileHandle::read_fixed(uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            ValueItem res = handle->read(size, true);
            res.getAsync();
            if(res.meta.vtype == VType::ui8) { io_error_to_exception((io_errors)(uint8_t)(size_t)res.val); return nullptr; }
            else return res;
        }else return handle->read(size, true);
    }
    uint32_t FileHandle::read_fixed(uint8_t* data, uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return (uint32_t)handle->read(data, size, true);
        }else return (uint32_t)handle->read(data, size, true);
    }
    ValueItem FileHandle::write(uint8_t* data, uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            ValueItem res = handle->write(data, size);
            res.getAsync();
            if(res.meta.vtype == VType::ui8) { io_error_to_exception((io_errors)(uint8_t)(size_t)res.val); return nullptr; }
            else return res;
        }else return handle->write(data, size);
    }
    ValueItem FileHandle::append(uint8_t* data, uint32_t size){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            ValueItem res = handle->append(data, size);
            res.getAsync();
            if(res.meta.vtype == VType::except_value)
                std::rethrow_exception(*(std::exception_ptr*)res.getSourcePtr());
            else return res;
        }else return handle->append(data, size);
    }
    ValueItem FileHandle::seek_pos(uint64_t offset, pointer_offset pointer_offset, pointer pointer){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return handle->seek_pos(offset, pointer_offset, pointer);
        }else return handle->seek_pos(offset, pointer_offset, pointer);
    }
    ValueItem FileHandle::seek_pos(uint64_t offset, pointer_offset pointer_offset){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return handle->seek_pos(offset, pointer_offset);
        }else return handle->seek_pos(offset, pointer_offset);
    
    }
    ValueItem FileHandle::tell_pos(pointer pointer){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return handle->tell_pos(pointer);
        }else return handle->tell_pos(pointer);
    }
    ValueItem FileHandle::flush(){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return handle->flush();
        }else return handle->flush();
    }
    ValueItem FileHandle::size(){
        if(mimic_non_async.has_value()){
            std::lock_guard<TaskMutex> lock(*mimic_non_async);
            return handle->file_size();
        }else return handle->file_size();
    }//return noting if failed to get size, otherwise return integer size
    void* FileHandle::internal_get_handle() const noexcept{
        return handle->get_handle();
    }

    ValueItem remove(const char* path, size_t length){
        std::u16string wpath(u"\\\\?\\");
        utf8::utf8to16(path, path + length, std::back_inserter(wpath));
        return (bool)DeleteFileW((wchar_t*)wpath.c_str());
    }

    BlockingFileHandle::BlockingFileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _sync_flags flags, share_mode share) noexcept(false) : open(open), flags(flags){
        std::u16string wpath;
        utf8::utf8to16(path, path + path_len, std::back_inserter(wpath));
        DWORD wshare_mode = 0;
        if(share.read)
            wshare_mode |= FILE_SHARE_READ;
        if(share.write)
            wshare_mode |= FILE_SHARE_WRITE;
        if(share._delete)
            wshare_mode |= FILE_SHARE_DELETE;
        
        DWORD wflags = 0;
        if(flags.delete_on_close)
            wflags |= FILE_FLAG_DELETE_ON_CLOSE;
        if(flags.no_buffering)
            wflags |= FILE_FLAG_NO_BUFFERING;
        if(flags.posix_semantics)
            wflags |= FILE_FLAG_POSIX_SEMANTICS;
        if(flags.random_access)
            wflags |= FILE_FLAG_RANDOM_ACCESS;
        if(flags.sequential_scan)
            wflags |= FILE_FLAG_SEQUENTIAL_SCAN;
        if(flags.write_through)
            wflags |= FILE_FLAG_WRITE_THROUGH;
        
        DWORD wopen = 0;
        switch(open){
            case open_mode::read:
                wopen = GENERIC_READ;
                break;
            case open_mode::write:
                wopen = GENERIC_WRITE;
                break;
            case open_mode::append:
                wopen = FILE_APPEND_DATA;
                break;
            case open_mode::read_write:
                wopen = GENERIC_READ | GENERIC_WRITE;
                break;
            default:
                throw InvalidArguments("Invalid open mode, excepted read, write, read_write or append, but got " + std::to_string((int)open));
        }
        DWORD creation_mode = 0;
        switch (action) {
            case on_open_action::open:
                creation_mode = OPEN_ALWAYS;
                break;
            case on_open_action::always_new:
                creation_mode = CREATE_ALWAYS;
                break;
            case on_open_action::create_new:
                creation_mode = CREATE_NEW;
                break;
            case on_open_action::open_exists:
                creation_mode = OPEN_EXISTING;
                break;
            case on_open_action::truncate_exists:
                creation_mode = TRUNCATE_EXISTING;
                break;
            default:
                throw InvalidArguments("Invalid on open action, excepted open, always_new, create_new, open_exists or truncate_exists, but got " + std::to_string((int)open));
        }
        
        handle = CreateFileW((wchar_t*)wpath.c_str(), wopen, wshare_mode, NULL, creation_mode , wflags, NULL);
        if(handle == INVALID_HANDLE_VALUE){
            handle = nullptr;
            switch(GetLastError()){
                case ERROR_FILE_NOT_FOUND:
                    throw AException("FileException", "File not found");
                case ERROR_ACCESS_DENIED:
                    throw AException("FileException", "Access denied");
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                    throw AException("FileException", "File exists");
                case ERROR_FILE_INVALID:
                    throw AException("FileException", "File invalid");
                case ERROR_FILE_TOO_LARGE:
                    throw AException("FileException", "File too large");
                case ERROR_INVALID_PARAMETER:
                    throw AException("FileException", "Invalid parameter");
                case ERROR_SHARING_VIOLATION:
                    throw AException("FileException", "Sharing violation");
                default:
                    throw AException("FileException", "Unknown error");
            }
        }
    }
    BlockingFileHandle::~BlockingFileHandle(){
        CloseHandle(handle);
    }
    int64_t BlockingFileHandle::read(uint8_t* data, uint32_t size){
        std::lock_guard<TaskMutex> lock(mutex);
        DWORD readed;
        if(!ReadFile(handle, data, size, &readed, NULL)){
            switch(GetLastError()){
                case ERROR_HANDLE_EOF:{
                    eof = true;
                    return readed;
                }
                case ERROR_INVALID_USER_BUFFER:
                    throw AException("FileException", "Invalid user buffer");
                case ERROR_NOT_ENOUGH_MEMORY:
                    throw AException("FileException", "Not enough memory");
                case ERROR_OPERATION_ABORTED:
                    throw AException("FileException", "Operation aborted");
                case ERROR_INVALID_HANDLE:
                    throw AException("FileException", "Invalid handle");
                case ERROR_IO_DEVICE:
                    throw AException("FileException", "IO device error");
                case ERROR_IO_INCOMPLETE:
                    throw AException("FileException", "IO incomplete");
                case ERROR_IO_PENDING:
                    throw AException("FileException", "IO pending");
                case ERROR_NOACCESS:
                    throw AException("FileException", "No access");
                case ERROR_NOT_SUPPORTED:
                    throw AException("FileException", "Not supported");
                case ERROR_SEEK:
                    throw AException("FileException", "Seek error");
                case ERROR_SHARING_VIOLATION:
                    throw AException("FileException", "Sharing violation");
                case ERROR_WRITE_PROTECT:
                    throw AException("FileException", "Write protect");
                default:
                    throw AException("FileException", "Unknown error");
            }
        }
        return readed;
    }
    int64_t BlockingFileHandle::write(uint8_t* data, uint32_t size){
        std::lock_guard<TaskMutex> lock(mutex);
        DWORD written;
        if(!WriteFile(handle, data, size, &written, NULL)){
            switch(GetLastError()){
                case ERROR_INVALID_USER_BUFFER:
                    throw AException("FileException", "Invalid user buffer");
                case ERROR_NOT_ENOUGH_MEMORY:
                    throw AException("FileException", "Not enough memory");
                case ERROR_OPERATION_ABORTED:
                    throw AException("FileException", "Operation aborted");
                case ERROR_INVALID_HANDLE:
                    throw AException("FileException", "Invalid handle");
                case ERROR_IO_DEVICE:
                    throw AException("FileException", "IO device error");
                case ERROR_IO_INCOMPLETE:
                    throw AException("FileException", "IO incomplete");
                case ERROR_IO_PENDING:
                    throw AException("FileException", "IO pending");
                case ERROR_NOACCESS:
                    throw AException("FileException", "No access");
                case ERROR_NOT_SUPPORTED:
                    throw AException("FileException", "Not supported");
                case ERROR_SEEK:
                    throw AException("FileException", "Seek error");
                case ERROR_SHARING_VIOLATION:
                    throw AException("FileException", "Sharing violation");
                case ERROR_WRITE_PROTECT:
                    throw AException("FileException", "Write protect");
                default:
                    throw AException("FileException", "Unknown error");
            }
        }
        return written;
    }
    bool BlockingFileHandle::seek_pos(uint64_t offset, pointer_offset pointer_offset){
        std::lock_guard<TaskMutex> lock(mutex);
        LARGE_INTEGER li;
        li.QuadPart = offset;
        if(!SetFilePointerEx(handle, li, NULL, (DWORD)pointer_offset)){
            return false;
        }
        eof = false;
        return true;
    }
    uint64_t BlockingFileHandle::tell_pos(){
        std::lock_guard<TaskMutex> lock(mutex);
        LARGE_INTEGER li;
        li.QuadPart = 0;
        if(!SetFilePointerEx(handle, li, &li, FILE_CURRENT))
            return 0;
        return li.QuadPart;
    }
    bool BlockingFileHandle::flush(){
        std::lock_guard<TaskMutex> lock(mutex);
        return FlushFileBuffers(handle);
    }
    uint64_t BlockingFileHandle::size(){
        std::lock_guard<TaskMutex> lock(mutex);
        LARGE_INTEGER li;
        if(!GetFileSizeEx(handle, &li))
            return 0;
        return li.QuadPart;
    }
    void* BlockingFileHandle::internal_get_handle() const noexcept{
        return handle;
    }
    bool BlockingFileHandle::eof_state() const noexcept{
        return eof;
    }
    bool BlockingFileHandle::valid() const noexcept{
        return handle != nullptr;
    }
#if CONFIGURATION_COMPATIBILITY_ENABLE_FSTREAM_FROM_BLOCKINGFILEHANDLE
    ::std::fstream BlockingFileHandle::get_fstream() const {
        if (handle != INVALID_HANDLE_VALUE) {
            int file_descriptor = _open_osfhandle((intptr_t)handle, 0);
            if (file_descriptor != -1) {
                std::string mode;
                switch (open) {
                case open_mode::read:
                    mode = "r";
                    break;
                case open_mode::write:
                    mode = "w";
                    break;
                case open_mode::read_write:
                    mode = "r+";
                    break;
                case open_mode::append:
                    mode = "a";
                    break;
                }
                if(flags.delete_on_close)
                    mode += "D";
                if(flags.random_access)
                    mode += "R";
                if(flags.sequential_scan)
                    mode += "S";
                if(flags.write_through)
                    mode += "T";

                FILE* file = _fdopen(file_descriptor, mode.c_str());
                if (file != NULL) 
                    return std::fstream(file);
                else {
                    _close(file_descriptor);
                }
            }
        }
        throw AException("FileException", "Can't open file");
    }
#endif
}