#ifndef SRC_RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON
#define SRC_RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON

#include <base/run_time.hpp>
#include <run_time/tasks/util/hill_climbing.hpp>
#include <util/threading.hpp>
#include <list>

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
namespace art{
    class NativeWorkerManager {
    public:
        virtual void handle(void* data, class NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) = 0;
        virtual ~NativeWorkerManager() = default;
    };
    class NativeWorkerHandle {
        friend class NativeWorkersSingleton;

        public: OVERLAPPED overlapped;
        private:NativeWorkerManager* manager;
    public: 
        NativeWorkerHandle(NativeWorkerManager* manager) : manager(manager) {}
        NativeWorkerHandle() = delete;
        NativeWorkerHandle(const NativeWorkerHandle&) = delete;
        NativeWorkerHandle(NativeWorkerHandle&&) = delete;
        NativeWorkerHandle& operator=(const NativeWorkerHandle&) = delete;
        NativeWorkerHandle& operator=(NativeWorkerHandle&&) = delete;
    };

    //not consume resources if not used
    class NativeWorkersSingleton {
        class CancellationNotify{};
        class CancellationManager : public NativeWorkerManager{
            public:
            virtual void handle(void* data, class NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) override {
                delete overlapped;
                throw CancellationNotify();
            }
            CancellationManager() = default;
            ~CancellationManager() = default;
        };
        class CancellationHandle : public NativeWorkerHandle{
            public:
            CancellationHandle(CancellationManager& ref) : NativeWorkerHandle(&ref) {}
        };

        static inline CancellationManager cancellation_manager_instance;
        static inline NativeWorkersSingleton* instance = nullptr;
        static inline art::mutex instance_mutex;
        art::shared_ptr<void> m_hCompletionPort;
        art::util::hill_climb hill_climb;
        art::mutex hill_climb_mutex;
        std::list<uint32_t> hill_climb_processed;


        NativeWorkersSingleton(){
            m_hCompletionPort.reset(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0), CloseHandle);
            if(!m_hCompletionPort)
                throw std::runtime_error("CreateIoCompletionPort failed");
            for(ptrdiff_t i = art::thread::hardware_concurrency()/3 + 1; i > 0; i--)
                art::thread(&NativeWorkersSingleton::dispatch, this).detach();
        }

        std::pair<uint32_t, uint32_t> proceed_hill_climb(double sample_seconds){
            const uint32_t max_threads = art::thread::hardware_concurrency();
            std::lock_guard<art::mutex> lock(hill_climb_mutex);
            uint32_t recommended_thread_count = 0;
            uint32_t recommended_sleep_count = 0;
            if(hill_climb_processed.empty()){
                art::thread(&NativeWorkersSingleton::dispatch, this).detach();
                return { 100, 1 };
            }
            for(auto& item : hill_climb_processed){
                auto [thread_count, sleep_count] = hill_climb.climb(hill_climb_processed.size(), sample_seconds, item, 1, max_threads);
                recommended_thread_count += thread_count;
                recommended_sleep_count += sleep_count;
                item = 0;
            }
            recommended_thread_count /= hill_climb_processed.size();
            recommended_sleep_count /= hill_climb_processed.size();

            ptrdiff_t diff = recommended_thread_count - hill_climb_processed.size();
            if(diff > 0){
                for(ptrdiff_t i = diff; i > 0; i--)
                    art::thread(&NativeWorkersSingleton::dispatch, this).detach();
            }else if(diff < 0){
                for(ptrdiff_t i = diff; i < 0; i++)
                    post_work(new CancellationHandle(cancellation_manager_instance), 0);
            }
            return { recommended_thread_count, 1 };
        }
        void dispatch() {
            if(enable_thread_naming)
                _set_name_thread_dbg("NativeWorker Dispatch");
            art::unique_lock<art::mutex> lock(hill_climb_mutex);
            uint32_t& item = hill_climb_processed.emplace_back();
            auto item_ptr = hill_climb_processed.begin();
            lock.unlock();
            while(true){
                DWORD dwBytesTransferred = 0;
                ULONG_PTR data = 0;
                NativeWorkerHandle* lpOverlapped = nullptr;

                auto status = GetQueuedCompletionStatus(m_hCompletionPort.get(), &dwBytesTransferred, &data, (OVERLAPPED**)&lpOverlapped, INFINITE);
                
                if(NULL == data){
                    ValueItem notify{ "GetQueuedCompletionStatus data is null" };
                    errors.async_notify(notify);
                    continue;
                }

                if(!lpOverlapped)
                {
                    ValueItem notify{ "GetQueuedCompletionStatus overlapped result is null" };
                    errors.async_notify(notify);
                    continue;
                }
                try{
                    lpOverlapped->manager->handle((void*)data, lpOverlapped, dwBytesTransferred, status);
                }catch(const CancellationNotify&){
                    break;
                }
                item++;
            }
            lock.lock();
            hill_climb_processed.erase(item_ptr);
        }
        bool _register_handle(HANDLE hFile, void* data){
            if(!CreateIoCompletionPort(hFile, m_hCompletionPort.get(), (ULONG_PTR)data, 0)){
                ValueItem notify{ "CreateIoCompletionPort failed with error ", (uint32_t)GetLastError() };
                errors.sync_notify(notify);
                return false;
            }
            return true;
        }
        static NativeWorkersSingleton& get_instance(){
            if(instance) return *instance;
            else {
                std::lock_guard<art::mutex> lock(instance_mutex);
                if(!instance)
                    instance = new NativeWorkersSingleton();
                return *instance;
            }
        }
    public:
        ~NativeWorkersSingleton() = default;
        static bool register_handle(HANDLE hFile, void* data){
            return get_instance()._register_handle(hFile, data);
        }
        static bool post_work(NativeWorkerHandle* overlapped, DWORD dwBytesTransferred = 0){
            return PostQueuedCompletionStatus(get_instance().m_hCompletionPort.get(), dwBytesTransferred, (ULONG_PTR)overlapped, (OVERLAPPED*)overlapped);
        }

