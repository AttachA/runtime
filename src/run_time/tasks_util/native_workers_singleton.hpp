#ifndef RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON
#define RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON

#include "../threading.hpp"
#include "hill_climbing.hpp"
#include <list>
#include "../../run_time.hpp"

#ifdef _WIN64
#include <Windows.h>
#endif
namespace art{
#ifdef _WIN64
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
        class CancelationNotify{};
        class CancelationManager : public NativeWorkerManager{
            public:
            virtual void handle(void* data, class NativeWorkerHandle* overlapped, unsigned long dwBytesTransferred, bool status) override {
                delete overlapped;
                throw CancelationNotify();
            }
            CancelationManager() = default;
            ~CancelationManager() = default;
        };
        class CancelationHandle : public NativeWorkerHandle{
            public:
            CancelationHandle(CancelationManager& ref) : NativeWorkerHandle(&ref) {}
        };

        static inline CancelationManager cancelation_manager_instance;
        static inline NativeWorkersSingleton* instance = nullptr;
        static inline art::mutex instance_mutex;
        std::shared_ptr<void> m_hCompletionPort;
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
            uint32_t recomended_thread_count = 0;
            uint32_t recomended_sleep_count = 0;
            if(hill_climb_processed.empty()){
                art::thread(&NativeWorkersSingleton::dispatch, this).detach();
                return { 100, 1 };
            }
            for(auto& item : hill_climb_processed){
                auto [thread_count, sleep_count] = hill_climb.climb(hill_climb_processed.size(), sample_seconds, item, 1, max_threads);
                recomended_thread_count += thread_count;
                recomended_sleep_count += sleep_count;
                item = 0;
            }
            recomended_thread_count /= hill_climb_processed.size();
            recomended_sleep_count /= hill_climb_processed.size();

            ptrdiff_t diff = recomended_thread_count - hill_climb_processed.size();
            if(diff > 0){
                for(ptrdiff_t i = diff; i > 0; i--)
                    art::thread(&NativeWorkersSingleton::dispatch, this).detach();
            }else if(diff < 0){
                for(ptrdiff_t i = diff; i < 0; i++)
                    post_work(new CancelationHandle(cancelation_manager_instance), 0);
            }
            return { recomended_thread_count, 1 };
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
                }catch(const CancelationNotify&){
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

        static std::pair<uint32_t, uint32_t> hill_climb_proceed(std::chrono::steady_clock::duration sample_time){
            std::lock_guard<art::mutex> lock(instance_mutex);
            if(!instance)
                return { 0, 0 };
            else
                return instance->proceed_hill_climb(std::chrono::duration<double>(sample_time).count());
            
        }
    };

#else // DEBUG
#endif

}
#endif /* RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON */
