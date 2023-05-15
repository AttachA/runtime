// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "file.hpp"
#include "exceptions.hpp"
#include "bytes.hpp"
#include "../tasks.hpp"
#include "../files.hpp"
#include "../AttachA_CXX.hpp"
#include <utf8cpp/utf8.h>
namespace AIs = AttachA::Interface::special;
using namespace bytes;

ProxyClassDefine define_FileHandle;
ProxyClassDefine define_BlockingFileHandle;
ProxyClassDefine define_TextFile;

class TextFile{
public:
    enum class Encoding {
        utf8,
        utf16,
        utf32,
        ascii
    };
private:
    typed_lgr<files::FileHandle> handle;
    template<typename T, size_t to_read>
    void read_buffer(list_array<ValueItem>& results_holder, T*& buffer, size_t& size, size_t& rest){
        ValueItem item = handle->read(to_read);
        typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
        auto results = Task::await_results(task);
        buffer = (T*)results[0].val;
        size = results[0].meta.val_len / sizeof(T);
        rest = results[1].meta.val_len % sizeof(T);
        convert_endian_arr<T>(endian, buffer, size);
    }
#define read_buffer_type(type, to_read) list_array<ValueItem> results; type* str; size_t max_i;size_t rest; read_buffer<type,to_read>(results, str, max_i, rest);if(max_i == 0)break;
    ValueItem validate_return_utf8(const std::string& str){
        if(file_encoding == Encoding::ascii)
            return str;
        if(str.size() == 0)
            return str;
        std::string result;
        result.reserve(str.size());
        utf8::replace_invalid(str.begin(), str.end(), std::back_inserter(result));
        if(result[result.size() - 1] != '\0')
            result.push_back('\0');
        result.shrink_to_fit();
        return result;
    }
    std::string convert_to_read(std::u16string& str, bool endian_convert){
        if(endian_convert)
            convert_endian_arr(endian, str.data(), str.size());
        std::string result;
        utf8::utf16to8(str.begin(), str.end(), std::back_inserter(result));
        return result;
    }
    std::u16string convert_to_write_u16(const std::string& line, bool endian_convert){
        std::u16string res;
        utf8::utf8to16(line.begin(), line.end(), std::back_inserter(res));
        if(endian_convert)
            convert_endian_arr(endian, res.data(), res.size());
        return res;
    }

    std::string convert_to_read(std::u32string& str, bool endian_convert){
        if(endian_convert)
            convert_endian_arr(endian, str.data(), str.size());
        std::string result;
        utf8::utf32to8(str.begin(), str.end(), std::back_inserter(result));
        return result;
    }
    std::u32string convert_to_write_u32(const std::string& line, bool endian_convert){
        std::u32string res;
        utf8::utf8to32(line.begin(), line.end(), std::back_inserter(res));
        if(endian_convert)
            convert_endian_arr(endian, res.data(), res.size());
        return res;
    }

