#ifndef SRC_RUN_TIME_LIBRARY_LANGUAGE
#define SRC_RUN_TIME_LIBRARY_LANGUAGE
#include "../attacha_abi_structs.hpp"
namespace art{
    namespace optimizers{
        ValueItem* createProxy_opcode_optimizer(ValueItem*, uint32_t);
        ValueItem* createProxy_stack_localizer(ValueItem*, uint32_t);
        ValueItem* createProxy_simd_super_optimizer(ValueItem*, uint32_t);
        ValueItem* createProxy_loop_unroller(ValueItem*, uint32_t);
        ValueItem* createProxy_mathematic_optimizer(ValueItem*, uint32_t);
    }
    namespace compilers{
        ValueItem* createProxy_universal_compiler(ValueItem*, uint32_t);
        //{
        //   void setCompilerConfiguration(string name, any...);
        //   any getCompilerConfiguration(string name);
        //   void languageDecodeConfiguration(string name, any...);
        //   any languageEncodeConfiguration(string name);
        //   patch_list compile_file(string|FileHandle|BlockingFileHandle path);//input file is a text file or file with precompiled information with extension .aap(attachAProgram) and with ".precompiled" header
        //   patch_list compile_text(string text);
        //   patch_list compile_precompiled(ui8[] precompiled_data);
        //   void compile_and_save_file(string|FileHandle|BlockingFileHandle path, string|FileHandle|BlockingFileHandle out_path);
        //   void compile_and_save_text(string text, string|FileHandle|BlockingFileHandle out_path);
        //
        //   void pre_compile_file(string|FileHandle|BlockingFileHandle path, string|FileHandle|BlockingFileHandle out_path);//input file is a text file
        //   void pre_compile_text(string text, string|FileHandle|BlockingFileHandle out_path);
        //   void pre_compile_and_save_file(string|FileHandle|BlockingFileHandle path, string|FileHandle|BlockingFileHandle out_path);//input file is a text file
        //   void pre_compile_and_save_text(string text, string|FileHandle|BlockingFileHandle out_path);
        //}
        //patch_list{
        //   void add_patches(patch_list);
        //   void add_patch(patch);
        //   void remove_patch(std::string name);
        //   patch get_patch(std::string name);
        //   iterator iterate();
        //}
    }



}

#endif /* SRC_RUN_TIME_LIBRARY_LANGUAGE */
