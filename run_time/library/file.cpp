#include "file.hpp"
#include <fstream>
#include "exceptions.hpp"
#include "../../library/enbt/enbt.hpp"
#include "../tasks.hpp"








struct RawFile {
    std::fstream file;
    bool is_last_read_operation : 1;
    bool is_last_operation_append : 1;
    bool sync_rw_pos : 1;
    RawFile(const std::string& path, bool sync_rw_pos = true) : sync_rw_pos(sync_rw_pos) {
        is_last_operation_append = false;
        is_last_read_operation = false;
        file.open(path, std::fstream::in | std::fstream::out | std::fstream::binary);
        if (!file.is_open()) {
            file.open(path, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
            is_last_operation_append = true;
        }
    }
    ~RawFile(){
        file.close();
    }

    void read(ValueItem& item, bool little_endian = true){
        if (!is_last_read_operation) {
            file.clear();
            if(sync_rw_pos) file.seekp(file.tellg(), std::ios::beg);
            is_last_read_operation = true;
        }
        if(!item.meta.allow_edit)
            throw InvalidOperation("Can't read to non-editable item");

        if(item.meta.use_gc || item.meta.as_ref) {
            switch (item.meta.vtype) {
            case VType::ui8:
            case VType::i8:
            case VType::boolean:
            case VType::type_identifier:
                file.read((char*)&item.getSourcePtr(), 1);
                break;
            case VType::ui16:
            case VType::i16:
                file.read((char*)&item.getSourcePtr(), 2);
                break;
            case VType::ui32:
            case VType::i32:
            case VType::flo:
                file.read((char*)&item.getSourcePtr(), 4);
                break;
            case VType::ui64:
            case VType::i64:
            case VType::doub:
            case VType::undefined_ptr:
                file.read((char*)&item.getSourcePtr(), 8);
                break;
            case VType::string:{
                std::string& str = *(std::string*)item.getSourcePtr();
                file.read(str.data(), str.length());
                break;
            }
            case VType::raw_arr_ui8:
            case VType::raw_arr_i8:
                file.read((char*)item.getSourcePtr(), item.meta.val_len);
                break;
            case VType::raw_arr_ui16:
            case VType::raw_arr_i16:
                file.read((char*)item.getSourcePtr(), item.meta.val_len * 2);
                ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, (uint16_t*)item.getSourcePtr(), item.meta.val_len);
                break;
            case VType::raw_arr_ui32:
            case VType::raw_arr_i32:
            case VType::raw_arr_flo:
                file.read((char*)item.getSourcePtr(), item.meta.val_len * 4);
                ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, (uint32_t*)item.getSourcePtr(), item.meta.val_len);
                break;
            case VType::raw_arr_ui64:
            case VType::raw_arr_i64:
            case VType::raw_arr_doub:
                file.read((char*)item.getSourcePtr(), item.meta.val_len * 8);
                ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, (uint32_t*)item.getSourcePtr(), item.meta.val_len);
                break;
            default:
                throw InvalidType("Can't read to this type");
            }
        }
        else
            throw InvalidArguments("Can't read to non-gc or non-ref item");
    }

    ValueItem read(size_t size){
        if (!is_last_read_operation) {
            file.clear();
            if(sync_rw_pos) file.seekp(file.tellg(), std::ios::beg);
            is_last_read_operation = true;
        }
        char* res = new char[size];
        file.read(res, size);
        return ValueItem(res, ValueMeta(VType::raw_arr_ui8,false,true, size), true);
    }

    void write(const uint8_t* data, size_t size){
        if (is_last_read_operation) {
            file.clear();
            if(sync_rw_pos) file.seekg(file.tellp(), std::ios::beg);
            is_last_read_operation = false;
        }
        file.write((char*)data, size);
    }
    void append(const uint8_t* data, size_t size){
        if (is_last_read_operation) {
            file.clear();
            is_last_read_operation = false;
        }
        file.seekp(0, std::ios::end);
        file.write((char*)data, size);
        is_last_operation_append = true;
    }

    void write(const ValueItem& item, bool little_endian = true){
        if (is_last_read_operation) {
            file.clear();
            if(sync_rw_pos) file.seekp(file.tellg(), std::ios::beg);
            is_last_read_operation = false;
        }

        switch (item.meta.vtype) {
        case VType::ui8:
        case VType::i8:
        case VType::boolean:
        case VType::type_identifier:
            file.write((const char*)&item.getSourcePtr(), 1);
            break;
        case VType::ui16:
        case VType::i16:
            file.write((const char*)&item.getSourcePtr(), 2);
            break;
        case VType::ui32:
        case VType::i32:
        case VType::flo:
            file.write((const char*)&item.getSourcePtr(), 4);
            break;
        case VType::ui64:
        case VType::i64:
        case VType::doub:
        case VType::undefined_ptr:
            file.write((const char*)&item.getSourcePtr(), 8);
            break;
        case VType::string:{
            const std::string& str = *(const std::string*)item.getSourcePtr();
            file.write(str.data(), str.length());
            break;
        }
        case VType::raw_arr_ui8:
        case VType::raw_arr_i8:
            file.write((const char*)item.getSourcePtr(), item.meta.val_len);
            break;
        case VType::raw_arr_ui16:
        case VType::raw_arr_i16:{
            std::unique_ptr<uint16_t[]> arr(new uint16_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 2);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 2);
            break;
        }
        case VType::raw_arr_ui32:
        case VType::raw_arr_i32:
        case VType::raw_arr_flo:{
            std::unique_ptr<uint32_t[]> arr(new uint32_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 4);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 4);
            break;
        }
        case VType::raw_arr_ui64:
        case VType::raw_arr_i64:
        case VType::raw_arr_doub:{
            std::unique_ptr<uint64_t[]> arr(new uint64_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 8);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 8);
            break;
        }
        default:
            throw InvalidType("Can't write from this type");
        }
    }
    void append(const ValueItem& item, bool little_endian = true){
        if (is_last_read_operation) {
            file.clear();
            is_last_read_operation = false;
        }
        file.seekp(0, std::ios::end);

        switch (item.meta.vtype) {
        case VType::ui8:
        case VType::i8:
        case VType::boolean:
        case VType::type_identifier:
            file.write((const char*)&item.getSourcePtr(), 1);
            break;
        case VType::ui16:
        case VType::i16:
            file.write((const char*)&item.getSourcePtr(), 2);
            break;
        case VType::ui32:
        case VType::i32:
        case VType::flo:
            file.write((const char*)&item.getSourcePtr(), 4);
            break;
        case VType::ui64:
        case VType::i64:
        case VType::doub:
        case VType::undefined_ptr:
            file.write((const char*)&item.getSourcePtr(), 8);
            break;
        case VType::string:{
            const std::string& str = *(const std::string*)item.getSourcePtr();
            file.write(str.data(), str.length());
            break;
        }
        case VType::raw_arr_ui8:
        case VType::raw_arr_i8:
            file.write((const char*)item.getSourcePtr(), item.meta.val_len);
            break;
        case VType::raw_arr_ui16:
        case VType::raw_arr_i16:{
            std::unique_ptr<uint16_t[]> arr(new uint16_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 2);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 2);
            break;
        }
        case VType::raw_arr_ui32:
        case VType::raw_arr_i32:
        case VType::raw_arr_flo:{
            std::unique_ptr<uint32_t[]> arr(new uint32_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 4);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 4);
            break;
        }
        case VType::raw_arr_ui64:
        case VType::raw_arr_i64:
        case VType::raw_arr_doub:{
            std::unique_ptr<uint64_t[]> arr(new uint64_t[item.meta.val_len]);
            std::memcpy(arr.get(), item.getSourcePtr(), item.meta.val_len * 8);
            ENBT::ConvertEndianArr(little_endian ? ENBT::Endian::little : ENBT::Endian::big, arr.get(), item.meta.val_len);
            file.write((const char*)arr.get(), item.meta.val_len * 8);
            break;
        }
        default:
            throw InvalidType("Can't write from this type");
        }
    }

    void seek_write(uint64_t pos){
        file.seekp(pos);
    }
    void seek_read(uint64_t pos){
        file.seekg(pos);
    }
    uint64_t tell_write(){
        return file.tellp();
    }
    uint64_t tell_read(){
        return file.tellg();
    }
    void flush(){
        file.flush();
    }
    bool is_open() const{
        return file.is_open();
    }
    void close(){
        file.close();
    }
};