    ValueItem read_line_ascii_utf8(){
        std::string line;
        line.reserve(1024);
        while(true){
            ValueItem item = handle->read(1024);
            typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
            auto results = Task::await_results(task);
            ValueItem& item = results[0];
            if(item.meta.val_len == 0)
                break;
            auto str = (uint8_t*)item.val;
            for(uint32_t i = 0; i < item.meta.val_len; i++){
                char c = str[i];
                if(c == '\n'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(line);
                }
                line += c;
            }
            if(results.size()==2)
                break;
            
        }
        if(line.size() == 0)
             return nullptr;
        else return validate_return_utf8(line);
    }
    ValueItem read_word_ascii_utf8(){
        std::string word;
        word.reserve(32);
        while(true){
            ValueItem item = handle->read(1024);
            typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
            auto results = Task::await_results(task);
            ValueItem& item = results[0];
            if(item.meta.val_len == 0)
                break;
            auto str =  (uint8_t*)item.val;
            for(uint32_t i = 0; i < item.meta.val_len; i++){
                char c = str[i];
                if(c == ' ' || c == '\n' || c == '\t' || c == '\r'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(word);
                }
                word += c;
            }
            if(results.size()==2)
                break;
        }
        if(word.size() == 0)
             return nullptr;
        else return validate_return_utf8(word);
    }
    ValueItem read_symbol_ascii_utf8(bool ascii_mode, bool skip_spaces){
        std::string symbol;
        symbol.reserve(2);
        bool state_readed = false;
        uint8_t bytes_need_read = 0;
        while(true){
            ValueItem item = handle->read(1);
            typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
            auto results = Task::await_results(task);
            ValueItem& item = results[0];
            if(item.meta.val_len == 0)
                break;
            uint8_t c = *(uint8_t*)item.val;
            switch (c) {
            case ' ':
                if(skip_spaces)
                    break;
                [[fallthrough]]
            default:
                if(c < 32)
                    break;
                [[fallthrough]]
            case '\n':
            case '\t':
            case '\b':
                if(ascii_mode)
                    return std::string(1, (char)c);
                else{
                    if(!state_readed){
                        symbol += c;
                        state_readed = true;
                        //decode bytes need read
                        if(c & 0b10000000){
                            if(c & 0b01000000){
                                if(c & 0b00100000){
                                    if(c & 0b00010000)
                                        bytes_need_read = 4;
                                    else
                                        bytes_need_read = 3;
                                }else
                                    bytes_need_read = 2;
                            }else
                                bytes_need_read = 1;
                        }
                    }
                    if(bytes_need_read == 0)
                        return validate_return_utf8(symbol);
                    else{ 
                        symbol += c;
                        bytes_need_read--;
                        break;
                    }
                }
            }
        }
        if(symbol.size() == 0)
             return nullptr;
        else return validate_return_utf8(symbol);
    }
    ValueItem write_ascii_utf8(ValueItem& item, bool ascii_mode){
        if(item.meta.vtype == VType::string){
            auto& string = *(std::string*)item.getSourcePtr();
            if(ascii_mode){
                for(char c : string){
                    if(c > 127)
                        throw InvalidType("Can't write utf8 or extrended ascii to regular ascii file");
                }
            }
            return handle->write((uint8_t*)string.data(), string.size());
        }else{
            auto string = (std::string)item;
            return handle->write((uint8_t*)string.data(), string.size());
        }
    }

