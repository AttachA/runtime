// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <util/platform.hpp>
#if !defined(SRC_RUN_TIME_ASM_CASM_DWARF_BUILDER) && PLATFORM_LINUX
#define SRC_RUN_TIME_ASM_CASM_DWARF_BUILDER
#include <cstddef>
#include <library/list_array.hpp>
#include <string>

struct CIE_entry {
    uint32_t id;
    uint8_t version;
    struct {
        bool enabled;
    } augmentation_remainder; // z
    struct {
        bool enabled;
        uint32_t plt_entry; // encoding 0x3 | 0x40 i.e. value is 4 bytes and relative to start of function
    } personality;          // P
    struct {
        bool enabled;
    } use_fde; // R
    struct {
        bool enabled;
    } lsda; // L
    uint32_t code_alignment_factor;
    int32_t data_alignment_factor;
    uint8_t return_address_register;
};
struct FDE_entry {
    uint64_t function_pointer;
    uint64_t function_size = 0;
};
enum class cfa_opcodes : uint8_t { // linux repo //arch/sh/include/asm/dwarf.h#L315
    CFA_advance_loc = 0x40,
    CFA_offset = 0x80,
    CFA_restore = 0xc0,
    CFA_nop = 0x00,
    CFA_set_loc = 0x01,
    CFA_advance_loc1 = 0x02,
    CFA_advance_loc2 = 0x03,
    CFA_advance_loc4 = 0x04,
    CFA_offset_extended = 0x05,
    CFA_restore_extended = 0x06,
    CFA_undefined = 0x07,
    CFA_same_value = 0x08,
    CFA_register = 0x09,
    CFA_remember_state = 0x0a,
    CFA_restore_state = 0x0b,
    CFA_def_cfa = 0x0c,
    CFA_def_cfa_register = 0x0d,
    CFA_def_cfa_offset = 0x0e,
    CFA_def_cfa_expression = 0x0f,
    CFA_expression = 0x10,
    CFA_offset_extended_sf = 0x11,
    CFA_def_cfa_sf = 0x12,
    CFA_def_cfa_offset_sf = 0x13,
    CFA_val_offset = 0x14,
    CFA_val_offset_sf = 0x15,
    CFA_val_expression = 0x16,
    CFA_lo_user = 0x1c,
    CFA_hi_user = 0x3f
};
inline list_array<uint8_t> make_uleb128(uint32_t value) {
    list_array<uint8_t> data;
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value)
            byte |= 0x80;
        data.push_back(byte);
    } while (value);
    return data;
}
inline list_array<uint8_t> make_sleb128(uint32_t value) {
    return make_uleb128(value);
}

struct cfi_builder {
    list_array<uint8_t> data;
    void def_cfa(uint8_t register_, uint32_t offset) {
        data.push_back((uint8_t)cfa_opcodes::CFA_def_cfa);
        data.push_back(register_);
        data.push_back(make_uleb128(offset));
    }
    void def_cfa_register(uint8_t register_) {
        data.push_back((uint8_t)cfa_opcodes::CFA_def_cfa_register);
        data.push_back(register_);
    }
    void def_cfa_offset(uint32_t offset) {
        data.push_back((uint8_t)cfa_opcodes::CFA_def_cfa_offset);
        data.push_back(make_uleb128(offset));
    }
    void offset(uint8_t register_, uint32_t offset) {
        data.push_back((uint8_t)cfa_opcodes::CFA_offset | register_);
        data.push_back(make_uleb128(offset));
    }
};
struct ffi_builder {
    list_array<uint8_t> data;
    uint64_t lastOffset = 0;
    void advance_loc(uint64_t offset) {
        uint64_t delta = offset - lastOffset;
        if (delta < (1 << 6))
            data.push_back((uint8_t)cfa_opcodes::CFA_advance_loc | delta);

        else if (delta < (1 << 8)) {
            data.push_back((uint8_t)cfa_opcodes::CFA_advance_loc1);
            data.push_back(delta);
        } else if (delta < (1 << 16)) {
            data.push_back((uint8_t)cfa_opcodes::CFA_advance_loc2);
            data.push_back(delta & 0xff);
            data.push_back((delta >> 8) & 0xff);
        } else {
            data.push_back((uint8_t)cfa_opcodes::CFA_advance_loc4);
            data.push_back(delta & 0xff);
            data.push_back((delta >> 8) & 0xff);
            data.push_back((delta >> 16) & 0xff);
            data.push_back((delta >> 24) & 0xff);
        }
        lastOffset = offset;
    }
    void stack_offset(int stackOffset) {
        data.push_back((uint8_t)cfa_opcodes::CFA_def_cfa_offset);
        data.push_back(make_uleb128(stackOffset));
    }
    void offset(int dwarfRegId, int stackLocation) {
        data.push_back((uint8_t)cfa_opcodes::CFA_offset | dwarfRegId);
        data.push_back(make_uleb128(stackLocation));
    }
};

#pragma pack(push, 1)
inline list_array<uint8_t> make_CIE(const CIE_entry& entry,
                                    const list_array<uint8_t>& augmentation_data,
                                    const list_array<uint8_t>& instructions) {
    list_array<uint8_t> data;
    art::ustring augmentation_config;
    if (entry.augmentation_remainder.enabled)
        augmentation_config += "z";
    size_t personality_offset_set = data.size();
    if (entry.personality.enabled) {
        augmentation_config += "P";
        augmentation_config.push_back((char)(0x3 | 0x40));
        personality_offset_set += augmentation_config.size();
        augmentation_config.push_back((char)0);
    }
    if (entry.use_fde.enabled)
        augmentation_config += "R";

    if (entry.lsda.enabled)
        augmentation_config += "L";
    data.push_back((uint8_t*)augmentation_config.c_str(), augmentation_config.size());
    data.push_back(make_uleb128(entry.code_alignment_factor));
    data.push_back(make_sleb128(entry.data_alignment_factor));
    data.push_back(entry.return_address_register);
    if (entry.augmentation_remainder.enabled || entry.personality.enabled)
        data.push_back(make_uleb128(entry.personality.enabled ? sizeof(uint32_t) : 0 + augmentation_data.size()));
    if (entry.personality.enabled) {
        data[personality_offset_set] = augmentation_data.size();
        auto personality_offset = make_uleb128(entry.personality.plt_entry);
        data.push_back(make_uleb128(augmentation_data.size() + personality_offset.size()));
        data.push_back(personality_offset);
    } else {
        data.push_back(make_uleb128(augmentation_data.size()));
    }
    data.push_back(augmentation_data);
    data.push_back(instructions);
    size_t cie_length_min = data.size() + sizeof(uint32_t) + sizeof(uint8_t);
    if (uint8_t padding = cie_length_min % sizeof(uint64_t)) {
        padding = sizeof(uint64_t) - padding;
        for (uint8_t i = 0; i < padding; i++)
            data.push_back(0);
        cie_length_min += padding;
    }
    if (cie_length_min < data.size())
        throw std::runtime_error("CIE length overflow");
    if (cie_length_min < UINT32_MAX) {
        struct {
            uint32_t length;
            uint32_t id;
            uint8_t version;
        } cie;
        cie.length = data.size() + sizeof(cie) - sizeof(cie.length);
        cie.id = entry.id;
        cie.version = entry.version;
        data.push_front((uint8_t*)&cie, sizeof(cie));
    } else {
        struct {
            uint32_t length = 0xffffffff;
            uint64_t length64;
            uint32_t id;
            uint8_t version;
        } cie;
        cie.length64 = data.size() + sizeof(cie) - sizeof(cie.length) - sizeof(cie.length64);
        cie.id = entry.id;
        cie.version = entry.version;
        data.push_front((uint8_t*)&cie, sizeof(cie));
    }
    return data;
}

struct DWARF {
    list_array<uint8_t> data;
    size_t fun_pointers_fix_pos;
    size_t fde_off;

    void patch_function_address(uint64_t address) {
        (*(uint64_t*)&data[fun_pointers_fix_pos]) = address;
    }
    void patch_function_size(uint64_t length) {
        (*(uint64_t*)&data[fun_pointers_fix_pos + sizeof(uint64_t)]) = length;
    }
};
inline void* get_cie_from_fde(void* ptr) {
    struct basic_fde {
        uint32_t length;
    }& fde = (basic_fde&)ptr;

    if (fde.length != 0xffffffff) {
        struct basic_fde {
            uint32_t length;
            int32_t CIE_pointer;
            uint64_t function_pointer;
            uint64_t function_size;
        }& fde = (basic_fde&)ptr;
        return (uint8_t*)&fde + fde.CIE_pointer;
    } else {
        struct extended_fde {
            uint32_t length;
            uint64_t length64;
            int32_t CIE_pointer;
            uint64_t function_pointer;
            uint64_t function_size;
        }& fde = (extended_fde&)ptr;
        return (uint8_t*)&fde + fde.CIE_pointer;
    }
}
inline DWARF complete_dwarf(const list_array<uint8_t>& CIE,
                            const FDE_entry& entry,
                            const list_array<uint8_t>& augmentation_data,
                            const list_array<uint8_t>& instructions) {
    DWARF result;
    result.data = CIE;
    result.fde_off = result.data.size();
    list_array<uint8_t> data;
    data.push_back(make_uleb128(augmentation_data.size()));
    data.push_back(augmentation_data);
    data.push_back(instructions);
    size_t fde_length_min = data.size() + sizeof(uint32_t) + sizeof(uint8_t);
    if (uint8_t padding = fde_length_min % sizeof(uint64_t)) {
        padding = sizeof(uint64_t) - padding;
        for (uint8_t i = 0; i < padding; i++)
            data.push_back(0);
        fde_length_min += padding;
    }
    if (fde_length_min < data.size())
        throw std::runtime_error("FDE length overflow");
    if (fde_length_min < UINT32_MAX) {
        struct {
            uint32_t length;
            int32_t CIE_pointer;
            uint64_t function_pointer;
            uint64_t function_size;
        } fde;
        fde.length = data.size() + sizeof(fde) - sizeof(fde.length);
        fde.CIE_pointer = -(data.size() + sizeof(fde));
        fde.function_pointer = entry.function_pointer;
        fde.function_size = entry.function_size;
        data.push_front((uint8_t*)&fde, sizeof(fde));
    } else {
        struct {
            uint32_t length = 0xffffffff;
            uint64_t length64;
            int32_t CIE_pointer;
            uint64_t function_pointer;
            uint64_t function_size;
        } fde;
        fde.length64 = data.size() + sizeof(fde) - sizeof(fde.length) - sizeof(fde.length64);
        fde.CIE_pointer = -(data.size() + sizeof(fde));
        fde.function_pointer = entry.function_pointer;
        fde.function_size = entry.function_size;
        data.push_front((uint8_t*)&fde, sizeof(fde));
    }
    result.data.push_back(std::move(data));
    result.fun_pointers_fix_pos = result.data.size() - 16;
    result.data.push_back({0, 0, 0, 0, 0, 0, 0, 0});
    return result;
}

#pragma pack(pop)
#endif /* SRC_RUN_TIME_ASM_CASM_DWARF_BUILDER */