struct TextFile{
    std::fstream file;
    bool is_last_read_operation : 1;
    bool is_last_operation_append : 1;
    bool sync_rw_pos : 1;
    TextFile(const std::string& path, bool sync_rw_pos = true) : sync_rw_pos(sync_rw_pos) {
        is_last_operation_append = false;
        is_last_read_operation = false;
        file.open(path, std::fstream::in | std::fstream::out);
        if (!file.is_open()) {
            file.open(path, std::fstream::in | std::fstream::out | std::fstream::trunc);
            is_last_operation_append = true;
        }
    }
    ~TextFile(){
        file.close();
    }
    ValueItem read_line(){
        if (!is_last_read_operation) {
            file.clear();
            if (sync_rw_pos) {
                file.seekg(file.tellp());
            }
            is_last_read_operation = true;
        }
        std::string line;
        std::getline(file, line);
        return ValueItem(line);
    }
    ValueItem read_word(){
        if (!is_last_read_operation) {
            file.clear();
            if (sync_rw_pos) {
                file.seekg(file.tellp());
            }
            is_last_read_operation = true;
        }
        std::string line;
        file >> line;
        return ValueItem(line);
    }
    void write_line(ValueItem& item){
        if (is_last_read_operation) {
            file.clear();
            if (sync_rw_pos) {
                file.seekp(file.tellg());
            }
            is_last_read_operation = false;
        }
        file << (std::string)item << std::endl;
    }
    void write(ValueItem& item){
        if (is_last_read_operation) {
            file.clear();
            if (sync_rw_pos) {
                file.seekp(file.tellg());
            }
            is_last_read_operation = false;
        }
        file << (std::string)item;
    }
};

