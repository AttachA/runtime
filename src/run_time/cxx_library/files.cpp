#include "files.hpp"
#include "../exceptions.hpp"
#include "../tasks_util/native_workers_singleton.hpp"
#include <Windows.h>
#include <winternl.h>
#include <io.h>
#include "../../../configuration/compatibility.hpp"
#if CONFIGURATION_COMPATIBILITY_ENABLE_FSTREAM_FROM_BLOCKINGFILEHANDLE
#include <fstream>
#endif
#include <utf8cpp/utf8.h>
#include <filesystem>
#include "../AttachA_CXX.hpp"
namespace art{
    class IFolderChangesMonitor{
        virtual void start() noexcept(false) = 0;//start async scan
        virtual void lazy_start() noexcept(false) = 0;//start async scan
        virtual void once_scan() noexcept(false) = 0;//manual scan, blocking, return immediately if already scanning
        virtual void stop() noexcept(false) = 0;
        virtual ValueItem get_event_folder_name_change() const = 0;
        virtual ValueItem get_event_file_name_change() const = 0;
        virtual ValueItem get_event_folder_size_change() const = 0;
        virtual ValueItem get_event_file_size_change() const = 0;
        virtual ValueItem get_event_folder_creation() const = 0;
        virtual ValueItem get_event_file_creation() const = 0;
        virtual ValueItem get_event_folder_removed() const = 0;
        virtual ValueItem get_event_file_removed() const = 0;
        virtual ValueItem get_event_watcher_shutdown() const = 0;
    };












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
                    art::unique_lock<MutexUnify> lock(unify);
                    fullifed = true;
                    if(awaiter){
                        if(is_read && !required_full)
                            awaiter->fres.finalResult(ValueItem(), lock);
                        else
                            awaiter->fres.finalResult(ValueItem(), lock);
                    }
                    awaiters.notify_all();
                    awaiter = nullptr;
                }
            }
            void await(){
                MutexUnify unify(mutex);
                art::unique_lock<MutexUnify> lock(unify);
                while(!fullifed)
                    awaiters.wait(lock);
            }
            void now_fullifed(){
                MutexUnify unify(mutex);
                art::unique_lock<MutexUnify> lock(unify);
                fullifed = true;
                if(awaiter) {
                    if(is_read)
                        awaiter->fres.finalResult(ValueItem((uint8_t*)buffer, buffer_size), lock);
                    else 
                        awaiter->fres.finalResult(fullifed_bytes, lock);
                }
                awaiters.notify_all();
                awaiter = nullptr;
            }
            void exception(io_errors e){
                MutexUnify unify(mutex);
                art::unique_lock<MutexUnify> lock(unify);
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
                awaiter = nullptr;
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
        void file_overlapped_on_destruct(ValueItem& it) {
            if(((File_*)(void*)it)->awaiter)
                ((File_*)(void*)it)->cancel();
            delete (File_*)(void*)it;
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
                    case pointer_mode::separated:
                        read_pointer += size;
                        break;
                    case pointer_mode::combined:
                        write_pointer = read_pointer = read_pointer + size;
                        break;
                }
                ValueItem args((void*)file);
                try{
                    file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr, file_overlapped_on_destruct);
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
                    case pointer_mode::separated:
                        read_pointer += size;
                        break;
                    case pointer_mode::combined:
                        write_pointer = read_pointer = read_pointer + size;
                        break;
                }
                ValueItem args((void*)file);
                try{
                    file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr, file_overlapped_on_destruct);
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
                    case pointer_mode::separated:
                        write_pointer += size;
                        break;
                    case pointer_mode::combined:
                        write_pointer = read_pointer = write_pointer + size;
                        break;
                }
                ValueItem args((void*)file);
                try{
                    file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr, file_overlapped_on_destruct);
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
                    file->awaiter = Task::callback_dummy(args, file_overlapped_on_await, file_overlapped_on_cancel,nullptr, file_overlapped_on_destruct);
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
                        case pointer_mode::separated:
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
                        case pointer_mode::separated:
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
                                case pointer_mode::separated:
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
            art::lock_guard<TaskMutex> lock(mutex);
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
            art::lock_guard<TaskMutex> lock(mutex);
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
            art::lock_guard<TaskMutex> lock(mutex);
            LARGE_INTEGER li;
            li.QuadPart = offset;
            if(!SetFilePointerEx(handle, li, NULL, (DWORD)pointer_offset)){
                return false;
            }
            eof = false;
            return true;
        }
        uint64_t BlockingFileHandle::tell_pos(){
            art::lock_guard<TaskMutex> lock(mutex);
            LARGE_INTEGER li;
            li.QuadPart = 0;
            if(!SetFilePointerEx(handle, li, &li, FILE_CURRENT))
                return 0;
            return li.QuadPart;
        }
        bool BlockingFileHandle::flush(){
            art::lock_guard<TaskMutex> lock(mutex);
            return FlushFileBuffers(handle);
        }
        uint64_t BlockingFileHandle::size(){
            art::lock_guard<TaskMutex> lock(mutex);
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

        

        class FolderBrowserImpl{
            std::wstring _path;
            static inline constexpr char separator = '/';
            static inline constexpr char native_separator = '\\';
            static char is_separator(char c){
                return c == '\\' || c == '/';
            }
            static char as_native_separator(char c){
                return is_separator(c) ? native_separator : c;
            }
            bool current_path_is_file(){
                DWORD attr = GetFileAttributesW(_path.c_str());
                if(attr == INVALID_FILE_ATTRIBUTES)
                    return false;
                return !(attr & FILE_ATTRIBUTE_DIRECTORY || attr & FILE_ATTRIBUTE_DEVICE);
            }
            bool current_path_is_folder(){
                DWORD attr = GetFileAttributesW(_path.c_str());
                if(attr == INVALID_FILE_ATTRIBUTES)
                    return false;
                return attr & FILE_ATTRIBUTE_DIRECTORY;
            }
            void reduce_to_folder(){
                if(current_path_is_folder())
                    return;
                size_t pos = _path.find_last_of(L"\\/");
                if(pos == std::wstring::npos)
                    _path.clear();
                else _path.resize(pos);
            }

            bool current_path_like_file(){
                size_t pos = _path.find_last_of(L"\\/.");
                if(pos == std::wstring::npos)
                    return false;
                return _path[pos] == L'.';
            }
            bool current_path_like_folder(){
                return !current_path_like_file();
            }
            template<class _FN>
            void iterate_current_path(_FN iterator){
                if(!current_path_is_folder())
                    return;
                std::wstring search_path = _path + L"\\*";
                if(search_path.length() > MAX_PATH){
                    for(auto& c : search_path)
                        if(c == L'/')
                            c = L'\\';
                }
                WIN32_FIND_DATAW fd;
                HANDLE hFind = ::FindFirstFileW(search_path.c_str(), &fd);
                if(hFind != INVALID_HANDLE_VALUE){
                    do{
                        if(!iterator(fd))
                            break;
                    }while(::FindNextFileW(hFind, &fd));
                    ::FindClose(hFind);
                }
            }
            void normalize_path() {
                std::wstring result;
                result.reserve(_path.length());
                uint8_t dot_count = 0;
                bool in_folder = false;
                for (auto& c : _path) {
                    if (c == L'.') {
                        ++dot_count;
                        continue;
                    }
                    if (dot_count == 2) {
                        if(in_folder) {
                            size_t pos = result.find_last_of(L"\\/.");
                            if (pos == std::wstring::npos)
                                result.resize(0);
                            else result.resize(pos);
                        }else
                            throw AException("FolderBrowserException", "Invalid path");
                    }
                    dot_count = 0;
                    if (is_separator(c)) {
                        in_folder = true;
                        c = L'\\';
                    }
                    result.push_back(c);
                }
                _path = std::move(result);
            }
        public:
            FolderBrowserImpl(const char* path, size_t length) noexcept(false){
                if(length == 0) {
                    return;
                }
                if(length == 1 && is_separator(path[0])) {
                    return;
                }
                if(length == 2 && is_separator(path[0]) && is_separator(path[1])) {
                    return;
                }
                else {
                    _path = L"\\\\?\\";
                    utf8::utf8to16(path, path + length, std::back_inserter(_path));
                    normalize_path();
                }
            }
            FolderBrowserImpl(const FolderBrowserImpl& copy){
                _path = copy._path;
            }
            FolderBrowserImpl(){}
            list_array<std::string> folders(){
                reduce_to_folder();
                list_array<std::string> result;
                if(_path.empty()){
                    DWORD drives = GetLogicalDrives();
                    for(int i = 0; i < 26; i++){
                        if (drives & (1 << i)) {
                            std::string disk("A:");
                            disk[0] += i;
                            result.push_back(disk);
                        }
                    }
                    return result;
                }
                iterate_current_path([&result](const WIN32_FIND_DATAW& fd){
                    if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                        if(!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
                            return false;
                        std::string utf8;
                        utf8::utf16to8(fd.cFileName, fd.cFileName + wcslen(fd.cFileName), std::back_inserter(utf8));
                        for(auto& c : utf8)
                            if(c == native_separator)
                                c = separator;
                        result.push_back(utf8);
                    }
                    return true;
                });
                return result;
            }
            list_array<std::string> files(){
                reduce_to_folder();
                list_array<std::string> result;
                iterate_current_path([&result](const WIN32_FIND_DATAW& fd){
                    if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || fd.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)){
                        std::string utf8;
                        utf8::utf16to8(fd.cFileName, fd.cFileName + wcslen(fd.cFileName), std::back_inserter(utf8));
                        for(auto& c : utf8)
                            if(c == native_separator)
                                c = separator;
                        result.push_back(utf8);
                    }
                    return true;
                });
                return result;
            }
        
            bool is_folder(){
                return current_path_is_folder();
            }
            bool is_file(){
                return current_path_is_file();
            }
        
            bool exists(){
                DWORD attr = GetFileAttributesW(_path.c_str());
                return attr != INVALID_FILE_ATTRIBUTES;
            }
            bool is_hidden(){
                DWORD attr = GetFileAttributesW(_path.c_str());
                if(attr == INVALID_FILE_ATTRIBUTES)
                    return false;
                return attr & FILE_ATTRIBUTE_HIDDEN;
            }
        
            bool create_path(const char* path, size_t length){
                if(length == 0)
                    return false;
                if(length == 1 && is_separator(path[0]))
                    return false;
                if(length == 2 && is_separator(path[0]) && is_separator(path[1]))
                    return false;
                std::wstring wpath;
                utf8::utf8to16(path, path + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                if(!current_path_is_folder()) {
                    size_t pos = wpath.find_last_of(L"\\/");
                    if(pos == std::wstring::npos)
                        return false;
                    wpath = wpath.substr(0, pos);
                }
                return CreateDirectoryW(wpath.c_str(), NULL) != 0;
            }
            bool create_current_path(){
                if(_path.empty())
                    return false;
                if(_path == L"\\" || _path == L"/")
                    return false;
                if(_path == L"\\\\" || _path == L"//" || _path == L"\\/" || _path == L"/\\")
                    return false;
                if(_path.length() > MAX_PATH){
                    std::wstring wpath = L"\\\\?\\" + _path;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                    return CreateDirectoryW(wpath.c_str(), NULL) != 0;
                }
                return CreateDirectoryW(_path.c_str(), NULL) != 0;
            }
        
            bool create_file(const char* file_name, size_t length){
                if(length == 0)
                    return false;
                if(length == 1 && is_separator(file_name[0]))
                    return false;
                if(length == 2 && is_separator(file_name[0]) && is_separator(file_name[1]))
                    return false;
                std::wstring wpath = _path + L"\\";
                utf8::utf8to16(file_name, file_name + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                if(hFile == INVALID_HANDLE_VALUE){
                    if(GetLastError() == ERROR_FILE_EXISTS)
                        return false;
                    return true;
                }
                CloseHandle(hFile);
                return true;
            }
            bool create_folder(const char* folder_name, size_t length){
                if(length == 0)
                    return false;
                if(length == 1 && is_separator(folder_name[0]))
                    return false;
                if(length == 2 && is_separator(folder_name[0]) && is_separator(folder_name[1]))
                    return false;
                std::wstring wpath = _path + L"\\";
                utf8::utf8to16(folder_name, folder_name + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                return CreateDirectoryW(wpath.c_str(), NULL) != 0;
            }
        
            bool remove_file(const char* file_name, size_t length){
                if(length == 0)
                    return false;
                if(length == 1 && is_separator(file_name[0]))
                    return false;
                if(length == 2 && is_separator(file_name[0]) && is_separator(file_name[1]))
                    return false;
                std::wstring wpath = _path + L"\\";
                utf8::utf8to16(file_name, file_name + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                return DeleteFileW(wpath.c_str()) != 0;
            }
            bool remove_folder(const char* folder_name, size_t length){
                if(length == 0)
                    return false;
                if(length == 1 && is_separator(folder_name[0]))
                    return false;
                if(length == 2 && is_separator(folder_name[0]) && is_separator(folder_name[1]))
                    return false;
                std::wstring wpath = _path + L"\\";
                utf8::utf8to16(folder_name, folder_name + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                return RemoveDirectoryW(wpath.c_str()) != 0;
            }
            bool remove_current_path(){
                if(_path.empty())
                    return false;
                if(_path == L"\\" || _path == L"/")
                    return false;
                if(_path == L"\\\\" || _path == L"//" || _path == L"\\/" || _path == L"/\\")
                    return false;
                if(_path.length() > MAX_PATH){
                    std::wstring wpath = L"\\\\?\\" + _path;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                    return RemoveDirectoryW(wpath.c_str()) != 0;
                }
                return RemoveDirectoryW(_path.c_str()) != 0;
            }

            bool rename_file(const char* old_name, size_t old_length, const char* new_name, size_t new_length){
                if(old_length == 0 || new_length == 0)
                    return false;
                if(old_length == 1 && is_separator(old_name[0]))
                    return false;
                if(old_length == 2 && is_separator(old_name[0]) && is_separator(old_name[1]))
                    return false;
                if(new_length == 1 && is_separator(new_name[0]))
                    return false;
                if(new_length == 2 && is_separator(new_name[0]) && is_separator(new_name[1]))
                    return false;
                std::wstring wold = _path + L"\\";
                std::wstring wnew = _path + L"\\";
                utf8::utf8to16(old_name, old_name + old_length, std::back_inserter(wold));
                utf8::utf8to16(new_name, new_name + new_length, std::back_inserter(wnew));
                if(wold.length() > MAX_PATH){
                    wold = L"\\\\?\\" + wold;
                    for(auto& c : wold)
                        if(c == L'/')
                            c = L'\\';
                }
                if(wnew.length() > MAX_PATH){
                    wnew = L"\\\\?\\" + wnew;
                    for(auto& c : wnew)
                        if(c == L'/')
                            c = L'\\';
                }
                return MoveFileW(wold.c_str(), wnew.c_str()) != 0;
            }
            bool rename_folder(const char* old_name, size_t old_length, const char* new_name, size_t new_length){
                return rename_file(old_name, old_length, new_name, new_length);//in windows, MoveFileW can rename folders
            }

            FolderBrowserImpl* join_folder(const char* folder_name, size_t length){
                if(length == 0)
                    return new FolderBrowserImpl();
                if(length == 1 && (is_separator(folder_name[0]) || folder_name[0] == '.')){
                    return new FolderBrowserImpl(*this);
                }
                if(length == 2 && folder_name[0] == '.' && folder_name[1] == '.'){
                    if(_path.empty())
                        return new FolderBrowserImpl();
                    else{
                        size_t pos = _path.find_last_of(L"\\/");
                        if(pos == std::wstring::npos)
                            return new FolderBrowserImpl();
                        else{
                            auto res = new FolderBrowserImpl(*this);
                            res->_path.resize(pos);
                            std::wstring wpath = res->_path;
                            if(wpath.starts_with(L"\\\\?\\"))
                                wpath.erase(0, 4);
                            if(wpath.length() == 2)
                                res->_path = L"";
                            return res;
                        }
                    }
                }
                if(_path.empty()){
                    if(length == 1 && isupper(folder_name[0])){
                        DWORD drives = GetLogicalDrives();
                        if(drives & (1 << (folder_name[0] - 'A'))){
                            std::string path = folder_name + std::string(":\\");
                            return new FolderBrowserImpl(path.c_str(), path.length());
                        }
                    }
                }
                std::wstring wpath = _path + L"\\";
                utf8::utf8to16(folder_name, folder_name + length, std::back_inserter(wpath));
                if(wpath.length() > MAX_PATH){
                    if(!wpath.starts_with(L"\\\\?\\"))
                        wpath = L"\\\\?\\" + wpath;
                    for(auto& c : wpath)
                        if(c == L'/')
                            c = L'\\';
                }
                auto res = new FolderBrowserImpl();
                res->_path = wpath;
                return res;
            }
            std::string get_current_path(){
                std::string res;
                utf8::utf16to8(_path.begin(), _path.end(), std::back_inserter(res));
                std::string_view pre_result = res;
                for (auto& c : res)
                    if (c == native_separator)
                        c = separator;
                if (pre_result.starts_with("\\\\?\\"))
                    pre_result.remove_prefix(4);
                if (pre_result.starts_with("Volume{")) {
                    std::string_view check_if_drive = pre_result;
                    check_if_drive.remove_prefix(7);
                    if (check_if_drive.length() < 36)
                        goto NOT_UUID_DRIVE;
                    if (check_if_drive[8] != '-' || check_if_drive[13] != '-' || check_if_drive[18] != '-' || check_if_drive[23] != '-')
                        goto NOT_UUID_DRIVE;
                    for (size_t i = 0; i < 36; ++i) {
                        if (i == 8 || i == 13 || i == 18 || i == 23)
                            continue;
                        if (!isxdigit(check_if_drive[i]))
                            goto NOT_UUID_DRIVE;
                    }
                    return res;
                }
            NOT_UUID_DRIVE:
                return (std::string)pre_result;
            }
        };

        struct FolderChangesMonitorHandle : public NativeWorkerHandle{
            uint8_t buffer[1024*4];
            FolderChangesMonitorHandle(NativeWorkerManager* manager) : NativeWorkerHandle(manager) {
                SecureZeroMemory(&overlapped, sizeof(overlapped));
                //array buffer will be filled with FILE_NOTIFY_EXTENDED_INFORMATION structures, no need to zero it
            }
        };
        class FolderChangesMonitorImpl : public NativeWorkerManager, public IFolderChangesMonitor{
            struct file_state{
                FILE_NOTIFY_EXTENDED_INFORMATION* current = nullptr;
                std::string full_path;//utf8
            };
            std::wstring _path;
            HANDLE _directory;
            bool depth_scan;
            bool _is_running = false;
            std::unordered_map<long long, file_state> states;


            typed_lgr<EventSystem> _folder_name_change = new EventSystem;
            typed_lgr<EventSystem> _folder_creation = new EventSystem;
            typed_lgr<EventSystem> _folder_removed = new EventSystem;
            typed_lgr<EventSystem> _folder_last_access = new EventSystem;
            typed_lgr<EventSystem> _folder_last_write = new EventSystem;
            typed_lgr<EventSystem> _folder_security_change = new EventSystem;
            typed_lgr<EventSystem> _folder_size_change = new EventSystem;
            typed_lgr<EventSystem> _folder_attributes = new EventSystem;


            typed_lgr<EventSystem> _file_name_change = new EventSystem;
            typed_lgr<EventSystem> _file_creation = new EventSystem;
            typed_lgr<EventSystem> _file_removed = new EventSystem;
            typed_lgr<EventSystem> _file_last_write = new EventSystem;
            typed_lgr<EventSystem> _file_last_access = new EventSystem;
            typed_lgr<EventSystem> _file_security_change = new EventSystem;
            typed_lgr<EventSystem> _file_size_change = new EventSystem;
            typed_lgr<EventSystem> _file_attributes = new EventSystem;

            typed_lgr<EventSystem> watcher_shutdown = new EventSystem;
            bool createHandle(FolderChangesMonitorHandle* handle){
                return ReadDirectoryChangesExW(
                    _directory,
                    handle->buffer, 
                    sizeof(handle->buffer), 
                    depth_scan, 
                    FILE_NOTIFY_CHANGE_FILE_NAME 
                    | FILE_NOTIFY_CHANGE_DIR_NAME 
                    | FILE_NOTIFY_CHANGE_ATTRIBUTES 
                    | FILE_NOTIFY_CHANGE_SIZE 
                    | FILE_NOTIFY_CHANGE_LAST_WRITE 
                    | FILE_NOTIFY_CHANGE_LAST_ACCESS 
                    | FILE_NOTIFY_CHANGE_CREATION 
                    | FILE_NOTIFY_CHANGE_SECURITY
                    ,
                    nullptr,
                    &handle->overlapped,
                    nullptr,
                    ReadDirectoryNotifyExtendedInformation
                );
            }
            enum class action_type{
                file_name_change,
                file_creation,
                file_removed,
                file_last_write,
                file_last_access,
                file_security_change,
                file_size_change,
                file_attributes,

                folder_name_change,
                folder_creation,
                folder_removed,
                folder_last_access,
                folder_last_write,
                folder_security_change,
                folder_size_change,
                folder_attributes
            };
            
            void manually_iterate(const std::wstring& _path, list_array<int64_t>& ids){
                WIN32_FIND_DATAW fd;
                HANDLE hFind;
                {
                    std::wstring path = _path + L"\\*";
                    hFind = ::FindFirstFileW(path.c_str(), &fd);
                }
                if(hFind != INVALID_HANDLE_VALUE){
                    do{
                        if (fd.cFileName[0] == '.' && fd.cFileName[1] == '\0')
                            continue;
                        if (fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == '\0')
                            continue;
                        std::wstring file_name = _path + L"\\"+ fd.cFileName;
                        BY_HANDLE_FILE_INFORMATION info;
                        auto file = CreateFileW(file_name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                        if(file != INVALID_HANDLE_VALUE){
                            if(GetFileInformationByHandle(file, &info)){
                                LARGE_INTEGER id;
                                LARGE_INTEGER file_size;
                                LARGE_INTEGER last_write;
                                LARGE_INTEGER last_access;
                                LARGE_INTEGER creation_time;
                                id.HighPart = info.nFileIndexHigh;
                                id.LowPart = info.nFileIndexLow;
                                file_size.HighPart = info.nFileSizeHigh;
                                file_size.LowPart = info.nFileSizeLow;
                                last_write.HighPart = info.ftLastWriteTime.dwHighDateTime;
                                last_write.LowPart = info.ftLastWriteTime.dwLowDateTime;
                                last_access.HighPart = info.ftLastAccessTime.dwHighDateTime;
                                last_access.LowPart = info.ftLastAccessTime.dwLowDateTime;
                                creation_time.HighPart = info.ftCreationTime.dwHighDateTime;
                                creation_time.LowPart = info.ftCreationTime.dwLowDateTime;
                                ids.push_back(id.QuadPart);
                                std::string name;
                                utf8::utf16to8(file_name.begin(), file_name.end(), std::back_inserter(name));
                                auto it = states.find(id.QuadPart);
                                DWORD action = 0;
                                if(it == states.end()){
                                    states[id.QuadPart] = {};
                                    it = states.find(id.QuadPart);
                                    ValueItem args{name};
                                    if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                        _folder_creation->async_notify(args);
                                    else
                                        _file_creation->async_notify(args);
                                    action = FILE_ACTION_ADDED;
                                }
                                else if((it->second.current->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
                                    {
                                        ValueItem args{name, (uint32_t)info.dwFileAttributes};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_removed->async_notify(args);
                                        else
                                            _file_removed->async_notify(args);
                                    }
                                    {
                                        ValueItem args{name, (uint32_t)it->second.current->FileAttributes};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_creation->async_notify(args);
                                        else
                                            _file_creation->async_notify(args);
                                    }
                                    action = FILE_ACTION_RENAMED_NEW_NAME;
                                }
                                else {
                                    if(it->second.current->FileAttributes != info.dwFileAttributes){
                                        ValueItem args{name, (uint32_t)info.dwFileAttributes};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_attributes->async_notify(args);
                                        else
                                            _file_attributes->async_notify(args);
                                        action = FILE_ACTION_MODIFIED;
                                    }
                                    if(it->second.current->FileSize.QuadPart != file_size.QuadPart){
                                        ValueItem args{name, (long long)file_size.QuadPart};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_size_change->async_notify(args);
                                        else
                                            _file_size_change->async_notify(args);
                                        action = FILE_ACTION_MODIFIED;
                                    }
                                    if(it->second.current->LastAccessTime.QuadPart != last_access.QuadPart){
                                        ValueItem args{name, (long long)last_access.QuadPart};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_last_access->async_notify(args);
                                        else
                                            _file_last_access->async_notify(args);
                                        action = FILE_ACTION_MODIFIED;
                                    }
                                    if(it->second.current->LastChangeTime.QuadPart != last_write.QuadPart){
                                        ValueItem args{name, (long long)last_write.QuadPart};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_creation->async_notify(args);
                                        else
                                            _file_creation->async_notify(args);
                                        action = FILE_ACTION_MODIFIED;
                                    }
                                    bool name_change = false;
                                    size_t wname_len = wcslen(it->second.current->FileName);
                                    if(it->second.current->FileNameLength / sizeof(wchar_t) != wname_len){
                                        name_change = true;
                                    }else{
                                        for(size_t i = 0; i < wname_len; ++i){
                                            if(it->second.current->FileName[i] != fd.cFileName[i]){
                                                name_change = true;
                                                break;
                                            }
                                        }
                                    }
                                    if(name_change){
                                        ValueItem args{name, fd.cFileName};
                                        if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                            _folder_name_change->async_notify(args);
                                        else
                                            _file_name_change->async_notify(args);
                                        action = FILE_ACTION_RENAMED_NEW_NAME;
                                    }
                                }
                                auto old = it->second.current;
                                int file_len = wcslen(fd.cFileName);
                                file_len -= 0;
                                it->second.current = (FILE_NOTIFY_EXTENDED_INFORMATION*)malloc(sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + file_len * sizeof(wchar_t));

                                if(!it->second.current){
                                    it->second.current = old;
                                    CloseHandle(file);
                                    continue;
                                } 
                                it->second.current->FileAttributes = info.dwFileAttributes;
                                it->second.current->FileSize = file_size;
                                it->second.current->LastAccessTime = last_access;
                                it->second.current->LastModificationTime = last_write;
                                it->second.current->LastChangeTime = last_write;
                                it->second.current->FileNameLength = file_len * sizeof(wchar_t);
                                for(int i = 0; i < file_len; i++)
                                    it->second.current->FileName[i] = fd.cFileName[i];
                                it->second.current->NextEntryOffset = 0;
                                it->second.current->Action = action;
                                if(old)
                                    free(old);
                                it->second.full_path = name;
                            }
                            CloseHandle(file);
                            if(depth_scan)
                                if(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                    manually_iterate(file_name, ids);
                        }
                    }while(::FindNextFileW(hFind, &fd));
                    ::FindClose(hFind);
                }
            }
            void manually_iterate(){
                list_array<int64_t> ids;
                manually_iterate(_path, ids);
                std::unordered_set<int64_t> _ids;
                _ids.reserve(ids.size());
                for(auto& id : ids)
                    _ids.insert(id);
                ids.clear();
                for(auto & it : states){
                    if(!_ids.contains(it.first)){
                        ValueItem args{ it.second.full_path, (uint32_t)it.second.current->FileAttributes};
                        if(it.second.current->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            _folder_removed->async_notify(args);
                        else
                            _file_removed->async_notify(args);
                        if(it.second.current)
                            free(it.second.current);
                        ids.push_back(it.first);
                    }
                }
                for(auto& id : ids)
                    states.erase(id);
            }

            void initialize() {
                _directory = CreateFileW(_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
                if (_directory == INVALID_HANDLE_VALUE)
                    throw AException("FolderChangesMonitorException", "Can't open directory");
                if (_path.ends_with(L'\\'))
                    _path.pop_back();
                else if (_path.ends_with(L'/'))
                    _path.pop_back();
                NativeWorkersSingleton::register_handle(_directory, this);
            }
        public:
            FolderChangesMonitorImpl(const std::wstring& path, bool depth_scan) noexcept(false) : depth_scan(depth_scan), _path(path){initialize();}
            FolderChangesMonitorImpl(std::wstring&& path, bool depth_scan) noexcept(false) : depth_scan(false), _path(path) {initialize();}
            ~FolderChangesMonitorImpl(){
                stop();
                if(_directory != INVALID_HANDLE_VALUE)
                    CloseHandle(_directory);
            }
            void handle(void* data, class NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) override {
                auto handle = (FolderChangesMonitorHandle*)overlapped;
                if(!status){
                    delete handle;
                    return;
                }
                FILE_NOTIFY_EXTENDED_INFORMATION* info = (FILE_NOTIFY_EXTENDED_INFORMATION*)handle->buffer;
                do{
                    std::wstring file_name = _path + L"\\";
                    file_name.insert(file_name.end(), info->FileName, info->FileName + info->FileNameLength / sizeof(wchar_t));
                    std::string name;
                    utf8::utf16to8(file_name.begin(), file_name.end(), std::back_inserter(name));
                    bool is_folder = info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                    switch(info->Action){
                        case FILE_ACTION_ADDED:{
                                ValueItem args{name};
                                auto it = states.find(info->FileId.QuadPart);
                                FILE_NOTIFY_EXTENDED_INFORMATION* allocated_info = (FILE_NOTIFY_EXTENDED_INFORMATION*)malloc(sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength* sizeof(wchar_t));
                                if(!allocated_info)
                                    break;
                                memcpy(allocated_info, info, sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                                if(it == states.end()){
                                    states[info->FileId.QuadPart].current = allocated_info;
                                    states[info->FileId.QuadPart].full_path = name;
                                    if(is_folder)
                                        _folder_creation->async_notify(args);
                                    else
                                        _file_creation->async_notify(args);
                                }else{
                                    states[info->FileId.QuadPart].current = allocated_info;
                                    states[info->FileId.QuadPart].full_path = name;
                                    if(is_folder)
                                        _folder_name_change->async_notify(args);
                                    else
                                        _file_name_change->async_notify(args);
                                }
                                break;
                            }
                        case FILE_ACTION_REMOVED:{
                                auto it = states.find(info->FileId.QuadPart);
                                if(it != states.end()){
                                    ValueItem args{name};
                                    if(is_folder)
                                        _folder_removed->async_notify(args);
                                    else
                                        _file_removed->async_notify(args);
                                    free(it->second.current);
                                    states.erase(it);
                                }
                                break;
                            }
                        case FILE_ACTION_MODIFIED:{
                                auto it = states.find(info->FileId.QuadPart);
                                if(it != states.end()){
                                    auto& old_info = it->second;
                                    if(old_info.current->LastAccessTime.QuadPart != info->LastAccessTime.QuadPart){
                                        ValueItem args{name, (long long)info->LastAccessTime.QuadPart};
                                        if(is_folder)
                                            _folder_last_access->async_notify(args);
                                        else
                                            _file_last_access->async_notify(args);
                                    }
                                    if(old_info.current->LastModificationTime.QuadPart != info->LastModificationTime.QuadPart){
                                        ValueItem args{name, (long long)info->LastModificationTime.QuadPart};
                                        if(is_folder)
                                            _folder_last_write->async_notify(args);
                                        else
                                            _file_last_write->async_notify(args);
                                    }
                                    if(old_info.current->FileAttributes != info->FileAttributes){
                                        ValueItem args{name, (uint32_t)info->FileAttributes};
                                        if(is_folder)
                                            _folder_attributes->async_notify(args);
                                        else
                                            _folder_attributes->async_notify(args);
                                    }
                                    if(old_info.current->FileSize.QuadPart != info->FileSize.QuadPart){
                                        ValueItem args{name, (long long)info->FileSize.QuadPart};
                                        if(is_folder)
                                            _folder_size_change->async_notify(args);
                                        else
                                            _folder_size_change->async_notify(args);
                                    }

                                }else{
                                    FILE_NOTIFY_EXTENDED_INFORMATION* allocated_info = (FILE_NOTIFY_EXTENDED_INFORMATION*)malloc(sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                                    if(!allocated_info)
                                        break;
                                    memcpy(allocated_info, info, sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                                    states[info->FileId.QuadPart].current = allocated_info;
                                    states[info->FileId.QuadPart].full_path = name;
                                }
                                break;
                            }
                        case FILE_ACTION_RENAMED_OLD_NAME:{
                            auto& old_info = states[info->FileId.QuadPart];
                            FILE_NOTIFY_EXTENDED_INFORMATION* allocated_info = (FILE_NOTIFY_EXTENDED_INFORMATION*)malloc(sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                            if(!allocated_info){
                                break;
                            }
                            if(old_info.current != nullptr)
                                free(old_info.current);
                            memcpy(allocated_info, info, sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                            old_info.current = allocated_info;
                            old_info.full_path = name;
                            break;
                        }
                        case FILE_ACTION_RENAMED_NEW_NAME:{
                            auto& old_info = states[info->FileId.QuadPart];
                            if(old_info.current != nullptr){
                                ValueItem args{ old_info.full_path, name};
                                if(info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                    _folder_name_change->async_notify(args);
                                else
                                    _file_name_change->async_notify(args);
                                FILE_NOTIFY_EXTENDED_INFORMATION* allocated_info = (FILE_NOTIFY_EXTENDED_INFORMATION*)malloc(sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                                if(!allocated_info){
                                    break;
                                }
                                if(old_info.current != nullptr)
                                    free(old_info.current);
                                memcpy(allocated_info, info, sizeof(FILE_NOTIFY_EXTENDED_INFORMATION) + info->FileNameLength);
                                old_info.current = allocated_info;
                                old_info.full_path = name;
                            }
                            break;
                        }
                        default:{
                            ValueItem args{"FolderChangesMonitor", "Unknown action"};
                            warning.async_notify(args);
                            break;
                        }
                    }
                    info = (FILE_NOTIFY_EXTENDED_INFORMATION*)((uint8_t*)info + info->NextEntryOffset);
                }while(info->NextEntryOffset != 0);
                if(createHandle(handle) == false){
                    if(GetLastError() == ERROR_NOTIFY_ENUM_DIR){
                        manually_iterate();
                        if(createHandle(handle) == true)
                            return;
                    }
                    _is_running = false;
                    delete handle;
                    ValueItem noting;
                    watcher_shutdown->async_notify(noting);
                }
            }
            void start() noexcept(false) {
                if (_is_running)
                    return;
                if (_directory == INVALID_HANDLE_VALUE)
                    throw AException("FolderChangesMonitorException", "Can't start monitor");
                manually_iterate();
                auto handle = new FolderChangesMonitorHandle(this);
                if (createHandle(handle) == false) {
                    delete handle;
                    throw AException("FolderChangesMonitorException", "Can't start monitor");
                }
                _is_running = true;
            }
            void lazy_start() noexcept(false) {
                if (_is_running)
                    return;
                if (_directory == INVALID_HANDLE_VALUE)
                    throw AException("FolderChangesMonitorException", "Can't start monitor");
                auto handle = new FolderChangesMonitorHandle(this);
                if (createHandle(handle) == false) {
                    delete handle;
                    throw AException("FolderChangesMonitorException", "Can't start monitor");
                }
                _is_running = true;
            }
            void once_scan() noexcept(false){
                if(_is_running)
                    return;
                manually_iterate();
            }
            void stop() noexcept(false){
                if(!_is_running)
                    return;
                if(_directory != INVALID_HANDLE_VALUE)
                    CancelIoEx(_directory, nullptr);
                _is_running = false;
                ValueItem noting;
                watcher_shutdown->async_notify(noting);
            }

            ValueItem get_event_folder_name_change() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_name_change), no_copy);
            }
            ValueItem get_event_file_name_change() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_name_change), no_copy);
            }
            ValueItem get_event_folder_size_change() const{
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_size_change), no_copy);
            }
            ValueItem get_event_file_size_change() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_size_change), no_copy);
            }
            ValueItem get_event_folder_creation() const{
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_creation), no_copy);
            }
            ValueItem get_event_file_creation() const{
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_creation), no_copy);
            }
            ValueItem get_event_folder_removed() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_removed), no_copy);
            }
            ValueItem get_event_file_removed() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_removed), no_copy);
            }
            ValueItem get_event_watcher_shutdown() const{
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), watcher_shutdown), no_copy);
            }

            ValueItem get_event_folder_last_access() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_last_access), no_copy);
            }
            ValueItem get_event_file_last_access() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_last_access), no_copy);
            }
            ValueItem get_event_folder_last_write() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_last_write), no_copy);
            }
            ValueItem get_event_file_last_write() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_last_write), no_copy);
            }
            ValueItem get_event_folder_security_change() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _folder_security_change), no_copy);
            }
            ValueItem get_event_file_security_change() const {
                return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _file_security_change), no_copy);
            }
        };


        AttachAVirtualTable* define_FolderChangesMonitor;
        AttachAFun(funs_FolderChangesMonitorImpl_start, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            self->start();
            })
        AttachAFun(funs_FolderChangesMonitorImpl_lazy_start, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            self->lazy_start();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_once_scan, 1,{
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            self->once_scan();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_stop, 1,{
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            self->stop();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_name_change, 1,{
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_folder_name_change();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_name_change, 1,{
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_file_name_change();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_size_change, 1,{
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_folder_size_change();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_size_change, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_file_size_change();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_creation, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_folder_creation();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_creation, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_file_creation();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_removed, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_folder_removed();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_removed, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_file_removed();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_watcher_shutdown, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor);
            return self->get_event_watcher_shutdown();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_last_access, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_folder_last_access();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_last_access, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_file_last_access();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_last_write, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_folder_last_write();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_last_write, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_file_last_write();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_folder_security_change, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_folder_security_change();
        })
        AttachAFun(funs_FolderChangesMonitorImpl_get_event_file_security_change, 1, {
            auto self = CXX::Interface::getExtractAs<typed_lgr<FolderChangesMonitorImpl>>(args[0], define_FolderChangesMonitor); 
            return self->get_event_file_security_change();
        })




        void init(){
            art::mutex m;
            std::lock_guard l(m);
            if(CXX::Interface::typeVTable<typed_lgr<FolderChangesMonitorImpl>>() != nullptr)
                return;
            define_FolderChangesMonitor = CXX::Interface::createTable<typed_lgr<FolderChangesMonitorImpl>>("folder_changes_monitor",
                CXX::Interface::direct_method("start", funs_FolderChangesMonitorImpl_start),
                CXX::Interface::direct_method("lazy_start", funs_FolderChangesMonitorImpl_lazy_start),
                CXX::Interface::direct_method("once_scan", funs_FolderChangesMonitorImpl_once_scan),
                CXX::Interface::direct_method("stop", funs_FolderChangesMonitorImpl_stop),
                CXX::Interface::direct_method("get_event_folder_name_change", funs_FolderChangesMonitorImpl_get_event_folder_name_change),
                CXX::Interface::direct_method("get_event_file_name_change", funs_FolderChangesMonitorImpl_get_event_file_name_change),
                CXX::Interface::direct_method("get_event_folder_size_change", funs_FolderChangesMonitorImpl_get_event_folder_size_change),
                CXX::Interface::direct_method("get_event_file_size_change", funs_FolderChangesMonitorImpl_get_event_file_size_change),
                CXX::Interface::direct_method("get_event_folder_creation", funs_FolderChangesMonitorImpl_get_event_folder_creation),
                CXX::Interface::direct_method("get_event_file_creation", funs_FolderChangesMonitorImpl_get_event_file_creation),
                CXX::Interface::direct_method("get_event_folder_removed", funs_FolderChangesMonitorImpl_get_event_folder_removed),
                CXX::Interface::direct_method("get_event_file_removed", funs_FolderChangesMonitorImpl_get_event_file_removed),
                CXX::Interface::direct_method("get_event_watcher_shutdown", funs_FolderChangesMonitorImpl_get_event_watcher_shutdown),
                CXX::Interface::direct_method("get_event_folder_last_access", funs_FolderChangesMonitorImpl_get_event_folder_last_access),
                CXX::Interface::direct_method("get_event_file_last_access", funs_FolderChangesMonitorImpl_get_event_file_last_access),
                CXX::Interface::direct_method("get_event_folder_last_write", funs_FolderChangesMonitorImpl_get_event_folder_last_write),
                CXX::Interface::direct_method("get_event_file_last_write", funs_FolderChangesMonitorImpl_get_event_file_last_write),
                CXX::Interface::direct_method("get_event_folder_security_change", funs_FolderChangesMonitorImpl_get_event_folder_security_change),
                CXX::Interface::direct_method("get_event_file_security_change", funs_FolderChangesMonitorImpl_get_event_file_security_change)
            );
            CXX::Interface::typeVTable<typed_lgr<FolderChangesMonitorImpl>>() = define_FolderChangesMonitor;
        }
        ValueItem createFolderChangesMonitor(const char* path, size_t length, bool depth){
            if(CXX::Interface::typeVTable<typed_lgr<EventSystem>>() == nullptr)
                throw MissingDependencyException("Parallel library with event_system is not loaded, required for folder_changes_monitor");
            if(CXX::Interface::typeVTable<typed_lgr<FolderChangesMonitorImpl>>() == nullptr)
                init();
            std::wstring wpath;
            utf8::utf8to16(path, path + length, std::back_inserter(wpath));
            return ValueItem(CXX::Interface::constructStructure<typed_lgr<FolderChangesMonitorImpl>>(define_FolderChangesMonitor, new FolderChangesMonitorImpl(std::move(wpath), depth)), no_copy);
        }
        
        ValueItem remove(const char* path, size_t length){
            std::wstring wpath(L"\\\\?\\");
            utf8::utf8to16(path, path + length, std::back_inserter(wpath));
            return (bool)DeleteFileW(wpath.c_str());
        }
        ValueItem rename(const char* path, size_t length, const char* new_path, size_t new_length){
            std::wstring wpath(L"\\\\?\\");
            utf8::utf8to16(path, path + length, std::back_inserter(wpath));
            std::wstring wnew_path(L"\\\\?\\");
            utf8::utf8to16(new_path, new_path + new_length, std::back_inserter(wnew_path));
            return (bool)MoveFileW(wpath.c_str(), wnew_path.c_str());
        }
        ValueItem copy(const char* path, size_t length, const char* new_path, size_t new_length){
            std::wstring wpath(L"\\\\?\\");
            utf8::utf8to16(path, path + length, std::back_inserter(wpath));
            std::wstring wnew_path(L"\\\\?\\");
            utf8::utf8to16(new_path, new_path + new_length, std::back_inserter(wnew_path));
            return (bool)CopyFileW(wpath.c_str(), wnew_path.c_str(), false);
        }















        FolderBrowser::FolderBrowser(FolderBrowserImpl* impl) noexcept {
            this->impl = impl;
        }
        FolderBrowser::FolderBrowser() {
            try {
                impl = new FolderBrowserImpl();
            }
            catch (...) {
                impl = nullptr;
                throw;
            }
        }
        FolderBrowser::FolderBrowser(const char* path, size_t length) noexcept(false) {
            try {
                impl = new FolderBrowserImpl(path, length);
            }
            catch (...) {
                impl = nullptr;
                throw;
            }
        }
        FolderBrowser::FolderBrowser(const FolderBrowser& copy) {
            try {
                if (copy.impl != nullptr)
                    impl = new FolderBrowserImpl(*copy.impl);
                else
                    impl = nullptr;
            }
            catch (...) {
                impl = nullptr;
                throw;
            }
        }
        FolderBrowser::FolderBrowser(FolderBrowser&& move) {
            impl = move.impl;
            move.impl = nullptr;
        }
        FolderBrowser::~FolderBrowser() {
            if (impl != nullptr)
                delete impl;
        }
        list_array<std::string> FolderBrowser::folders() {
            if (impl == nullptr)
                return false;
            return impl->folders();
        }
        list_array<std::string> FolderBrowser::files() {
            if (impl == nullptr)
                return false;
            return impl->files();
        }

        bool FolderBrowser::is_folder() {
            if (impl == nullptr)
                return false;
            return impl->is_folder();
        }
        bool FolderBrowser::is_file() {
            if (impl == nullptr)
                return false;
            return impl->is_file();
        }

        bool FolderBrowser::exists() {
            if (impl == nullptr)
                return false;
            return impl->exists();
        }
        bool FolderBrowser::is_hidden() {
            if (impl == nullptr)
                return false;
            return impl->is_hidden();
        }

        bool FolderBrowser::create_path(const char* path, size_t length) {
            if (impl == nullptr)
                return false;
            return impl->create_path(path, length);
        }
        bool FolderBrowser::create_current_path() {
            if (impl == nullptr)
                return false;
            return impl->create_current_path();
        }

        bool FolderBrowser::create_file(const char* file_name, size_t length) {
            if (impl == nullptr)
                return false;
            return impl->create_file(file_name, length);
        }
        bool FolderBrowser::create_folder(const char* folder_name, size_t length) {
            if (impl == nullptr)
                return false;
            return impl->create_folder(folder_name, length);
        }

        bool FolderBrowser::remove_file(const char* file_name, size_t length) {
            if (impl == nullptr)
                return false;
            return impl->remove_file(file_name, length);
        }
        bool FolderBrowser::remove_folder(const char* folder_name, size_t length) {
            if (impl == nullptr)
                return false;
            return impl->remove_folder(folder_name, length);
        }
        bool FolderBrowser::remove_current_path() {
            if (impl == nullptr)
                return false;
            return impl->remove_current_path();
        }
        bool FolderBrowser::rename_file(const char* old_name, size_t old_length, const char* new_name, size_t new_length) {
            if (impl == nullptr)
                return false;
            return impl->rename_file(old_name, old_length, new_name, new_length);
        }
        bool FolderBrowser::rename_folder(const char* old_name, size_t old_length, const char* new_name, size_t new_length) {
            if (impl == nullptr)
                return false;
            return impl->rename_folder(old_name, old_length, new_name, new_length);
        }

        typed_lgr<FolderBrowser> FolderBrowser::join_folder(const char* folder_name, size_t length) {
            if (impl == nullptr)
                throw AException("FolderBrowserException", "FolderBrowser is not initialized");
            return new FolderBrowser(impl->join_folder(folder_name, length));
        }
        std::string FolderBrowser::get_current_path() {
            if (impl == nullptr)
                return "";
            return impl->get_current_path();
        }
        bool FolderBrowser::is_corrupted() {
            return !impl;
        }
    }
}