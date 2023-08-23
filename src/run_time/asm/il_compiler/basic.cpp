#include <run_time/asm/il_compiler/basic.hpp>
#include <run_time/asm/il_compiler/art_1_0_0.hpp>
namespace art{
    namespace il_compiler{
        basic* map_compiler(art::ustring name_version){
            if(name_version == "art_1.0.0")
                return new art_1_0_0::compiler();
            throw InvalidFunction("Invalid function header, unsupported header name/version: " + name_version);
        }
    }
}