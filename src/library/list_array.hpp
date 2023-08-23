#ifndef SRC_LIBRARY_LIST_ARRAY
#define SRC_LIBRARY_LIST_ARRAY
#include <util/hash.hpp>
#include "list_array/list_array.hpp"

namespace art{
    template<class T>
    struct hash<list_array<T>> {
        size_t operator()(const list_array<T>& t) const {
            art::hash<T> h;
            if(t.blocks_more(1)){
                size_t result = 0;
                for (size_t i = 0; i < t.size(); i++)
                    result = art::mur_combine(result, h(t[i]));
                return result;
            }else
                return h(t.data(), t.size());
        }
        size_t operator()(const list_array<T>* arr, size_t size) const {
            size_t result = 0;
            for (size_t i = 0; i < size; i++)
                result = art::mur_combine(result, operator()(arr[i]));
            return result;
        }
    };
}
#endif /* SRC_LIBRARY_LIST_ARRAY */