    ValueItem read_line_utf16(){
        std::u16string line;
        line.reserve(1024);
        while(true){
            read_buffer_type(uint16_t, 1024);
            for(uint32_t i = 0; i < max_i; i++){
                uint16_t c = str[i];
                if(c == '\n'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(convert_to_read(line, true));
                }
                line.push_back(c);
            }
            if(results.size()==2)
                break;
        }
        if(line.size() == 0)
            return nullptr;
        else
            return validate_return_utf8(convert_to_read(line, true));
    }
    ValueItem read_word_utf16(){
        std::u16string word;
        word.reserve(32);
        while(true){
            read_buffer_type(uint16_t, 1024);
            for(uint32_t i = 0; i < max_i; i++){
                uint16_t c = str[i];
                if(c == ' ' || c == '\n' || c == '\t' || c == '\r'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(convert_to_read(word, true));
                }
                word.push_back(c);
            }
            if(results.size()==2)
                break;
        }
        if(word.size() == 0)
            return nullptr;
        else
            return validate_return_utf8(convert_to_read(word, true));
    }
    ValueItem read_symbol_utf16(bool skip_spaces){
        std::u16string symbol;
        symbol.reserve(32);
        bool state_readed = false;
        while(true){
            ValueItem item = handle->read(2);
            typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
            auto results = Task::await_results(task);
            {
                ValueItem& item = results[0];
                if(item.meta.val_len == 0){
                    if(state_readed)
                        throw InvalidInput("Invalid utf16 symbol, wrong codepoint");
                    else break;
                }
                uint16_t c = *(uint16_t*)item.val;
                if(state_readed){
                    if((c & 0b1111110000000000) != 0b1101110000000000)
                        return validate_return_utf8(convert_to_read(symbol += c, true));
                    else{
                        throw InvalidInput("Invalid utf16 symbol, wrong codepoint");
                    }
                }
                switch (c) {
                case ' ':
                    if(skip_spaces)
                        break;
                    [[fallthrough]]
                default:
                    if(c < 32)
                        break;
                    [[fallthrough]]
                case '\n':
                case '\t':
                case '\b':
                    symbol += c;
                    state_readed = true;
                    if constexpr(Endian::native != Endian::little)
                        c = convert_endian<Endian::native>(c);
                    c &= 0b1111110000000000;
                    if(c){
                        if(c == 0b1101100000000000)
                            return validate_return_utf8(convert_to_read(symbol, true));
                        else
                            throw InvalidInput("Invalid utf16 symbol, wrong codepoint");
                    }else break;
                }
            }
        }
        if(symbol.size() == 0)
            return nullptr;
        else
            return validate_return_utf8(convert_to_read(symbol, true));
    }
    ValueItem write_utf16(ValueItem& item){
        std::u16string string;
        if(item.meta.vtype == VType::string){
            string = convert_to_write_u16(*(std::string*)item.getSourcePtr(), true);
        
        }else
            string = convert_to_write_u16(((std::string)item), true);
        size_t size = string.size() * 2;
        auto string_ptr = (uint8_t*)string.data();
        while(size > UINT32_MAX){
            handle->write(string_ptr, UINT32_MAX);
            size -= UINT32_MAX;
            string_ptr += UINT32_MAX;
        }
        if(size)
            handle->write(string_ptr, size);
    }
    void read_bom_utf16(){
        do{
            read_buffer_type(uint16_t, 2);
            if(str[0] == 0xFEFF)endian = Endian::big;
            else if(str[0] == 0xFFFE)endian = Endian::little;
            else{
                handle->seek_pos(-2, files::pointer_offset::current, files::pointer::read);
                endian = Endian::native;
            }
        }while(false);
    }
    void init_bom_utf16(){
        if(handle->size())return;
        if(endian == Endian::big)
            handle->write((uint8_t*)"\xFE\xFF", 2);
        else if(endian == Endian::little)
            handle->write((uint8_t*)"\xFF\xFE", 2);
    }

    ValueItem read_line_utf32(){
        std::u32string line;
        line.reserve(1024);
        while(true){
            read_buffer_type(uint32_t, 1024);
            for(uint32_t i = 0; i < max_i; i++){
                uint32_t c = str[i];
                if(c == '\n'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(convert_to_read(line, true));
                }
                line.push_back(c);
            }
            if(results.size()==2)
                break;
        }
        if(line.size() == 0)
            return nullptr;
        else
            return validate_return_utf8(convert_to_read(line, true));
    }
    ValueItem read_word_utf32(){
        std::u32string word;
        word.reserve(32);
        while(true){
            read_buffer_type(uint32_t, 1024);
            for(uint32_t i = 0; i < max_i; i++){
                uint32_t c = str[i];
                if(c == ' ' || c == '\n' || c == '\t' || c == '\r'){
                    handle->seek_pos(i + 1, files::pointer_offset::current, files::pointer::read);
                    return validate_return_utf8(convert_to_read(word, true));
                }
                word.push_back(c);
            }
            if(results.size()==2)
                break;
        }
        if(word.size() == 0)
            return nullptr;
        else
            return validate_return_utf8(convert_to_read(word, true));
    }
    ValueItem read_symbol_utf32(bool skip_spaces){
        while(true){
            ValueItem item = handle->read(4);
            typed_lgr<Task> task = *(typed_lgr<Task>*)item.val;
            auto results = Task::await_results(task);
            {
                ValueItem& item = results[0];
                if(item.meta.val_len == 0)
                    return nullptr;
                uint32_t c = *(uint32_t*)item.val;
                switch (c) {
                case ' ':
                    if(skip_spaces)
                        break;
                    [[fallthrough]]
                default:
                    if(c < 32)
                        break;
                    [[fallthrough]]
                case '\n':
                case '\t':
                case '\b':{
                    std::u32string res(1,c);
                    return validate_return_utf8(convert_to_read(res, true));
                }
                }
            }
        }
    }
    ValueItem write_utf32(ValueItem& item){
        std::u32string string;
        if(item.meta.vtype == VType::string){
            string = convert_to_write_u32(*(std::string*)item.getSourcePtr(), true);
        
        }else
            string = convert_to_write_u32(((std::string)item), true);
        size_t size = string.size() * 2;
        auto string_ptr = (uint8_t*)string.data();
        while(size > UINT32_MAX){
            handle->write(string_ptr, UINT32_MAX);
            size -= UINT32_MAX;
            string_ptr += UINT32_MAX;
        }
        if(size)
            handle->write(string_ptr, size);
    }
    void read_bom_utf32(){
        do{
            read_buffer_type(uint32_t, 4);
            if(str[0] == 0x0000FEFF) endian = Endian::big;
            else if(str[0] == 0xFFFE0000) endian = Endian::little;
            else{
                handle->seek_pos(-4, files::pointer_offset::current, files::pointer::read);
                endian = Endian::native;
            }
        }while(false);
    }
    void init_bom_utf32(){
        if(handle->size())return;
        if(endian == Endian::big)
            handle->write((uint8_t*)"\x00\x00\xFE\xFF", 4);
        else if(endian == Endian::little)
            handle->write((uint8_t*)"\xFF\xFE\x00\x00", 4);
    }
#undef read_buffer_type
    Endian endian;
public:
    const Encoding file_encoding;
    TextFile(typed_lgr<files::FileHandle> handle, Encoding encoding, Endian endian) : handle(handle), file_encoding(encoding), endian(endian) {}
    ValueItem read_line(){
        switch (file_encoding) {
        case Encoding::ascii:
        case Encoding::utf8:
            return read_line_ascii_utf8();
        case Encoding::utf16:
            return read_line_utf16();
        case Encoding::utf32:
            return read_line_utf32();
        default:
            return nullptr;
        }
    }
    ValueItem read_word(){
        switch (file_encoding) {
        case Encoding::ascii:
        case Encoding::utf8:
            return read_word_ascii_utf8();
        case Encoding::utf16:
            return read_word_utf16();
        case Encoding::utf32:
            return read_word_utf32();
        default:
            return nullptr;
        }
    }
    ValueItem read_symbol(bool skip_spaces){
        switch (file_encoding) {
        case Encoding::ascii:
            return read_symbol_ascii_utf8(true, skip_spaces);
        case Encoding::utf8:
            return read_symbol_ascii_utf8(false, skip_spaces);
        case Encoding::utf16:
            return read_symbol_utf16(skip_spaces);
        case Encoding::utf32:
            return read_symbol_utf32(skip_spaces);
        default:
            return nullptr;
        }
    }
    ValueItem write(ValueItem& item){
        switch (file_encoding) {
        case Encoding::ascii:
            return write_ascii_utf8(item, true);
        case Encoding::utf8:
            return write_ascii_utf8(item, false);
        case Encoding::utf16:
            return write_utf16(item);
        case Encoding::utf32:
            return write_utf32(item);
        default:
            return nullptr;
        }
    }

    void read_bom(){
        switch (file_encoding) {
        case Encoding::utf8:
        case Encoding::utf16:
            read_bom_utf16();
            break;
        case Encoding::utf32:
            read_bom_utf32();
            break;
        default:
            break;
        }
    }
    void init_bom(){
        switch (file_encoding) {
        case Encoding::utf8:
        case Encoding::utf16:
            init_bom_utf16();
            break;
        case Encoding::utf32:
            init_bom_utf32();
            break;
        default:
            break;
        }
    }
};

namespace file {
    template<class T, ProxyClassDefine& t_define>
    typed_lgr<T>& checked_get(ValueItem* args, uint32_t len, uint32_t min_len){
        if(len < min_len)
            throw InvalidArguments("excepted at least " + std::to_string(min_len) + " arguments, got " + std::to_string(len));
        ProxyClass& proxy = (ProxyClass&)args[0];
        if(proxy.declare_ty != &t_define){
            if(proxy.declare_ty->name != t_define.name)
                throw InvalidArguments("excepted " + t_define.name + ", got " + proxy.declare_ty->name);
            else
                throw InvalidArguments("excepted " + t_define.name + ", got non native " + t_define.name);
        }
        return *(::typed_lgr<T>*)(proxy.class_ptr);
    }
	namespace constructor {
        ValueItem* createProxy_FileHandle(ValueItem* args, uint32_t len){
            if(len < 1)
                throw InvalidArguments("Expected at least 1 argument");
            auto path = (std::string)args[0];
            bool is_async = true;
            if(len >= 2)if(args[2].meta.vtype != VType::noting) is_async = (bool)args[1];
            files::open_mode mode = files::open_mode::read_write;
            if(len >= 3)if(args[2].meta.vtype != VType::noting) mode = (files::open_mode)(uint8_t)args[2];
            files::on_open_action action = files::on_open_action::open;
            if(len >= 4)if(args[3].meta.vtype != VType::noting) action = (files::on_open_action)(uint8_t)args[3];
            files::_sync_flags flags{0};
            if(len >= 5)if(args[4].meta.vtype != VType::noting) flags.value = (uint8_t)args[4];
            files::share_mode share;
            if(len >= 6)if(args[5].meta.vtype != VType::noting) share.value = (uint8_t)args[5];
            files::pointer_mode pointer_mode = files::pointer_mode::combined;
            if(len >= 7)if(args[6].meta.vtype != VType::noting) pointer_mode = (files::pointer_mode)(uint8_t)args[6];

            if(is_async){
                files::_async_flags aflags;
                aflags.value = flags.value;
                return new ValueItem(new ProxyClass(new typed_lgr<files::FileHandle>(new files::FileHandle(path.c_str(), path.size(), mode, action, aflags, share, pointer_mode))), VType::proxy, no_copy);
            }
            else
                return new ValueItem(new ProxyClass(new typed_lgr<files::FileHandle>(new files::FileHandle(path.c_str(), path.size(), mode, action, flags, share, pointer_mode))), VType::proxy, no_copy);
        }
        ValueItem* createProxy_BlockingFileHandle(ValueItem* args, uint32_t len){
            if(len < 1)
                throw InvalidArguments("Expected at least 1 argument");
            auto path = (std::string)args[0];
            files::open_mode mode = files::open_mode::read_write;
            if(len >= 2)if(args[1].meta.vtype != VType::noting) mode = (files::open_mode)(uint8_t)args[1];
            files::on_open_action action = files::on_open_action::open;
            if(len >= 3)if(args[2].meta.vtype != VType::noting) action = (files::on_open_action)(uint8_t)args[2];
            files::_sync_flags flags{0};
            if(len >= 4)if(args[3].meta.vtype != VType::noting) flags.value = (uint8_t)args[3];
            files::share_mode share;
            if(len >= 5)if(args[4].meta.vtype != VType::noting) share.value = (uint8_t)args[4];
            files::pointer_mode pointer_mode = files::pointer_mode::combined;
            if(len >= 6)if(args[5].meta.vtype != VType::noting) pointer_mode = (files::pointer_mode)(uint8_t)args[5];

            return new ValueItem(new ProxyClass(new typed_lgr<files::BlockingFileHandle>(new files::BlockingFileHandle(path.c_str(), path.size(), mode, action, flags, share))), VType::proxy, no_copy);
        }

		ValueItem* createProxy_TextFile(ValueItem* args, uint32_t len){
            auto file_handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 1);
            auto endian = Endian::native;
            if(len >= 2)if(args[1].meta.vtype != VType::noting) endian = (Endian)(uint8_t)args[1];
            auto encoding = TextFile::Encoding::utf8;
            if(len >= 3)if(args[2].meta.vtype != VType::noting) encoding = (TextFile::Encoding)(uint8_t)args[2];
            return new ValueItem(new ProxyClass(new typed_lgr<TextFile>(new TextFile(file_handle, encoding, endian)), &define_TextFile), VType::proxy, no_copy);
        }
	}


#pragma region FileHandle
    ValueItem* funs_FileHandle_read(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->read((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else 
            return new ValueItem(handle->read((uint32_t)args[1]));
    }
    ValueItem* funs_FileHandle_read_fixed(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->read_fixed((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else 
            return new ValueItem(handle->read_fixed((uint32_t)args[1]));
    }
    ValueItem* funs_FileHandle_write(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->write((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else 
            throw InvalidArguments("excepted raw_arr_ui8 or raw_arr_i8, got " + enum_to_string(item.meta.vtype));
    }
    ValueItem* funs_FileHandle_append(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->append((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else 
            throw InvalidArguments("excepted raw_arr_ui8 or raw_arr_i8, got " + enum_to_string(item.meta.vtype));
    }
    ValueItem* funs_FileHandle_seek_pos(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        uint64_t offset = (uint64_t)args[1];
        files::pointer_offset pointer_offset = files::pointer_offset::begin;
        if(len >= 3) if(args[2].meta.vtype != VType::noting) pointer_offset = (files::pointer_offset)(uint8_t)args[2];
        if(len >= 4) if(args[3].meta.vtype != VType::noting) handle->seek_pos(offset, pointer_offset, (files::pointer)(uint8_t)args[3]);
        else handle->seek_pos(offset, pointer_offset);
    }
    ValueItem* funs_FileHandle_tell_pos(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 2);
        return new ValueItem(handle->tell_pos((files::pointer)(uint8_t)args[1]));
    }
    ValueItem* funs_FileHandle_flush(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 1);
        return new ValueItem(handle->flush());
    }
    ValueItem* funs_FileHandle_size(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 1);
        return new ValueItem(handle->size());
    }
    ValueItem* funs_FileHandle_internal_get_handle(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::FileHandle, define_FileHandle>(args, len, 1);
        return new ValueItem(handle->internal_get_handle());
    }
#pragma endregion

#pragma region BlockingFileHandle
    ValueItem* funs_BlockingFileHandle_read(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->read((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else {
            uint32_t len = (uint32_t)args[1];
            if(len == 0) return new ValueItem(0);
            uint8_t* ptr = new uint8_t[len];
            auto res = handle->read(ptr, len);
            if(res <= 0) {
                delete[] ptr;
                return new ValueItem(res);
            }
            else return new ValueItem(ptr, len, no_copy);
        }
    }
    ValueItem* funs_BlockingFileHandle_write(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 2);
        ValueItem& item = args[1];
        if(item.meta.vtype != VType::raw_arr_ui8 || item.meta.vtype != VType::raw_arr_i8)
            return new ValueItem(handle->write((uint8_t*)item.getSourcePtr(), item.meta.val_len));
        else 
            throw InvalidArguments("excepted raw_arr_ui8 or raw_arr_i8, got " + enum_to_string(item.meta.vtype));
    }
    ValueItem* funs_BlockingFileHandle_seek_pos(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 2);
        uint64_t offset = (uint64_t)args[1];
        files::pointer_offset pointer_offset = files::pointer_offset::begin;
        if(len >= 3) if(args[2].meta.vtype != VType::noting) pointer_offset = (files::pointer_offset)(uint8_t)args[2];
        handle->seek_pos(offset, pointer_offset);
    }
    ValueItem* funs_BlockingFileHandle_tell_pos(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->tell_pos());
    }
    ValueItem* funs_BlockingFileHandle_flush(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->flush());
    }
    ValueItem* funs_BlockingFileHandle_size(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->size());
    }
    ValueItem* funs_BlockingFileHandle_eof_state(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->eof_state());
    }
    ValueItem* funs_BlockingFileHandle_valid(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->valid());
    }
    ValueItem* funs_BlockingFileHandle_internal_get_handle(ValueItem* args, uint32_t len){
        auto& handle = checked_get<files::BlockingFileHandle, define_BlockingFileHandle>(args, len, 1);
        return new ValueItem(handle->internal_get_handle());
    }
#pragma endregion

