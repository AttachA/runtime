
#include "bytes.hpp"
#include "../../library/string_help.hpp"
#include "../attacha_abi.hpp"
#include <utf8cpp/utf8.h>
namespace bytes{
    void convert_endian(Endian value_endian, ValueItem& value){
        value.getAsync();
        if (value_endian != Endian::native){
            switch (value.meta.vtype) {
                case VType::noting:
                case VType::boolean:
                case VType::i8:
                case VType::ui8:
                case VType::type_identifier:
                case VType::raw_arr_i8:
                case VType::raw_arr_ui8:
                case VType::string:
                    return;
                case VType::i16:
                case VType::ui16:
                    swap_bytes(*(uint16_t*)value.getSourcePtr());
                    return;
                case VType::i32:
                case VType::ui32:
                case VType::flo:
                    swap_bytes(*(uint32_t*)value.getSourcePtr());
                    return;
                case VType::i64:
                case VType::ui64:
                case VType::doub:
                case VType::undefined_ptr:
                case VType::time_point:
                    swap_bytes(*(uint64_t*)value.getSourcePtr());
                    return;
                case VType::raw_arr_i16:
                case VType::raw_arr_ui16:
                    swap_bytes(((uint16_t*)value.getSourcePtr()), value.meta.val_len);
                    return;
                case VType::raw_arr_i32:
                case VType::raw_arr_ui32:
                case VType::raw_arr_flo:
                    swap_bytes(((uint32_t*)value.getSourcePtr()), value.meta.val_len);
                    return;
                case VType::raw_arr_i64:
                case VType::raw_arr_ui64:
                case VType::raw_arr_doub:
                    swap_bytes(((uint64_t*)value.getSourcePtr()), value.meta.val_len);
                    return;
                case VType::uarr:
                    for(auto& i : *(list_array<ValueItem>*)value.getSourcePtr())
                        convert_endian(value_endian, i);
                    return;
                case VType::faarr:
                case VType::saarr: {
                    auto arr = (ValueItem*)value.getSourcePtr();
                    for (size_t i = 0; i < value.meta.val_len; i++)
                        convert_endian(value_endian, arr[i]);
                    return;
                }

                case VType::map:
                case VType::set:
                case VType::async_res:
                case VType::except_value:
                case VType::struct_:
                case VType::function:
                default:
                    throw InvalidOperation("Can't convert endian for this type: " + enum_to_string(value.meta.vtype));
            }
        }
    }
    
	ValueItem* current_endian(ValueItem* args, uint32_t len){
        return new ValueItem((uint8_t)Endian::native);
    }
	ValueItem* convert_endian(ValueItem* args, uint32_t len){
        if(len < 2)
            throw InvalidArguments("Invalid number of arguments for convert_endian");
        Endian endian;
        if(is_integer(args[1].meta.vtype))
            endian = (Endian)(uint8_t)args[1];
        else if (args[1].meta.vtype == VType::string){
            auto& str = *(std::string*)args[1].getSourcePtr();
            static const std::string big = "big";
            static const std::string little = "little";
            static const std::string native = "native";
            if(string_help::iequals(str, big))
                endian = Endian::big;
            else if(string_help::iequals(str, little))
                endian = Endian::little;
            else if(string_help::iequals(str, native))
                endian = Endian::native;
            else
                throw InvalidArguments("Invalid endian type: " + str);
        }
        else{
            auto str = (std::string)args[1];
            static const std::string big = "big";
            static const std::string little = "little";
            static const std::string native = "native";
            if(string_help::iequals(str, big))
                endian = Endian::big;
            else if(string_help::iequals(str, little))
                endian = Endian::little;
            else if(string_help::iequals(str, native))
                endian = Endian::native;
            else
                throw InvalidArguments("Invalid endian type: " + str);
        }
        convert_endian(endian, args[0]);
        return nullptr;
    }

