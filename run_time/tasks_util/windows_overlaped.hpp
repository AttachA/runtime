// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <deque>
#include <thread>
#include <memory>
#include <sstream>
#include "../../run_time.hpp"

#include <Windows.h>

class OverlappedController{
    std::shared_ptr<void> m_hCompletionPort;
    using threads = std::deque<std::thread>;
    threads m_dispatcherThreads;
    std::atomic_size_t on_dispatch_count = 0;
public:

    OverlappedController(){
        m_hCompletionPort.reset(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0), CloseHandle);
        if(!m_hCompletionPort)
            throw std::runtime_error("CreateIoCompletionPort failed");
    }
    ~OverlappedController(){
        stop();
    }
    bool register_handle(HANDLE hFile, void* data){
        if(!CreateIoCompletionPort(hFile, m_hCompletionPort.get(), (ULONG_PTR)data, 0)){
            ValueItem notify{ "CreateIoCompletionPort failed with error ", (uint32_t)GetLastError() };
            errors.async_notify(notify);
            return false;
        }
        return true;
    }
    void run(size_t count = std::thread::hardware_concurrency()){
        if (m_dispatcherThreads.empty()) {
            const auto concurrentThreads = count;

            m_dispatcherThreads.resize(concurrentThreads);

            for (auto& dispatcherThread : m_dispatcherThreads)
                dispatcherThread = std::thread(&OverlappedController::dispatch, this);
        }
    }
    void stop(){
        //notify unmanaged and managed dispatchers to stop
        for(size_t i=0;i<on_dispatch_count;i++){
            if (0 == ::PostQueuedCompletionStatus(m_hCompletionPort.get(), 0, 0, 0)){
                ValueItem notify{ "PostQueuedCompletionStatus failed with error ", (uint32_t)GetLastError()};
                errors.async_notify(notify);
            }
        }

        for (auto& dispatcherThread : m_dispatcherThreads)
        {
            #ifndef DISABLE_RUNTIME_INFO
            std::stringstream ss; ss << "waiting for a thread " << dispatcherThread.get_id() << " to stop";
            ValueItem notify{ ss.str() };
            info.async_notify(notify);
            #endif
            dispatcherThread.join();
        }
        m_dispatcherThreads.clear();
    }
    bool in_run(){
        return on_dispatch_count > 0;
    }
    virtual void handle(void* data, void* overlapped, DWORD dwBytesTransferred, bool status) = 0;
    void dispatch() {
        on_dispatch_count++;
        while(true){
            DWORD dwBytesTransferred = 0;
            ULONG_PTR data = 0;
            LPOVERLAPPED lpOverlapped = nullptr;

            auto status = GetQueuedCompletionStatus(m_hCompletionPort.get(), &dwBytesTransferred, &data, &lpOverlapped, INFINITE);
            if(!status)
            {
                ValueItem notify{ "GetQueuedCompletionStatus failed with error ", (uint32_t)GetLastError(), m_hCompletionPort.get() };
                errors.async_notify(notify);
                continue;
            }
            
            if(NULL == data)
                break;

            if(!lpOverlapped)
            {
                ValueItem notify{ "GetQueuedCompletionStatus overlapped result is null" };
                errors.async_notify(notify);
                continue;
            }
            handle((void*)data, lpOverlapped, dwBytesTransferred, status);
        }
        on_dispatch_count--;
    }

    void set_dispatchers(size_t count){
        stop();
        run(count);
    }
};