#pragma region TextFile
    ValueItem* funs_TextFile_read_line(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 1);
        return new ValueItem(handle->read_line());
    }
    ValueItem* funs_TextFile_read_word(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 1);
        return new ValueItem(handle->read_word());
    }
    ValueItem* funs_TextFile_read_symbol(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 1);
        if(len>=2)
            return new ValueItem(handle->read_symbol((bool)args[2]));
        else return new ValueItem(handle->read_symbol(false));
    }
    ValueItem* funs_TextFile_write(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 2);
        return new ValueItem(handle->write(args[1]));
    }
    ValueItem* funs_TextFile_read_bom(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 1);
        handle->read_bom();
        return nullptr;
    }
    ValueItem* funs_TextFile_init_bom(ValueItem* args, uint32_t len){
        auto& handle = checked_get<TextFile, define_TextFile>(args, len, 1);
        handle->init_bom();
        return nullptr;
    }
#pragma endregion
    

	ValueItem* remove(ValueItem* args, uint32_t len){
        if(len < 1) throw InvalidArguments("excepted 1 argument, got 0");
        if(args[0].meta.vtype == VType::string){
            std::string& path = *(std::string*)args[0].getSourcePtr();
            return new ValueItem(files::remove(path.c_str(), path.size()));
        }
        else{
            std::string path = (std::string)args[0];
            return new ValueItem(files::remove(path.c_str(), path.size()));
        }
    }

	void init(){
        define_FileHandle.name = "file_handle";
        define_FileHandle.copy = AIs::proxyCopy<files::FileHandle, true>;
        define_FileHandle.destructor = AIs::proxyDestruct<files::FileHandle, true>;
        define_FileHandle.funs["read"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_read, false), false, ClassAccess::pub);
        define_FileHandle.funs["read_fixed"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_read_fixed, false), false, ClassAccess::pub);
        define_FileHandle.funs["write"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_write, false), false, ClassAccess::pub);
        define_FileHandle.funs["append"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_append, false), false, ClassAccess::pub);
        define_FileHandle.funs["seek_pos"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_seek_pos, false), false, ClassAccess::pub);
        define_FileHandle.funs["tell_pos"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_tell_pos, false), false, ClassAccess::pub);
        define_FileHandle.funs["flush"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_flush, false), false, ClassAccess::pub);
        define_FileHandle.funs["size"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_size, false), false, ClassAccess::pub);
        define_FileHandle.funs["get_native_handle"] = ClassFnDefine(new FuncEnviropment(funs_FileHandle_internal_get_handle, false), false, ClassAccess::intern);

        define_BlockingFileHandle.name = "blocking_file_handle";
        define_BlockingFileHandle.copy = AIs::proxyCopy<files::BlockingFileHandle, true>;
        define_BlockingFileHandle.destructor = AIs::proxyDestruct<files::BlockingFileHandle, true>;
        define_BlockingFileHandle.funs["read"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_read, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["write"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_write, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["seek_pos"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_seek_pos, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["tell_pos"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_tell_pos, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["flush"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_flush, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["size"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_size, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["eof_state"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_eof_state, false), false, ClassAccess::pub);
        define_BlockingFileHandle.funs["valid"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_valid, false), false, ClassAccess::pub);
        define_FileHandle.funs["get_native_handle"] = ClassFnDefine(new FuncEnviropment(funs_BlockingFileHandle_internal_get_handle, false), false, ClassAccess::intern);

        define_TextFile.name = "text_file";
        define_TextFile.copy = AIs::proxyCopy<TextFile, true>;
        define_TextFile.destructor = AIs::proxyDestruct<TextFile, true>;
        define_TextFile.funs["read_line"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_read_line, false), false, ClassAccess::pub);
        define_TextFile.funs["read_word"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_read_symbol, false), false, ClassAccess::pub);
        define_TextFile.funs["read_symbol"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_read_symbol, false), false, ClassAccess::pub);
        define_TextFile.funs["write"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_write, false), false, ClassAccess::pub);
        define_TextFile.funs["read_bom"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_read_bom, false), false, ClassAccess::pub);
        define_TextFile.funs["init_bom"] = ClassFnDefine(new FuncEnviropment(funs_TextFile_init_bom, false), false, ClassAccess::pub);
    }
}