    ValueItem* swap_bytes(ValueItem* args, uint32_t len){
        args[0].getAsync();
        switch (args[0].meta.vtype) {
            case VType::noting:
            case VType::boolean:
            case VType::i8:
            case VType::ui8:
            case VType::type_identifier:
            case VType::raw_arr_i8:
            case VType::raw_arr_ui8:
            case VType::string:
                return nullptr;
            case VType::i16:
            case VType::ui16:
                swap_bytes(*(uint16_t*)args[0].getSourcePtr());
                return nullptr;
            case VType::i32:
            case VType::ui32:
            case VType::flo:
                swap_bytes(*(uint32_t*)args[0].getSourcePtr());
                return nullptr;
            case VType::i64:
            case VType::ui64:
            case VType::doub:
            case VType::undefined_ptr:
            case VType::time_point:
                swap_bytes(*(uint64_t*)args[0].getSourcePtr());
                return nullptr;
            case VType::raw_arr_i16:
            case VType::raw_arr_ui16:
                swap_bytes(((uint16_t*)args[0].getSourcePtr()), args[0].meta.val_len);
                return nullptr;
            case VType::raw_arr_i32:
            case VType::raw_arr_ui32:
            case VType::raw_arr_flo:
                swap_bytes(((uint32_t*)args[0].getSourcePtr()), args[0].meta.val_len);
                return nullptr;
            case VType::raw_arr_i64:
            case VType::raw_arr_ui64:
            case VType::raw_arr_doub:
                swap_bytes(((uint64_t*)args[0].getSourcePtr()), args[0].meta.val_len);
                return nullptr;
            case VType::uarr:
                for(auto& i : *(list_array<ValueItem>*)args[0].getSourcePtr())
                    swap_bytes(&i, 1);
                return nullptr;
            case VType::faarr:
            case VType::saarr: {
                auto arr = (ValueItem*)args[0].getSourcePtr();
                for (size_t i = 0; i < args[0].meta.val_len; i++)
                    swap_bytes(&arr[i], 1);
                return nullptr;
            }
            case VType::map:
            case VType::set:
            case VType::async_res:
            case VType::except_value:
            case VType::struct_:
            case VType::function:
            default:
                throw InvalidOperation("Can't swap bytes for this type: " + enum_to_string(args[0].meta.vtype));
        }
    }
	ValueItem* from_bytes(ValueItem* args, uint32_t len){
        if(len < 2)
            throw InvalidArguments("Invalid number of arguments for from_bytes");
        ValueMeta type = (ValueMeta)args[0];
        uint8_t* bytes;
        uint32_t bytes_len;
        if(args[1].meta.vtype == VType::raw_arr_ui8 || args[1].meta.vtype == VType::raw_arr_i8){
            bytes = (uint8_t*)args[1].getSourcePtr();
            bytes_len = args[1].meta.val_len;
        }
        else throw InvalidArguments("Invalid type for bytes argument in from_bytes");
        if(type.vtype == VType::noting)
            return nullptr;
        if(bytes_len == 0)
            throw InvalidArguments("Invalid number of bytes for from_bytes");
        ValueItem result;
        switch (type.vtype) {
        case VType::noting:
            return nullptr;
        case VType::boolean:
            result = *(bool*)bytes;
            break;
        case VType::i8:
            result = *(int8_t*)bytes;
            break;
        case VType::ui8:
            result = *(uint8_t*)bytes;
            break;
        case VType::i16:
            if(bytes_len < 2)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 2, got " + std::to_string(bytes_len));
            result = *(int16_t*)bytes;
            break;
        case VType::ui16:
            if(bytes_len < 2)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 2, got " + std::to_string(bytes_len));
            result = *(uint16_t*)bytes;
            break;
        case VType::i32:
            if(bytes_len < 4)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 4, got " + std::to_string(bytes_len));
            result = *(int32_t*)bytes;
            break;
        case VType::ui32:
            if(bytes_len < 4)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 4, got " + std::to_string(bytes_len));
            result = *(uint32_t*)bytes;
            break;
        case VType::i64:
            if(bytes_len < 8)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 8, got " + std::to_string(bytes_len));
            result = *(int64_t*)bytes;
            break;
        case VType::ui64:
            if(bytes_len < 8)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 8, got " + std::to_string(bytes_len));
            result = *(uint64_t*)bytes;
            break;
        case VType::flo:
            if(bytes_len < 4)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 4, got " + std::to_string(bytes_len));
            result = *(float*)bytes;
            break;
        case VType::doub:
            if(bytes_len < 8)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 8, got " + std::to_string(bytes_len));
            result = *(double*)bytes;
            break;
        case VType::string: {
                if(bytes_len == 0){
                    result = std::string();
                    break;
                }
                std::string str((char*)bytes, bytes_len);
                if(str.back() != '\0')
                    str.push_back('\0');
                std::string filtered;
                filtered.reserve(str.size());
                utf8::replace_invalid(str.begin(), str.end(), std::back_inserter(filtered));
                filtered.shrink_to_fit();
                result = filtered;
                break;
            }
        case VType::raw_arr_i8:
            if(bytes_len < type.val_len)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(type.val_len));
            result = ValueItem((int8_t*)bytes, bytes_len);
            break;
        case VType::raw_arr_ui8:
            if(bytes_len < type.val_len)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(type.val_len));
            result = ValueItem((uint8_t*)bytes, bytes_len);
            break;
        case VType::raw_arr_i16:
            if(bytes_len < type.val_len * 2)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/2) + " < " + std::to_string(type.val_len * 2));
            result = ValueItem((int16_t*)bytes, bytes_len / 2);
            break;
        case VType::raw_arr_ui16:
            if(bytes_len < type.val_len * 2)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/2) + " < " + std::to_string(type.val_len * 2));
            result = ValueItem((uint16_t*)bytes, bytes_len / 2);
            break;
        case VType::raw_arr_i32:
            if(bytes_len < type.val_len * 4)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/4) + " < " + std::to_string(type.val_len * 4));
            result = ValueItem((int32_t*)bytes, bytes_len / 4);
            break;
        case VType::raw_arr_ui32:
            if(bytes_len < type.val_len * 4)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/4) + " < " + std::to_string(type.val_len * 4));
            result = ValueItem((uint32_t*)bytes, bytes_len / 4);
            break;
        case VType::raw_arr_i64:
            if(bytes_len < type.val_len * 8)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/8) + " < " + std::to_string(type.val_len * 8));
            result = ValueItem((int64_t*)bytes, bytes_len / 8);
            break;
        case VType::raw_arr_ui64:
            if(bytes_len < type.val_len * 8)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/8) + " < " + std::to_string(type.val_len * 8));
            result = ValueItem((uint64_t*)bytes, bytes_len / 8);
            break;
        case VType::raw_arr_flo:
            if(bytes_len < type.val_len * 4)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/4) + " < " + std::to_string(type.val_len * 4));
            result = ValueItem((float*)bytes, bytes_len / 4);
            break;
        case VType::raw_arr_doub:
            if(bytes_len < type.val_len * 8)
                throw OutOfRange("Invalid number of bytes for from_bytes, " + std::to_string(bytes_len/8) + " < " + std::to_string(type.val_len * 8));
            result = ValueItem((double*)bytes, bytes_len / 8);
            break;
            
	    case VType::type_identifier:
            if(bytes_len < 8)
                throw InvalidArguments("Invalid number of bytes for from_bytes, expected 8, got " + std::to_string(bytes_len));
            result = ValueItem(*(ValueMeta*)bytes);
            break;
        case VType::uarr: 
        case VType::faarr:
        case VType::saarr:
        case VType::map:
        case VType::set:
        case VType::async_res:
        case VType::except_value:
        case VType::struct_:
        case VType::function:
        default:
            throw InvalidArguments("Can't convert from bytes to " + enum_to_string(type.vtype));
        }
        result.meta.allow_edit = type.allow_edit;
        if(type.use_gc)
            result.make_gc();
        return new ValueItem(std::move(result));
    }
	ValueItem* _no_buffer_to_bytes(ValueItem* args, uint32_t){
        ValueItem& value = args[0];
        value.getAsync();
        void* source = value.getSourcePtr();
        
        switch (value.meta.vtype) {
        case VType::noting:
            return new ValueItem(VType::raw_arr_ui8);
        case VType::boolean:
        case VType::i8:
        case VType::ui8:
            return new ValueItem((uint8_t*)&source, 1);
        case VType::i16:
        case VType::ui16:
            return new ValueItem((uint8_t*)&source, 2);
        case VType::i32:
        case VType::ui32:
        case VType::flo:
            return new ValueItem((uint8_t*)&source, 4);
        case VType::i64:
        case VType::ui64:
        case VType::doub:
        case VType::time_point:
        case VType::type_identifier:
            return new ValueItem((uint8_t*)&source, 8);
        case VType::raw_arr_i8:
        case VType::raw_arr_ui8:
            return new ValueItem((uint8_t*)source, value.meta.val_len);
        case VType::raw_arr_i16:
        case VType::raw_arr_ui16:
            return new ValueItem((uint8_t*)source, value.meta.val_len * 2);
        case VType::raw_arr_i32:
        case VType::raw_arr_ui32:
        case VType::raw_arr_flo:
            return new ValueItem((uint8_t*)source, value.meta.val_len * 4);
        case VType::raw_arr_i64:
        case VType::raw_arr_ui64:
        case VType::raw_arr_doub:
            return new ValueItem((uint8_t*)source, value.meta.val_len * 8);
        case VType::string: {
            std::string* str = (std::string*)source;
            return new ValueItem((uint8_t*)str->c_str(), str->size());
        }
        default:
            throw InvalidArguments("Can't convert " + enum_to_string(value.meta.vtype) + " to bytes");
        }
        
    }
    ValueItem* _buffer_to_bytes(ValueItem* args, uint32_t len){
        if(len < 2)
            throw InvalidArguments("Invalid number of arguments for to_bytes");
        ValueItem& value = args[0];
        value.getAsync();
        ValueItem& buffer = args[1];
        buffer.getAsync();
        if(buffer.meta.vtype != VType::raw_arr_ui8 && buffer.meta.vtype != VType::raw_arr_i8)
            throw InvalidArguments("Invalid type for buffer, expected raw_arr_ui8 or raw_arr_i8, got " + enum_to_string(buffer.meta.vtype));
        
        void* source = value.getSourcePtr();
        uint32_t bytes_len = buffer.meta.val_len;
        uint8_t* bytes = (uint8_t*)buffer.getSourcePtr();
        switch (value.meta.vtype) {
        case VType::noting:
            return nullptr;
        case VType::boolean:
        case VType::i8:
        case VType::ui8:
            if(bytes_len < 1)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < 1");
            *bytes = *(uint8_t*)source;
            break;
        case VType::i16:
        case VType::ui16:
            if(bytes_len < 2)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < 2");
            *(uint16_t*)bytes = *(uint16_t*)source;
            break;
        case VType::i32:
        case VType::ui32:
        case VType::flo:
            if(bytes_len < 4)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < 4");
            *(uint32_t*)bytes = *(uint32_t*)source;
            break;
        case VType::i64:
        case VType::ui64:
        case VType::doub:
        case VType::time_point:
        case VType::type_identifier:
            if(bytes_len < 8)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < 8");
            *(uint64_t*)bytes = *(uint64_t*)source;
            break;
        case VType::raw_arr_i8:
        case VType::raw_arr_ui8:
            if(bytes_len < value.meta.val_len)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(value.meta.val_len));
            memcpy(bytes, source, value.meta.val_len);
            break;
        case VType::raw_arr_i16:
        case VType::raw_arr_ui16:
            if(bytes_len < value.meta.val_len * 2)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(value.meta.val_len * 2));
            memcpy(bytes, source, value.meta.val_len * 2);
            break;
        case VType::raw_arr_i32:
        case VType::raw_arr_ui32:
        case VType::raw_arr_flo:
            if(bytes_len < value.meta.val_len * 4)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(value.meta.val_len * 4));
            memcpy(bytes, source, value.meta.val_len * 4);
            break;
        case VType::raw_arr_i64:
        case VType::raw_arr_ui64:
        case VType::raw_arr_doub:
            if(bytes_len < value.meta.val_len * 8)
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(value.meta.val_len * 8));
            memcpy(bytes, source, value.meta.val_len * 8);
            break;
        case VType::string: {
            std::string& str = *(std::string*)source;
            if(bytes_len < str.size())
                throw OutOfRange("Invalid number of bytes for to_bytes, " + std::to_string(bytes_len) + " < " + std::to_string(str.size()));
            memcpy(bytes, str.data(), str.size());
            break;
        }
        default:
            throw InvalidArguments("Can't convert " + enum_to_string(value.meta.vtype) + " to bytes");
        }
        return nullptr;
    }
    ValueItem* to_bytes(ValueItem* args, uint32_t len){
        if(len < 1)
            throw InvalidArguments("Invalid number of arguments for to_bytes");
        if(len == 1)
            return _no_buffer_to_bytes(args, len);
        else
            return _buffer_to_bytes(args, len);
    }
}