#ifndef SRC_RUN_TIME_UTIL_SHARED_PTR
#define SRC_RUN_TIME_UTIL_SHARED_PTR
#include <memory>
namespace art{
    template<class T>
    struct shared_ptr : public std::shared_ptr<T>{
        shared_ptr() : std::shared_ptr<T>(){}
        shared_ptr(T* ptr) : std::shared_ptr<T>(ptr){}
        shared_ptr(std::nullptr_t) : std::shared_ptr<T>(nullptr){}
        shared_ptr(const shared_ptr<T>& ptr) : std::shared_ptr<T>(ptr){}
        shared_ptr(shared_ptr<T>&& ptr) : std::shared_ptr<T>(std::move(ptr)){}
        
        shared_ptr<T>& operator=(const shared_ptr<T>& ptr){
            this->std::shared_ptr<T>::operator=(ptr);
            return *this;
        }
        shared_ptr<T>& operator=(shared_ptr<T>&& ptr){
            this->std::shared_ptr<T>::operator=(std::move(ptr));
            return *this;
        }
        shared_ptr<T>& operator=(T* ptr) {
            this->std::shared_ptr<T>::operator=(std::shared_ptr<T>(ptr));
            return *this;
        }
        shared_ptr<T>& operator=(std::nullptr_t) {
            this->std::shared_ptr<T>::operator=(nullptr);
            return *this;
        }
    };
}

#endif /* SRC_RUN_TIME_UTIL_SHARED_PTR */