        static std::pair<uint32_t, uint32_t> hill_climb_proceed(std::chrono::high_resolution_clock::duration sample_time){
            std::lock_guard<art::mutex> lock(instance_mutex);
            if(!instance)
                return { 0, 0 };
            else
                return instance->proceed_hill_climb(std::chrono::duration<double>(sample_time).count());
            
        }
    };

}
#else 
#include <liburing.h>
namespace art{
    class NativeWorkerManager {
    public:
        virtual void handle(class NativeWorkerHandle* overlapped, io_uring_cqe* cqe) = 0;
        virtual ~NativeWorkerManager() = default;
    };
    class NativeWorkerHandle {
        friend class NativeWorkersSingleton;
        private:NativeWorkerManager* manager;
    public: 
        NativeWorkerHandle(NativeWorkerManager* manager) : manager(manager) {}
        NativeWorkerHandle() = delete;
        NativeWorkerHandle(const NativeWorkerHandle&) = delete;
        NativeWorkerHandle(NativeWorkerHandle&&) = delete;
        NativeWorkerHandle& operator=(const NativeWorkerHandle&) = delete;
        NativeWorkerHandle& operator=(NativeWorkerHandle&&) = delete;
    };
    //not consume resources if not used
    class NativeWorkersSingleton {
        class CancellationNotify{};
        class CancellationManager : public NativeWorkerManager{
            public:
            virtual void handle(class NativeWorkerHandle* overlapped, io_uring_cqe* ) override {
                delete overlapped;
                throw CancellationNotify();
            }
            CancellationManager() = default;
            ~CancellationManager() = default;
        };
        class CancellationHandle : public NativeWorkerHandle{
            public:
            CancellationHandle(CancellationManager& ref) : NativeWorkerHandle(&ref) {}
        };

        static inline CancellationManager cancellation_manager_instance;
        static inline NativeWorkersSingleton* instance = nullptr;
        static inline art::mutex instance_mutex;
        io_uring m_ring;
        art::util::hill_climb hill_climb;
        art::mutex hill_climb_mutex;
        std::list<uint32_t> hill_climb_processed;


        NativeWorkersSingleton(){
            //init m_ring
            if(int res = io_uring_queue_init(2048, &m_ring, 0) < 0){
                ValueItem notify{ "io_uring_queue_init failed with error ", res };
                errors.sync_notify(notify);
                return;
            }
            for(ptrdiff_t i = art::thread::hardware_concurrency()/3 + 1; i > 0; i--)
                art::thread(&NativeWorkersSingleton::dispatch, this).detach();
        }

