#include <run_time/asm/compiler/compiler_include.hpp>
namespace art{
	void Compiler::StaticCompiler::is_gc(const ValueIndexPos& value, ValueMeta value_meta){
        //TODO: Implement
        compiler.dynamic().is_gc(value);
    }
}