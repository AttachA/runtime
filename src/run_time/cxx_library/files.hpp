#ifndef SRC_RUN_TIME_CXX_LIBRARY_FILES
#define SRC_RUN_TIME_CXX_LIBRARY_FILES
#include <run_time/tasks.hpp>
#include <util/in_place_optional.hpp>
#include <run_time/attacha_abi_structs.hpp>
#include <configuration/compatibility.hpp>
#if CONFIGURATION_COMPATIBILITY_ENABLE_FSTREAM_FROM_BLOCKINGFILEHANDLE
#include <fstream>
#endif
#if defined(_WIN32) || defined(_WIN64)
#define FILE_HANDLE void*
#else
#define FILE_HANDLE int
#endif
namespace art{
    namespace files {
        enum open_mode : uint8_t {
            read,
            write,
            read_write,
            append
        };
        union share_mode {//used in windows
            struct{
                bool read : 1;
                bool write : 1;
                bool _delete : 1;
            };
            uint8_t value = 0;
            share_mode(bool read = true, bool write = true, bool _delete = false):read(read), write(write), _delete(_delete){}
        };
        enum class pointer : uint8_t {
            read,
            write
        };
        enum class pointer_offset : uint8_t{
            begin,
            current,
            end
        };
        enum class pointer_mode : uint8_t{
            separated,
            combined
        };
        enum class on_open_action : uint8_t{
            open,
            always_new,
            create_new,
            open_exists,
            truncate_exists
        };
        union _async_flags{
            struct {
                bool delete_on_close : 1;
                bool posix_semantics : 1;//used in windows
                bool random_access : 1;//hint to cache manager
                bool sequential_scan : 1;//hint to cache manager
            };
            uint8_t value;
        };
        union _sync_flags{
            struct  {
                bool delete_on_close : 1;
                bool posix_semantics : 1;//used in windows
                bool random_access : 1;//hint to cache manager
                bool sequential_scan : 1;//hint to cache manager
                bool no_buffering : 1;//hint to cache manager, affect seek and read write operations, like disc page size aligned operations
                bool write_through : 1;//hint to cache manager
            };
            uint8_t value;
        };

        enum class io_errors : uint8_t {
            no_error,
            eof,
            no_enough_memory,
            invalid_user_buffer,
            no_enough_quota,
            unknown_error
        };

        class FileHandle {
            class FileManager* handle;
            i_p_optional<TaskMutex> mimic_non_async;
        public:
            FileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _async_flags flags, share_mode share = {}, pointer_mode pointer_mode = pointer_mode::combined) noexcept(false);
            FileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _sync_flags flags, share_mode share = {}, pointer_mode pointer_mode = pointer_mode::combined) noexcept(false);
            ~FileHandle();

            ValueItem read(uint32_t size);
            uint32_t read(uint8_t* data, uint32_t size);
            
            ValueItem read_fixed(uint32_t size);
            uint32_t read_fixed(uint8_t* data, uint32_t size);

            ValueItem write(uint8_t* data, uint32_t size);
            ValueItem append(uint8_t* data, uint32_t size);

            ValueItem seek_pos(uint64_t offset, pointer_offset pointer_offset, pointer pointer);
            ValueItem seek_pos(uint64_t offset, pointer_offset pointer_offset);

            ValueItem tell_pos(pointer pointer);

            ValueItem flush();

            ValueItem size();

            FILE_HANDLE internal_get_handle() const noexcept;
        };
        
        class BlockingFileHandle { //block workers threads, one pointer per handle
            TaskMutex mutex;
            FILE_HANDLE handle{0};
            bool eof = false;
            _sync_flags flags;
            open_mode open;
            #if defined(__linux__) || defined(_LINUX_) || defined(__linux) || defined(__gnu_linux__)
            art::ustring _path;
            #endif
        public:
            BlockingFileHandle(const char* path, size_t path_len, open_mode open, on_open_action action, _sync_flags flags, share_mode share = {}) noexcept(false);
            ~BlockingFileHandle();
            int64_t read(uint8_t* data, uint32_t size);
            int64_t write(uint8_t* data, uint32_t size);
            bool seek_pos(uint64_t offset, pointer_offset pointer_offset);
            uint64_t tell_pos();
            bool flush();
            uint64_t size();
            bool eof_state() const noexcept;
            bool valid() const noexcept;
            FILE_HANDLE internal_get_handle() const noexcept;

    #if CONFIGURATION_COMPATIBILITY_ENABLE_FSTREAM_FROM_BLOCKINGFILEHANDLE
            ::std::iostream get_iostream() const;//not mapped to proxy definition, can be used only in native c++ code
    #endif
            //require explicit close, destructor will not close it,
            // BlockingFileHandle will be not used when ::std::fstream alive,
            // can cause desync, ie thread unsafe
        };

        class FolderBrowser{
            class FolderBrowserImpl* impl;
            FolderBrowser(class FolderBrowserImpl* impl) noexcept;
        public:
            //set as /, in windows it will be emulate folder with names like C:, D:, E: and so on
            FolderBrowser();
            FolderBrowser(const char* path, size_t length) noexcept(false);
            FolderBrowser(const FolderBrowser& copy);
            FolderBrowser(FolderBrowser&& move);
            ~FolderBrowser();
            list_array<art::ustring> folders();
            list_array<art::ustring> files();
        
            bool is_folder();
            bool is_file();
        
            bool exists();
            bool is_hidden();
        
            bool create_path(const char* path, size_t length);
            bool create_current_path();
        
            bool create_file(const char* file_name, size_t length);
            bool create_folder(const char* folder_name, size_t length);
        
            bool remove_file(const char* file_name, size_t length);
            bool remove_folder(const char* folder_name, size_t length);
            bool remove_current_path();

            bool rename_file(const char* old_name, size_t old_length, const char* new_name, size_t new_length);
            bool rename_folder(const char* old_name, size_t old_length, const char* new_name, size_t new_length);

        
            typed_lgr<FolderBrowser> join_folder(const char* folder_name, size_t length);
            art::ustring get_current_path();

            bool is_corrupted();
        };

        ValueItem createFolderChangesMonitor(const char* path, size_t length, bool depth);


        ValueItem remove(const char* path, size_t length);
        ValueItem rename(const char* path, size_t length, const char* new_path, size_t new_length);
        ValueItem copy(const char* path, size_t length, const char* new_path, size_t new_length);
    }
}
#undef FILE_HANDLE
#endif /* SRC_RUN_TIME_CXX_LIBRARY_FILES */
