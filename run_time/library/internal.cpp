#include "../tasks_util/light_stack.hpp"
#include "../CASM.hpp"

namespace internal {
    namespace memory{
        //returns farr[farr[ptr from, ptr to, len, str desk, bool is_fault]...], arg: array/value ptr
        ValueItem* dump(ValueItem* vals, uint32_t len){
            if(len == 1)
                return light_stack::dump(vals[0].getSourcePtr());
            throw InvalidArguments("This function requires 1 argument");
        }
    }
    namespace stack {
        //reduce stack size, returns bool, args: shrink treeshold(optional)
        ValueItem* shrink(ValueItem* vals, uint32_t len){
            if(len == 1)
                return new ValueItem(light_stack::shrink_current((size_t)vals[0]));
            else if(len == 0)
                return new ValueItem(light_stack::shrink_current());
            throw InvalidArguments("This function requires 0 or 1 argument");
        }
        //grow stack size, returns bool, args: grow count
        ValueItem* prepare(ValueItem* vals, uint32_t len);
        //make sure stack size is enough and increase if too small, returns bool, args: grow count
        ValueItem* reserve(ValueItem* vals, uint32_t len){
            if(len == 1)
                return new ValueItem(light_stack::prepare((size_t)vals[0]));
            throw InvalidArguments("This function requires 1 argument");
        }


        //returns farr[farr[ptr from, ptr to, str desk, bool is_fault]...], args: none
        ValueItem* dump(ValueItem* vals, uint32_t len){
            if(len == 0)
                return light_stack::dump_current();
            throw InvalidArguments("This function requires 0 argument");
        }

        ValueItem* bs_supported(ValueItem* vals, uint32_t len){
            return new ValueItem(light_stack::is_supported());
        }


        ValueItem* used_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::used_size());
        }
        ValueItem* unused_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::unused_size());
        }
        ValueItem* allocated_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::allocated_size());
        }
        ValueItem* free_size(ValueItem*, uint32_t){
            return new ValueItem(light_stack::free_size());
        }


        //returns stack trace, args: none
        ValueItem* trace(ValueItem* vals, uint32_t len){
            throw NotImplementedException();
        }
    }
}