        std::pair<uint32_t, uint32_t> proceed_hill_climb(double sample_seconds){
            const uint32_t max_threads = art::thread::hardware_concurrency();
            std::lock_guard<art::mutex> lock(hill_climb_mutex);
            uint32_t recommended_thread_count = 0;
            uint32_t recommended_sleep_count = 0;
            if(hill_climb_processed.empty()){
                art::thread(&NativeWorkersSingleton::dispatch, this).detach();
                return { 100, 1 };
            }
            for(auto& item : hill_climb_processed){
                auto [thread_count, sleep_count] = hill_climb.climb(hill_climb_processed.size(), sample_seconds, item, 1, max_threads);
                recommended_thread_count += thread_count;
                recommended_sleep_count += sleep_count;
                item = 0;
            }
            recommended_thread_count /= hill_climb_processed.size();
            recommended_sleep_count /= hill_climb_processed.size();

            ptrdiff_t diff = recommended_thread_count - hill_climb_processed.size();
            if(diff > 0){
                for(ptrdiff_t i = diff; i > 0; i--)
                    art::thread(&NativeWorkersSingleton::dispatch, this).detach();
            }else if(diff < 0){
                for(ptrdiff_t i = diff; i < 0; i++)
                    post_yield(new CancellationHandle(cancellation_manager_instance));
            }
            return { recommended_thread_count, 1 };
        }
        void dispatch() {
            if(enable_thread_naming)
                _set_name_thread_dbg("NativeWorker Dispatch");
            art::unique_lock<art::mutex> lock(hill_climb_mutex);
            uint32_t& item = hill_climb_processed.emplace_back();
            auto item_ptr = hill_climb_processed.begin();
            lock.unlock();
            while(true){
                uint64_t handle = 0;
                io_uring_cqe* cqe = nullptr;
                //liburing wait to complete any work, io_uring m_ring
                int res = io_uring_wait_cqe(&m_ring, &cqe);
                if(res < 0){
                    ValueItem notify{ "io_uring_wait_cqe failed with error ", res };
                    errors.sync_notify(notify);
                }
                handle = cqe->user_data;
                NativeWorkerHandle* completion = reinterpret_cast<NativeWorkerHandle*>(handle);
                if(!completion) {
                    io_uring_cqe_seen(&m_ring, cqe);
                    ValueItem notify{ "io_uring_wait_cqe returned undefined completion, skip" };
                    warning.async_notify(notify);
                    continue;
                }
                if(completion->manager == &cancellation_manager_instance){
                    io_uring_cqe_seen(&m_ring, cqe);
                    delete completion;
                    break;
                }
                if(!completion->manager){
                    io_uring_cqe_seen(&m_ring, cqe);
                    ValueItem notify{ "io_uring_wait_cqe returned completion with undefined manager, skip",  completion };
                    warning.async_notify(notify);
                    continue;
                }
                try{
                    completion->manager->handle(completion, cqe);
                }catch(CancellationNotify&){
                    io_uring_cqe_seen(&m_ring, cqe);
                    break;
                }catch(...){
                    io_uring_cqe_seen(&m_ring, cqe);
                    throw;
                }
                io_uring_cqe_seen(&m_ring, cqe);
                item++;
            }
            lock.lock();
            hill_climb_processed.erase(item_ptr);
        }
        static NativeWorkersSingleton& get_instance(){
            if(instance) return *instance;
            else {
                std::lock_guard<art::mutex> lock(instance_mutex);
                if(!instance)
                    instance = new NativeWorkersSingleton();
                return *instance;
            }
        }
        static void sumbmit(NativeWorkersSingleton& instance){
            if(int res = io_uring_submit(&instance.m_ring); res < 0){
                ValueItem notify{ "io_uring_submit failed with error ", res };
                errors.sync_notify(notify);
            }
        }
        class AwaitCancel : public NativeWorkerHandle, NativeWorkerManager {
            bool success = false;
            TaskMutex mutex;
            TaskConditionVariable awaiter;
        public:
            AwaitCancel() : NativeWorkerHandle(this) {}
            void handle(NativeWorkerHandle* self, io_uring_cqe* cqe) override {
                lock_guard<TaskMutex> lock(mutex);
                success = cqe->res >= 0;
                awaiter.notify_all();
            }
            bool await_fd(int handle){
                MutexUnify unify(mutex);
                unique_lock lock(unify);
                post_cancel_fd(this, handle);
                awaiter.wait(lock);
                return success;
            }
            bool await_fd_all(int handle){
                MutexUnify unify(mutex);
                unique_lock lock(unify);
                post_cancel_fd_all(this, handle);
                awaiter.wait(lock);
                return success;
            }
        };
    public:
        ~NativeWorkersSingleton(){
            io_uring_queue_exit(&m_ring);
        }
        static void post_readv(NativeWorkerHandle* handle, int hFile, const iovec* pVec, uint32_t nVec, uint64_t offset){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_readv(sqe, hFile, pVec, nVec, offset);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_readv2(NativeWorkerHandle* handle, int hFile, const iovec* pVec, uint32_t nVec, uint64_t offset, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_readv2(sqe, hFile, pVec, nVec, offset, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_writev(NativeWorkerHandle* handle, int hFile, const iovec* pVec, uint32_t nVec, uint64_t offset){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_writev(sqe, hFile, pVec, nVec, offset);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_writev2(NativeWorkerHandle* handle, int hFile, const iovec* pVec, uint32_t nVec, uint64_t offset, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_writev2(sqe, hFile, pVec, nVec, offset, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_read(NativeWorkerHandle* handle, int hFile, void* pBuffer, uint32_t nBuffer, uint64_t offset){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_read(sqe, hFile, pBuffer, nBuffer, offset);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_write(NativeWorkerHandle* handle, int hFile, const void* pBuffer, uint32_t nBuffer, uint64_t offset){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_write(sqe, hFile, pBuffer, nBuffer, offset);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_read_fixed(NativeWorkerHandle* handle, int hFile, void* pBuffer, uint32_t nBuffer, uint64_t offset, int32_t buf_index){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_read_fixed(sqe, hFile, pBuffer, nBuffer, offset, buf_index);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_write_fixed(NativeWorkerHandle* handle, int hFile, const void* pBuffer, uint32_t nBuffer, uint64_t offset, int32_t buf_index){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_write_fixed(sqe, hFile, pBuffer, nBuffer, offset, buf_index);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_fsync(NativeWorkerHandle* handle, int hFile, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_fsync(sqe, hFile, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_fsync_range(NativeWorkerHandle* handle, int hFile, uint64_t offset, uint64_t nbytes, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_rw(IORING_OP_SYNC_FILE_RANGE, sqe, hFile, nullptr, offset, nbytes); 
            sqe->sync_range_flags = flags;
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            if(int res = io_uring_submit(&instance.m_ring); res < 0){ 
                ValueItem notify{ "io_uring_submit failed with error ", res }; 
                errors.sync_notify(notify); 
            } 
        }
        static void post_recvmsg(NativeWorkerHandle* handle, int hSocket, msghdr* pMsg, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_recvmsg(sqe, hSocket, pMsg, flags); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            if(int res = io_uring_submit(&instance.m_ring); res < 0){ 
                ValueItem notify{ "io_uring_submit failed with error ", res }; 
                errors.sync_notify(notify); 
            } 
        }
        static void post_sendmsg(NativeWorkerHandle* handle, int hSocket, const msghdr* pMsg, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_sendmsg(sqe, hSocket, pMsg, flags); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            if(int res = io_uring_submit(&instance.m_ring); res < 0){ 
                ValueItem notify{ "io_uring_submit failed with error ", res }; 
                errors.sync_notify(notify); 
            } 
        }
        static void post_recv(NativeWorkerHandle* handle, int hSocket, void* pBuffer, uint32_t nBuffer, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_recv(sqe, hSocket, pBuffer, nBuffer, flags); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            sumbmit(instance);
        }
        static void post_recvfrom(NativeWorkerHandle* handle, int hSocket, const void* pBuffer, uint32_t nBuffer, int32_t flags, sockaddr* addr, socklen_t* addr_len){
            throw NotImplementedException();
        }
        static void post_send(NativeWorkerHandle* handle, int hSocket, const void* pBuffer, uint32_t nBuffer, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_send(sqe, hSocket, pBuffer, nBuffer, flags); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            sumbmit(instance);
        }
        static void post_sendto(NativeWorkerHandle* handle, int hSocket, const void* pBuffer, uint32_t nBuffer, int32_t flags, sockaddr* addr, socklen_t addr_len){
            throw NotImplementedException();
        }
        
        static void post_pool(NativeWorkerHandle* handle, int hSocket, short mask){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_poll_add(sqe, hSocket, mask);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_yield(NativeWorkerHandle* handle){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_nop(sqe);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_accept(NativeWorkerHandle* handle, int hSocket, sockaddr* pAddr, socklen_t* pAddrLen, int32_t flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_accept(sqe, hSocket, pAddr, pAddrLen, flags); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            sumbmit(instance);
        }
        static void post_connect(NativeWorkerHandle* handle, int hSocket, const sockaddr* pAddr, socklen_t addrLen){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring); 
            io_uring_prep_connect(sqe, hSocket, pAddr, addrLen); 
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle)); 
            sumbmit(instance);
        }
        static void post_shutdown(NativeWorkerHandle* handle, int hSocket, int how){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_shutdown(sqe, hSocket, how);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_close(NativeWorkerHandle* handle, int hSocket){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_close(sqe, hSocket);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_timeout(NativeWorkerHandle* handle, __kernel_timespec* pTimeSpec){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_timeout(sqe, pTimeSpec, 0, 0);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_openat(NativeWorkerHandle* handle, int hDir, const char* pPath, int flags, mode_t mode){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_openat(sqe, hDir, pPath, flags, mode);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_statx(NativeWorkerHandle* handle, int hDir, const char* pPath, int flags, unsigned int mask, struct statx* pStatxbuf){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_statx(sqe, hDir, pPath, flags, mask, pStatxbuf);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_splice(NativeWorkerHandle* handle, int hIn, loff_t pOffIn, int hOut, loff_t pOffOut, size_t nBytes, unsigned int flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_splice(sqe, hIn, pOffIn, hOut, pOffOut, nBytes, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_tee(NativeWorkerHandle* handle, int hIn, int hOut, size_t nBytes, unsigned int flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_tee(sqe, hIn, hOut, nBytes, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_renameat(NativeWorkerHandle* handle, int hOldDir, const char* pOldPath, int hNewDir, const char* pNewPath, unsigned int flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_renameat(sqe, hOldDir, pOldPath, hNewDir, pNewPath, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_mkdirat(NativeWorkerHandle* handle, int hDir, const char* pPath, mode_t mode){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_mkdirat(sqe, hDir, pPath, mode);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_symlinkat(NativeWorkerHandle* handle, const char* pPath, int hDir, const char* pLink){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_symlinkat(sqe, pPath, hDir, pLink);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_linkat(NativeWorkerHandle* handle, int hOldDir, const char* pOldPath, int hNewDir, const char* pNewPath, int flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_linkat(sqe, hOldDir, pOldPath, hNewDir, pNewPath, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_unlinkat(NativeWorkerHandle* handle, int hDir, const char* pPath, int flags){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_unlinkat(sqe, hDir, pPath, flags);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_fallocate(NativeWorkerHandle* handle, int hFile, int mode, off_t pOffset, off_t nBytes){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_fallocate(sqe, hFile, mode, pOffset, nBytes);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }

        static void post_cancel(NativeWorkerHandle* handle){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_cancel(sqe, handle, 0);
            sumbmit(instance);
        }
        static void post_cancel_all(NativeWorkerHandle* handle){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_cancel(sqe, handle, IORING_ASYNC_CANCEL_ALL);
            sumbmit(instance);
        }
        static void post_cancel_fd(NativeWorkerHandle* handle, int hIn){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_cancel_fd(sqe, hIn, 0);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }
        static void post_cancel_fd_all(NativeWorkerHandle* handle, int hIn){
            auto& instance = get_instance();
            io_uring_sqe* sqe = io_uring_get_sqe(&instance.m_ring);
            io_uring_prep_cancel_fd(sqe, hIn, IORING_ASYNC_CANCEL_ALL);
            io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(handle));
            sumbmit(instance);
        }



        static bool await_cancel_fd(int hIn){
            AwaitCancel cancel;
            return cancel.await_fd(hIn);
        }
        static bool await_cancel_fd_all(int hIn){
            AwaitCancel cancel;
            return cancel.await_fd_all(hIn);
        }
        

        static std::pair<uint32_t, uint32_t> hill_climb_proceed(std::chrono::high_resolution_clock::duration sample_time){
            std::lock_guard<art::mutex> lock(instance_mutex);
            if(!instance)
                return { 0, 0 };
            else
                return instance->proceed_hill_climb(std::chrono::duration<double>(sample_time).count());
            
        }
    };
}
#endif 
#endif /* SRC_RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON */
