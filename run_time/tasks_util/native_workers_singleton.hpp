#ifndef RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON
#define RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON

#include <deque>
#include <thread>
#include <memory>
#include <sstream>
#include "../../run_time.hpp"

#ifdef _WIN64
#include <Windows.h>

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
    static inline NativeWorkersSingleton* instance = nullptr;
    static inline std::mutex instance_mutex;
    std::shared_ptr<void> m_hCompletionPort;
    
    NativeWorkersSingleton(){
        m_hCompletionPort.reset(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0), CloseHandle);
        if(!m_hCompletionPort)
            throw std::runtime_error("CreateIoCompletionPort failed");
        for(ptrdiff_t i = std::thread::hardware_concurrency(); i > 0; i--)
            std::thread(&NativeWorkersSingleton::dispatch, this).detach();
    }

    void dispatch() {
        while(true){
            DWORD dwBytesTransferred = 0;
            ULONG_PTR data = 0;
            NativeWorkerHandle* lpOverlapped = nullptr;

            auto status = GetQueuedCompletionStatus(m_hCompletionPort.get(), &dwBytesTransferred, &data, (OVERLAPPED**)&lpOverlapped, INFINITE);
            if(!status)
            {
                ValueItem notify{ "GetQueuedCompletionStatus failed with error ", (uint32_t)GetLastError(), m_hCompletionPort.get() };
                errors.async_notify(notify);
                continue;
            }
            
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
            lpOverlapped->manager->handle((void*)data, lpOverlapped, dwBytesTransferred, status);
        }
    }
    bool _register_handle(HANDLE hFile, void* data){
        if(!CreateIoCompletionPort(hFile, m_hCompletionPort.get(), (ULONG_PTR)data, 0)){
            ValueItem notify{ "CreateIoCompletionPort failed with error ", (uint32_t)GetLastError() };
            errors.async_notify(notify);
            return false;
        }
        return true;
    }
    static NativeWorkersSingleton& get_instance(){
        if(instance) return *instance;
        else {
            std::lock_guard<std::mutex> lock(instance_mutex);
            if(!instance)
                instance = new NativeWorkersSingleton();
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

};

#else // DEBUG
#endif

#endif /* RUN_TIME_TASKS_UTIL_NATIVE_WORKERS_SINGLETON */