struct EnbtReaderHandler {
    std::fstream file;
    TaskMutex no_race;

    EnbtReaderHandler(const std::string& path){
        file.open(path, std::fstream::in | std::fstream::out);
        if (!file.is_open()) {
            file.open(path, std::fstream::in | std::fstream::out | std::fstream::trunc);
        }
    }
    ~EnbtReaderHandler(){
        file.close();
    }
};

struct EnbtResult {
    bool token_aviable;
    ENBT::Type_ID id;
    size_t pos;
    ENBT cached;
    EnbtResult(typed_lgr<EnbtReaderHandler>& handle, size_t pos, ENBT::Type_ID id):handle(handle),pos(pos),id(id),token_aviable(true){}
    EnbtResult(typed_lgr<EnbtReaderHandler>& handle, size_t pos):handle(handle),pos(pos),id{},token_aviable(false){}
    operator ValueItem(){
        if(!token_aviable){

        }
    }
private:
    typed_lgr<EnbtReaderHandler> handle;
};
class EnbtFile{
    typed_lgr<EnbtReaderHandler> handle;
public:
    EnbtFile(const std::string& path) : handle(new EnbtReaderHandler(path)) {}

    ValueItem read(){
        
    }

};









namespace file {
	namespace constructor {
		ValueItem* createProxy_RawFile(ValueItem*, uint32_t);
		ValueItem* createProxy_TextFile(ValueItem*, uint32_t);
		ValueItem* createProxy_EnbtFile(ValueItem*, uint32_t);
		ValueItem* createProxy_JsonFile(ValueItem*, uint32_t);
		ValueItem* createProxy_YamlFile(ValueItem*, uint32_t);
	}
	namespace log {
		namespace constructor {
			ValueItem* createProxy_TextLog(ValueItem*, uint32_t);
			ValueItem* createProxy_EnbtLog(ValueItem*, uint32_t);
			ValueItem* createProxy_JsonLog(ValueItem*, uint32_t);
		}
	}

	ValueItem* readBytes(ValueItem*, uint32_t); //ui8[] / uarr<ui8>
	ValueItem* readLines(ValueItem*, uint32_t); //uarr<string>
	ValueItem* readEnbts(ValueItem*, uint32_t); //uarr<any>
	ValueItem* readJson(ValueItem*, uint32_t); //any
	ValueItem* readYaml(ValueItem*, uint32_t); //any
}