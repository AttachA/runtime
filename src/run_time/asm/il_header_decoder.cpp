#include "il_header_decoder.hpp"
#include "../util/tools.hpp"
namespace art{
    size_t il_header::decoded::decode(
        CASM& casm_assembler,//for labels
        const std::vector<uint8_t>& data,
        size_t start,
        size_t end_offset
    ){
        std::string version = reader::readString(data, end_offset, start);
        return decode(
            version,
            casm_assembler,
            data,
            start,
            end_offset
        );
    }

    size_t il_header::decoded::decode(
        const std::string& header_compiler_name_version,
        CASM& a,
        const std::vector<uint8_t>& data,
        size_t to_be_skiped,
        size_t data_len
    ){
        if(compiler) delete compiler;
        
        compiler = il_compiler::map_compiler(header_compiler_name_version);
        compiler->decode_header(
            data,
            to_be_skiped,
            data_len,
            a,
            jump_list,
            locals,
            flags,
            used_static_values,
            used_enviro_vals,
            used_arguments,
            constants_values
        );
        return to_be_skiped;
    }
    il_header::decoded::~decoded(){
        if(compiler)
            delete compiler;
    }
}