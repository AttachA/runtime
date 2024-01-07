// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_RUN_TIME_ATTACHA_ABI_STRUCTS
    #define SRC_RUN_TIME_ATTACHA_ABI_STRUCTS
    #include <cassert>
    #include <chrono>
    #include <cstdint>
    #include <exception>
    #include <unordered_map>
    #include <unordered_set>

    #include <library/list_array.hpp>
    #include <util/array.hpp>
    #include <util/enum_helper.hpp>
    #include <util/exceptions.hpp>
    #include <util/hash.hpp>
    #include <util/link_garbage_remover.hpp>
    #include <util/shared_ptr.hpp>
    #include <util/ustring.hpp>

namespace art {
    struct Task;
    class Generator;
    class FuncEnvironment;

    ENUM_t(
        Opcode,
        uint8_t,
        noting,
        create_saarr,
        remove,
        sum,
        minus,
        div,
        mul,
        rest,
        bit_xor,
        bit_or,
        bit_and,
        bit_not,
        bit_shift_left,
        bit_shift_right,
        log_not,
        compare,
        jump,
        arg_set,
        call,
        call_self,
        call_local,
        call_and_ret,
        call_self_and_ret,
        call_local_and_ret,
        ret,
        ret_take,
        ret_noting,
        copy,
        move,
        arr_op,
        debug_break,
        force_debug_break,
        throw_ex,
        as,
        is,
        store_bool, //store bool from value for if statements, set false if type is noting, numeric types is zero and containers is empty, if another then true

        load_bool, //used if need save equality result, set numeric type as 1 or 0

        inline_native,
        call_value_function,
        call_value_function_id,
        call_value_function_and_ret,
        call_value_function_id_and_ret,
        static_call_value_function,
        static_call_value_function_and_ret,
        static_call_value_function_id,
        static_call_value_function_id_and_ret,
        set_structure_value,
        get_structure_value,
        explicit_await,
        generator_get, //get value from generator or async task

        yield,

        handle_begin,
        handle_catch,
        handle_finally,
        handle_end,
        value_hold,
        value_unhold,
        is_gc,
        to_gc,
        localize_gc,
        from_gc,
        table_jump,
        xarray_slice, //faarr and saarr slice by creating reference to original array with moved pointer and new size

        store_constant,
        get_reference,
        make_as_const,
        remove_const_protect,
        copy_un_constant,
        copy_un_reference,
        move_un_reference,
        remove_qualifiers,

        global_get,
        global_set,
        global_take,
        global_move,
        global_copy_to_constants,
        global_copy_to_static,
        global_reference_to_constants,
        global_reference_to_static
    );

    ENUM_t(
        OpcodeArray,
        uint8_t,
        set,
        insert,
        push_end,
        push_start,
        insert_range,

        get,
        take,
        take_end,
        take_start,
        get_range,
        take_range,


        pop_end,
        pop_start,
        remove_item,
        remove_range,

        resize,
        resize_default,


        reserve_push_end,
        reserve_push_start,
        commit,
        decommit,
        remove_reserved,
        size
    );

    ENUM_t(
        ArrCheckMode,
        uint8_t,
        no_check,
        check,
        no_throw_check
    );

    ENUM_t(
        TableJumpCheckFailAction,
        uint8_t,
        jump_specified,
        throw_exception,
        unchecked
    );

    union OpArrFlags {
        struct {
            uint8_t move_mode : 1;
            ArrCheckMode checked : 2;
        };

        uint8_t raw;
    };

    union TableJumpFlags {
        struct {
            uint8_t is_signed : 1;
            TableJumpCheckFailAction too_large : 2;
            TableJumpCheckFailAction too_small : 2;
        };

        uint8_t raw;
    };

    ENUM_ta(
        JumpCondition,
        uint8_t,
        (is_more = is_unsigned_more)(is_lower = is_unsigned_lower)(is_lower_or_eq = is_unsigned_lower_or_eq)(is_more_or_eq = is_unsigned_more_or_eq),

        no_condition,
        is_equal,
        is_not_equal,
        is_unsigned_more,
        is_unsigned_lower,
        is_unsigned_lower_or_eq,
        is_unsigned_more_or_eq,
        is_signed_more,
        is_signed_lower,
        is_signed_lower_or_eq,
        is_signed_more_or_eq,
        is_zero
    );

    struct Command {
        Command() {
            code = Opcode::noting;
            is_gc_mode = false;
            static_mode = false;
        }

        Command(Opcode op, bool gc_mode = false, bool set_static_mode = false) {
            code = op;
            is_gc_mode = gc_mode;
            static_mode = set_static_mode;
        }

        Opcode code;
        uint8_t is_gc_mode : 1;
        uint8_t static_mode : 1;
    };

    union CallFlags {
        struct {
            uint8_t always_dynamic : 1; //prevent holding old function reference
            uint8_t async_mode : 1;
            uint8_t use_result : 1;
            uint8_t : 5;
        };

        uint8_t encoded = 0;
    };

    struct RFLAGS {
        uint16_t carry : 1;
        uint16_t : 1;
        uint16_t parity : 1;
        uint16_t : 1;
        uint16_t auxiliary_carry : 1;
        uint16_t : 1;
        uint16_t zero : 1;
        uint16_t sign_f : 1;
        uint16_t tf : 1;
        uint16_t ief : 1;
        uint16_t direction : 1;
        uint16_t overflow : 1;
        uint16_t iopl : 1;
        uint16_t nt : 1;
        uint16_t : 1;

        struct off_left {
            static constexpr uint8_t nt = 13;
            static constexpr uint8_t iopl = 12;
            static constexpr uint8_t overflow = 11;
            static constexpr uint8_t direction = 10;
            static constexpr uint8_t ief = 9;
            static constexpr uint8_t tf = 8;
            static constexpr uint8_t sign_f = 7;
            static constexpr uint8_t zero = 6;
            static constexpr uint8_t auxiliary_carry = 4;
            static constexpr uint8_t parity = 2;
            static constexpr uint8_t carry = 0;
        };

        struct bit {
            static constexpr uint16_t nt = 0x2000;
            static constexpr uint16_t iopl = 0x1000;
            static constexpr uint16_t overflow = 0x800;
            static constexpr uint16_t direction = 0x400;
            static constexpr uint16_t ief = 0x200;
            static constexpr uint16_t tf = 0x100;
            static constexpr uint16_t sign_f = 0x80;
            static constexpr uint16_t zero = 0x40;
            static constexpr uint16_t auxiliary_carry = 0x10;
            static constexpr uint16_t parity = 0x4;
            static constexpr uint16_t carry = 0x1;
        };
    };

    ENUM_t(
        VType,
        uint8_t,
        noting,
        boolean,
        i8,
        i16,
        i32,
        i64,
        ui8,
        ui16,
        ui32,
        ui64,
        flo,
        doub,
        //character,//char32utf32,  TO-DO
        raw_arr_i8,
        raw_arr_i16,
        raw_arr_i32,
        raw_arr_i64,
        raw_arr_ui8,
        raw_arr_ui16,
        raw_arr_ui32,
        raw_arr_ui64,
        raw_arr_flo,
        raw_arr_doub,
        uarr,
        string, //Convert to struct_
        async_res,
        undefined_ptr,
        except_value, //default from except call
        faarr,        //fixed any array
        saarr,        //stack fixed any array //only local, cannot returned, cannot be used with lgr, cannot be passed as arguments

        struct_, //like c++ class, but with dynamic abilities

        type_identifier,
        function,
        map,        //unordered_map<any,any,art::hash<any>
        set,        //unordered_set<any>
        time_point, //std::chrono::high_resolution_clock::time_point
        generator,
        any_obj, //valid only for structures
    );

    ENUM_t(
        ValuePos,
        uint8_t,
        in_enviro,
        in_arguments,
        in_static,
        in_constants
    );

    struct ValueIndexPos {
        uint16_t index;
        ValuePos pos = ValuePos::in_enviro;

        ValueIndexPos()
            : index(0) {}

        ValueIndexPos(uint16_t index, ValuePos pos)
            : index(index), pos(pos) {}

        ValueIndexPos(const ValueIndexPos& copy)
            : index(copy.index), pos(copy.pos) {}

        void operator=(const ValueIndexPos& copy) {
            index = copy.index;
            pos = copy.pos;
        }

        bool operator==(const ValueIndexPos& compare) {
            return index == compare.index && pos == compare.pos;
        }

        bool operator!=(const ValueIndexPos& compare) {
            return index != compare.index || pos != compare.pos;
        }
    };

    inline ValueIndexPos operator""_env(unsigned long long index) {
        assert(index <= UINT16_MAX);
        return ValueIndexPos(index, ValuePos::in_enviro);
    }

    inline ValueIndexPos operator""_arg(unsigned long long index) {
        assert(index <= UINT16_MAX);
        return ValueIndexPos(index, ValuePos::in_arguments);
    }

    inline ValueIndexPos operator""_sta(unsigned long long index) {
        assert(index <= UINT16_MAX);
        return ValueIndexPos(index, ValuePos::in_static);
    }

    inline ValueIndexPos operator""_con(unsigned long long index) {
        assert(index <= UINT16_MAX);
        return ValueIndexPos(index, ValuePos::in_constants);
    }

    struct FunctionMetaFlags {
        uint64_t length; //length including meta

        struct {
            bool vec128_0 : 1;
            bool vec128_1 : 1;
            bool vec128_2 : 1;
            bool vec128_3 : 1;
            bool vec128_4 : 1;
            bool vec128_5 : 1;
            bool vec128_6 : 1;
            bool vec128_7 : 1;
            bool vec128_8 : 1;
            bool vec128_9 : 1;
            bool vec128_10 : 1;
            bool vec128_11 : 1;
            bool vec128_12 : 1;
            bool vec128_13 : 1;
            bool vec128_14 : 1;
            bool vec128_15 : 1;
        } used_vec;

        bool can_be_unloaded : 1;
        bool is_translated : 1; //function that returns another function, used to implement generics, lambdas or dynamic functions
        bool has_local_functions : 1;
        bool has_debug_info : 1;
        bool is_cheap : 1;
        bool used_enviro_vals : 1;
        bool used_arguments : 1;
        bool used_static : 1;
        bool in_debug : 1;
        bool run_time_computable : 1; //in files always false
        bool is_patchable : 1;        //define function is patchable or not, if not patchable, in function header and footer excluded atomic usage count modification
                                      //10bits left
    };

    union ValueMeta {
        size_t encoded;

        struct {
            VType vtype;
            uint8_t use_gc : 1;
            uint8_t allow_edit : 1;
            uint8_t as_ref : 1;
            uint32_t val_len;
        };

        ValueMeta() = default;
        ValueMeta(const ValueMeta& copy) = default;

        ValueMeta(VType ty, bool gc = false, bool editable = true, uint32_t length = 0, bool as_ref = false)
            : encoded(0) {
            vtype = ty;
            use_gc = gc;
            allow_edit = editable;
            val_len = length;
            as_ref = as_ref;
        }

        ValueMeta(size_t enc) {
            encoded = enc;
        }

        art::ustring to_string() const {
            art::ustring ret;
            if (!allow_edit)
                ret += "const ";
            ret += enum_to_string(vtype);
            if (use_gc)
                ret += "^";
            if (as_ref)
                ret += "&";
            if (val_len)
                ret += "[" + std::to_string(val_len) + "]";
            return ret;
        }
    };
    class Structure;
    struct ValueItem;

    class ValueItemIterator {
        ValueItem& item;
        void* iterator_data;

        ValueItemIterator(ValueItem& item, void* iterator_data)
            : item(item), iterator_data(iterator_data) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        ValueItemIterator(ValueItem& item, bool end = false);

        ValueItemIterator(ValueItemIterator&& move) noexcept
            : item(move.item) {
            iterator_data = move.iterator_data;
            move.iterator_data = nullptr;
        }

        ValueItemIterator(const ValueItemIterator& move);
        ~ValueItemIterator();

        ValueItemIterator& operator++();
        ValueItemIterator operator++(int);
        ValueItem& operator*();
        ValueItem* operator->();


        operator ValueItem() const;
        ValueItem get() const;
        ValueItemIterator& operator=(const ValueItem& item);
        bool operator==(const ValueItemIterator& compare) const;
        bool operator!=(const ValueItemIterator& compare) const;
    };

    class ValueItemConstIterator {
        ValueItemIterator iterator;

    public:
        using iterator_category = std::forward_iterator_tag;

        ValueItemConstIterator(const ValueItem& item, bool end = false)
            : iterator(const_cast<ValueItem&>(item), end) {}

        ValueItemConstIterator(ValueItemConstIterator&& move) noexcept
            : iterator(std::move(move.iterator)) {}

        ValueItemConstIterator(const ValueItemConstIterator& copy)
            : iterator(copy.iterator) {}

        ~ValueItemConstIterator() = default;

        ValueItemConstIterator& operator++() {
            iterator.operator++();
            return *this;
        }

        ValueItemConstIterator operator++(int) {
            ValueItemConstIterator ret(*this);
            iterator.operator++();
            return ret;
        }

        const ValueItem& operator*() const {
            return const_cast<ValueItemIterator&>(iterator).operator*();
        }

        operator ValueItem() const;
        ValueItem get() const;

        bool operator==(const ValueItemConstIterator& compare) const {
            return iterator.operator==(compare.iterator);
        }

        bool operator!=(const ValueItemConstIterator& compare) const {
            return iterator.operator!=(compare.iterator);
        }
    };

    struct as_reference_t {};

    constexpr inline as_reference_t as_reference = {};

    struct no_copy_t {};

    constexpr inline no_copy_t no_copy = {};

    struct ValueItem {
        void* val;
        ValueMeta meta;
        ValueItem(std::nullptr_t);
        ValueItem(bool val);
        ValueItem(int8_t val);
        ValueItem(uint8_t val);
        ValueItem(int16_t val);
        ValueItem(uint16_t val);
        ValueItem(int32_t val);
        ValueItem(uint32_t val);
        ValueItem(int64_t val);
        ValueItem(uint64_t val);
        ValueItem(float val);
        ValueItem(double val);
    #ifdef _WIN32
        ValueItem(long val)
            : ValueItem(int32_t(val)) {}

        ValueItem(unsigned long val)
            : ValueItem(uint32_t(val)) {}
    #else
        ValueItem(long long val)
            : ValueItem(int64_t(val)) {}

        ValueItem(unsigned long long val)
            : ValueItem(uint64_t(val)) {}
    #endif
        ValueItem(const art::ustring& val);
        ValueItem(art::ustring&& val);
        ValueItem(const char* str);
        ValueItem(const list_array<ValueItem>& val);
        ValueItem(list_array<ValueItem>&& val);
        ValueItem(ValueItem* vals, uint32_t len);
        ValueItem(ValueItem* vals, uint32_t len, no_copy_t);
        ValueItem(ValueItem* vals, uint32_t len, as_reference_t);
        ValueItem(void* undefined_ptr);

        ValueItem(const int8_t* vals, uint32_t len);
        ValueItem(const uint8_t* vals, uint32_t len);
        ValueItem(const int16_t* vals, uint32_t len);
        ValueItem(const uint16_t* vals, uint32_t len);
        ValueItem(const int32_t* vals, uint32_t len);
        ValueItem(const uint32_t* vals, uint32_t len);
        ValueItem(const int64_t* vals, uint32_t len);
        ValueItem(const uint64_t* vals, uint32_t len);
        ValueItem(const float* vals, uint32_t len);
        ValueItem(const double* vals, uint32_t len);

        ValueItem(int8_t* vals, uint32_t len, no_copy_t);
        ValueItem(uint8_t* vals, uint32_t len, no_copy_t);
        ValueItem(int16_t* vals, uint32_t len, no_copy_t);
        ValueItem(uint16_t* vals, uint32_t len, no_copy_t);
        ValueItem(int32_t* vals, uint32_t len, no_copy_t);
        ValueItem(uint32_t* vals, uint32_t len, no_copy_t);
        ValueItem(int64_t* vals, uint32_t len, no_copy_t);
        ValueItem(uint64_t* vals, uint32_t len, no_copy_t);
        ValueItem(float* vals, uint32_t len, no_copy_t);
        ValueItem(double* vals, uint32_t len, no_copy_t);

        ValueItem(int8_t* vals, uint32_t len, as_reference_t);
        ValueItem(uint8_t* vals, uint32_t len, as_reference_t);
        ValueItem(int16_t* vals, uint32_t len, as_reference_t);
        ValueItem(uint16_t* vals, uint32_t len, as_reference_t);
        ValueItem(int32_t* vals, uint32_t len, as_reference_t);
        ValueItem(uint32_t* vals, uint32_t len, as_reference_t);
        ValueItem(int64_t* vals, uint32_t len, as_reference_t);
        ValueItem(uint64_t* vals, uint32_t len, as_reference_t);
        ValueItem(float* vals, uint32_t len, as_reference_t);
        ValueItem(double* vals, uint32_t len, as_reference_t);
        ValueItem(class Structure*, no_copy_t);

        template <size_t len>
        ValueItem(ValueItem (&vals)[len])
            : ValueItem(vals, len) {}

        ValueItem(const art::typed_lgr<Task>& task);
        ValueItem(const art::shared_ptr<Generator>& generator);
        ValueItem(const std::initializer_list<int8_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<uint8_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<int16_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<uint16_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<int32_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<uint32_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<int64_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<uint64_t>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<float>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<double>& args)
            : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
        ValueItem(const std::initializer_list<ValueItem>& args);
        ValueItem(const std::exception_ptr&);
        ValueItem(const std::chrono::high_resolution_clock::time_point&);
        ValueItem(const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& map);
        ValueItem(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&& map);
        ValueItem(const std::unordered_set<ValueItem, art::hash<ValueItem>>& set);
        ValueItem(std::unordered_set<ValueItem, art::hash<ValueItem>>&& set);


        ValueItem(const art::shared_ptr<FuncEnvironment>&);

        ValueItem() {
            val = nullptr;
            meta.encoded = 0;
        }

        ValueItem(ValueItem&& move);
        ValueItem(const void* val, ValueMeta meta);
        ValueItem(void* val, ValueMeta meta, as_reference_t);
        ValueItem(void* val, ValueMeta meta, no_copy_t);
        ValueItem(VType);
        ValueItem(ValueMeta);
        ValueItem(const ValueItem&);

        ValueItem(ValueItem& ref, as_reference_t);
        ValueItem(bool& val, as_reference_t);
        ValueItem(int8_t& val, as_reference_t);
        ValueItem(uint8_t& val, as_reference_t);
        ValueItem(int16_t& val, as_reference_t);
        ValueItem(uint16_t& val, as_reference_t);
        ValueItem(int32_t& val, as_reference_t);
        ValueItem(uint32_t& val, as_reference_t);
        ValueItem(int64_t& val, as_reference_t);
        ValueItem(uint64_t& val, as_reference_t);
        ValueItem(float& val, as_reference_t);
        ValueItem(double& val, as_reference_t);
        ValueItem(class Structure*, as_reference_t);
        ValueItem(art::ustring& val, as_reference_t);
        ValueItem(list_array<ValueItem>& val, as_reference_t);

        ValueItem(std::exception_ptr&, as_reference_t);
        ValueItem(std::chrono::high_resolution_clock::time_point&, as_reference_t);
        ValueItem(std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&, as_reference_t);
        ValueItem(std::unordered_set<ValueItem, art::hash<ValueItem>>&, as_reference_t);
        ValueItem(art::typed_lgr<Task>& task, as_reference_t);
        ValueItem(art::shared_ptr<Generator>& generator, as_reference_t);
        ValueItem(ValueMeta&, as_reference_t);
        ValueItem(art::shared_ptr<FuncEnvironment>&, as_reference_t);


        ValueItem(const ValueItem& ref, as_reference_t);
        ValueItem(const bool& val, as_reference_t);
        ValueItem(const int8_t& val, as_reference_t);
        ValueItem(const uint8_t& val, as_reference_t);
        ValueItem(const int16_t& val, as_reference_t);
        ValueItem(const uint16_t& val, as_reference_t);
        ValueItem(const int32_t& val, as_reference_t);
        ValueItem(const uint32_t& val, as_reference_t);
        ValueItem(const int64_t& val, as_reference_t);
        ValueItem(const uint64_t& val, as_reference_t);
        ValueItem(const float& val, as_reference_t);
        ValueItem(const double& val, as_reference_t);
        ValueItem(const class Structure*, as_reference_t);
        ValueItem(const art::ustring& val, as_reference_t);
        ValueItem(const list_array<ValueItem>& val, as_reference_t);

        ValueItem(const std::exception_ptr&, as_reference_t);
        ValueItem(const std::chrono::high_resolution_clock::time_point&, as_reference_t);
        ValueItem(const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&, as_reference_t);
        ValueItem(const std::unordered_set<ValueItem, art::hash<ValueItem>>&, as_reference_t);
        ValueItem(const art::typed_lgr<Task>& task, as_reference_t);
        ValueItem(const art::shared_ptr<Generator>& generator, as_reference_t);
        ValueItem(const ValueMeta&, as_reference_t);
        ValueItem(const art::shared_ptr<FuncEnvironment>&, as_reference_t);


        ValueItem(array_t<bool>&& val);
        ValueItem(array_t<int8_t>&& val);
        ValueItem(array_t<uint8_t>&& val);
        ValueItem(array_t<char>&& val);
        ValueItem(array_t<int16_t>&& val);
        ValueItem(array_t<uint16_t>&& val);
        ValueItem(array_t<int32_t>&& val);
        ValueItem(array_t<uint32_t>&& val);
        ValueItem(array_t<int64_t>&& val);
        ValueItem(array_t<uint64_t>&& val);
        ValueItem(array_t<float>&& val);
        ValueItem(array_t<double>&& val);
        ValueItem(array_t<ValueItem>&& val);

        ValueItem(const array_t<bool>& val);
        ValueItem(const array_t<int8_t>& val);
        ValueItem(const array_t<uint8_t>& val);
        ValueItem(const array_t<char>& val);
        ValueItem(const array_t<int16_t>& val);
        ValueItem(const array_t<uint16_t>& val);
        ValueItem(const array_t<int32_t>& val);
        ValueItem(const array_t<uint32_t>& val);
        ValueItem(const array_t<int64_t>& val);
        ValueItem(const array_t<uint64_t>& val);
        ValueItem(const array_t<float>& val);
        ValueItem(const array_t<double>& val);
        ValueItem(const array_t<ValueItem>& val);

        ValueItem(const array_ref_t<bool>& val);
        ValueItem(const array_ref_t<int8_t>& val);
        ValueItem(const array_ref_t<uint8_t>& val);
        ValueItem(const array_ref_t<char>& val);
        ValueItem(const array_ref_t<int16_t>& val);
        ValueItem(const array_ref_t<uint16_t>& val);
        ValueItem(const array_ref_t<int32_t>& val);
        ValueItem(const array_ref_t<uint32_t>& val);
        ValueItem(const array_ref_t<int64_t>& val);
        ValueItem(const array_ref_t<uint64_t>& val);
        ValueItem(const array_ref_t<float>& val);
        ValueItem(const array_ref_t<double>& val);
        ValueItem(const array_ref_t<ValueItem>& val);


        ValueItem& operator=(const ValueItem& copy);
        ValueItem& operator=(ValueItem&& copy);
        ~ValueItem();
        int8_t compare(const ValueItem& cmp) const;
        bool operator<(const ValueItem& cmp) const;
        bool operator>(const ValueItem& cmp) const;
        bool operator==(const ValueItem& cmp) const;
        bool operator!=(const ValueItem& cmp) const;
        bool operator>=(const ValueItem& cmp) const;
        bool operator<=(const ValueItem& cmp) const;


        ValueItem& operator+=(const ValueItem& op);
        ValueItem& operator-=(const ValueItem& op);
        ValueItem& operator*=(const ValueItem& op);
        ValueItem& operator/=(const ValueItem& op);
        ValueItem& operator%=(const ValueItem& op);
        ValueItem& operator^=(const ValueItem& op);
        ValueItem& operator&=(const ValueItem& op);
        ValueItem& operator|=(const ValueItem& op);
        ValueItem& operator<<=(const ValueItem& op);
        ValueItem& operator>>=(const ValueItem& op);
        ValueItem& operator++();
        ValueItem& operator--();
        ValueItem& operator!();

        ValueItem operator+(const ValueItem& op) const;
        ValueItem operator-(const ValueItem& op) const;
        ValueItem operator*(const ValueItem& op) const;
        ValueItem operator/(const ValueItem& op) const;
        ValueItem operator^(const ValueItem& op) const;
        ValueItem operator&(const ValueItem& op) const;
        ValueItem operator|(const ValueItem& op) const;

        explicit operator Structure&();
        explicit operator std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&();
        explicit operator std::unordered_set<ValueItem, art::hash<ValueItem>>&();
        explicit operator art::typed_lgr<Task>&();
        explicit operator art::shared_ptr<Generator>&();
        explicit operator art::shared_ptr<FuncEnvironment>&();


        explicit operator bool() const;
        explicit operator int8_t() const;
        explicit operator uint8_t() const;
        explicit operator int16_t() const;
        explicit operator uint16_t() const;
        explicit operator int32_t() const;
        explicit operator uint32_t() const;
        explicit operator int64_t() const;
        explicit operator uint64_t() const;
        explicit operator float() const;
        explicit operator double() const;
    #ifdef _WIN32
        explicit operator long() const {
            return (long)(int32_t) * this;
        }

        explicit operator unsigned long() const {
            return (unsigned long)(uint32_t) * this;
        }
    #else
        explicit operator long long() const {
            return (long long)(int64_t) * this;
        }

        explicit operator unsigned long long() const {
            return (unsigned long long)(uint64_t) * this;
        }
    #endif
        explicit operator void*() const;
        explicit operator art::ustring() const;
        explicit operator list_array<ValueItem>() const;
        explicit operator ValueMeta() const;
        explicit operator std::exception_ptr() const;
        explicit operator std::chrono::high_resolution_clock::time_point() const;
        explicit operator const Structure&() const;
        explicit operator const std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&() const;
        explicit operator const std::unordered_set<ValueItem, art::hash<ValueItem>>&() const;
        explicit operator const art::typed_lgr<Task>&() const;
        explicit operator const art::shared_ptr<Generator>&() const;
        explicit operator const art::shared_ptr<FuncEnvironment>&() const;
        explicit operator const array_t<bool>() const;
        explicit operator const array_t<int8_t>() const;
        explicit operator const array_t<uint8_t>() const;
        explicit operator const array_t<char>() const;
        explicit operator const array_t<int16_t>() const;
        explicit operator const array_t<uint16_t>() const;
        explicit operator const array_t<int32_t>() const;
        explicit operator const array_t<uint32_t>() const;
        explicit operator const array_t<int64_t>() const;
        explicit operator const array_t<uint64_t>() const;
        explicit operator const array_t<float>() const;
        explicit operator const array_t<double>() const;
    #ifdef _WIN32
        explicit operator const array_t<long>() const;
    #endif
        explicit operator const array_t<ValueItem>() const;
        explicit operator const array_ref_t<bool>() const;
        explicit operator const array_ref_t<int8_t>() const;
        explicit operator const array_ref_t<uint8_t>() const;
        explicit operator const array_ref_t<char>() const;
        explicit operator const array_ref_t<int16_t>() const;
        explicit operator const array_ref_t<uint16_t>() const;
        explicit operator const array_ref_t<int32_t>() const;
        explicit operator const array_ref_t<uint32_t>() const;
        explicit operator const array_ref_t<int64_t>() const;
        explicit operator const array_ref_t<uint64_t>() const;
        explicit operator const array_ref_t<float>() const;
        explicit operator const array_ref_t<double>() const;
    #ifdef _WIN32
        explicit operator const array_ref_t<long>() const;
    #endif
        explicit operator const array_ref_t<ValueItem>() const;
        explicit operator array_ref_t<bool>();
        explicit operator array_ref_t<int8_t>();
        explicit operator array_ref_t<uint8_t>();
        explicit operator array_ref_t<int16_t>();
        explicit operator array_ref_t<uint16_t>();
        explicit operator array_ref_t<int32_t>();
        explicit operator array_ref_t<uint32_t>();
        explicit operator array_ref_t<int64_t>();
        explicit operator array_ref_t<uint64_t>();
        explicit operator array_ref_t<float>();
        explicit operator array_ref_t<double>();
    #ifdef _WIN32
        explicit operator array_ref_t<long>();
    #endif
        explicit operator array_ref_t<ValueItem>();
        ValueItem* operator()(ValueItem* arguments, uint32_t arguments_size);
        ValueItem& getAsync();
        void getAsyncResult(ValueItem& res, uint64_t result_id);
        void getGeneratorResult(ValueItem& res);
        void*& getSourcePtr();
        const void*& getSourcePtr() const;
        void*& unRef();
        const void* const& unRef() const;
        art::shared_ptr<FuncEnvironment>* funPtr();
        const art::shared_ptr<FuncEnvironment>* funPtr() const;
        void make_gc();
        void localize_gc();
        void ungc();
        bool is_gc();

        size_t hash() const;
        size_t hash(uint32_t seed) const;
        ValueItem make_slice(uint32_t start, uint32_t end) const;


        ValueItem& operator[](const ValueItem& index);
        const ValueItem& operator[](const ValueItem& index) const;
        ValueItem get(const ValueItem& index) const;
        void set(const ValueItem& index, const ValueItem& value);
        bool has(const ValueItem& index) const;
        ValueItemIterator begin();
        ValueItemIterator end();
        ValueItemConstIterator begin() const;
        ValueItemConstIterator end() const;

        ValueItemConstIterator cbegin() const {
            return begin();
        }

        ValueItemConstIterator cend() const {
            return end();
        }

        size_t size() const;
    };

    typedef ValueItem* (*Environment)(ValueItem* args, uint32_t len);

    ENUM_t(ClassAccess, uint8_t,
           pub,   //anyone can use
           priv,  //main only
           prot,  //derived or main
           intern //internal, derived or main
    );

    struct StructureTag {
        art::ustring name;
        art::shared_ptr<FuncEnvironment> enviro;
        ValueItem value;
    };

    using MethodTag = StructureTag;
    using ValueTag = StructureTag;

    struct ValueInfo {
        art::ustring name;
        list_array<ValueTag>* optional_tags;
        size_t offset;
        ValueMeta type;
        uint16_t bit_used;
        uint8_t bit_offset : 7;
        bool inlined : 1;
        bool allow_abstract_assign : 1;
        bool zero_after_cleanup : 1;
        ClassAccess access : 2;
        ValueInfo();
        ValueInfo(const art::ustring& name, size_t offset, ValueMeta type, uint16_t bit_used, uint8_t bit_offset, bool inlined, bool allow_abstract_assign, ClassAccess access, const list_array<ValueTag>& tags, bool zero_after_cleanup = false);

        ~ValueInfo();

        ValueInfo(const ValueInfo& copy);
        ValueInfo(ValueInfo&& move);
        ValueInfo& operator=(const ValueInfo& copy);
        ValueInfo& operator=(ValueInfo&& move);
    };

    struct StructStaticValue {
        art::ustring name;
        list_array<ValueTag>* optional_tags;
        ValueItem value;
        ClassAccess access : 2;
    };

    namespace structure_helpers {
        template <typename T>
        T static_value_get(const char* ptr, size_t offset, uint16_t bit_used, uint8_t bit_offset) {
            if (sizeof(T) * 8 < bit_used)
                throw InvalidArguments("bit_used is too big for type");

            ptr += offset;
            ptr += bit_offset / 8;

            if ((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
                return *(T*)ptr;
            uint8_t bit_offset2 = bit_offset % 8;

            uint8_t used_bits = bit_used ? bit_used % 8 : 0;
            uint16_t used_bytes = bit_used ? bit_used / 8 : sizeof(T);
            if (used_bytes >= sizeof(T)) {
                used_bytes = sizeof(T);
                used_bits = 0;
            }

            char buffer[sizeof(T)]{0};
            for (uint8_t i = 0; i < used_bytes - 1; i++)
                buffer[i] = (ptr[i] >> bit_offset2) | (ptr[i + 1] << (8 - bit_offset2));

            buffer[used_bytes - 1] = buffer[used_bytes - 1] >> bit_offset2;
            buffer[used_bytes - 1] &= (1 << used_bits) - 1;
            return *(T*)buffer;
        }

        template <typename T>
        T& static_value_get_ref(char* ptr, size_t offset, uint16_t bit_used, uint8_t bit_offset) {
            if ((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
                throw InvalidArguments("bit_used is not aligned for type");
            ptr += offset;
            ptr += bit_offset / 8;
            return *(T*)ptr;
        }

        template <typename T>
        void static_value_set(char* ptr, size_t offset, uint16_t bit_used, uint8_t bit_offset, T value) {
            if (sizeof(T) * 8 < bit_used)
                throw InvalidArguments("bit_used is too big for type");
            ptr += offset;
            ptr += bit_offset / 8;
            if ((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
                *(T*)ptr = value;
            uint8_t bit_offset2 = bit_offset % 8;

            uint8_t used_bits = bit_used ? bit_used % 8 : 0;
            uint16_t used_bytes = bit_used ? bit_used / 8 : sizeof(T);
            if (used_bytes >= sizeof(T)) {
                used_bytes = sizeof(T);
                used_bits = 0;
            }

            char buffer[sizeof(T)]{0};
            (*(T*)buffer) = value;

            for (uint8_t i = 0; i < used_bytes - 1; i++)
                buffer[i] = (buffer[i] << bit_offset2) | (buffer[i + 1] >> (8 - bit_offset2));

            buffer[used_bytes - 1] = buffer[used_bytes - 1] << bit_offset2;
            buffer[used_bytes - 1] &= (1 << used_bits) - 1;
            for (uint8_t i = 0; i < used_bytes; i++)
                ptr[i] = (ptr[i] & ~(buffer[i] << bit_offset2)) | (buffer[i] << bit_offset2);
        }

        template <typename T>
        void static_value_set_ref(char* ptr, size_t offset, uint16_t bit_used, uint8_t bit_offset, T value) {
            if ((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
                throw InvalidArguments("bit_used is not aligned for type");
            ptr += offset;
            ptr += bit_offset / 8;
            *(T*)ptr = value;
        }

        template <typename T>
        ValueItem getRawArray(char* ptr, const ValueInfo& item) {
            if (item.inlined) {
                if (item.type.as_ref)
                    return ValueItem((T*)&static_value_get_ref<T*>(ptr, item.offset, 0, 0), item.type.val_len, as_reference);
                else
                    return ValueItem((T*)&static_value_get_ref<T*>(ptr, item.offset, 0, 0), item.type.val_len);
            } else {
                if (item.type.as_ref)
                    return ValueItem(static_value_get<T*>(ptr, item.offset, 0, item.bit_offset), item.type.val_len, as_reference);
                else
                    return ValueItem(static_value_get<T*>(ptr, item.offset, 0, item.bit_offset), item.type.val_len);
            }
        }

        template <typename T>
        ValueItem getRawArray(const char* ptr, const ValueInfo& item) {
            char* fake_ptr = const_cast<char*>(ptr);
            ValueItem result;
            if (item.inlined) {
                if (item.type.as_ref)
                    result = ValueItem((T*)&static_value_get_ref<T*>(fake_ptr, item.offset, 0, 0), item.type.val_len, as_reference);
                else
                    result = ValueItem((T*)&static_value_get_ref<T*>(fake_ptr, item.offset, 0, 0), item.type.val_len);
            } else {
                if (item.type.as_ref)
                    result = ValueItem(static_value_get<T*>(fake_ptr, item.offset, 0, item.bit_offset), item.type.val_len, as_reference);
                else
                    result = ValueItem(static_value_get<T*>(fake_ptr, item.offset, 0, item.bit_offset), item.type.val_len);
            }
            result.meta.allow_edit = false;
            return result;
        }

        template <typename T>
        ValueItem getType(char* ptr, const ValueInfo& item) {
            if (item.type.as_ref)
                return ValueItem(static_value_get_ref<T>(ptr, item.offset, 0, 0), as_reference);
            else
                return ValueItem(static_value_get_ref<T>(ptr, item.offset, 0, 0));
        }

        template <typename T>
        ValueItem getRawArrayRef(char* ptr, const ValueInfo& item) {
            if (item.inlined)
                return ValueItem((T*)&static_value_get_ref<T*>(ptr, item.offset, 0, 0), item.type.val_len, as_reference);
            else
                return ValueItem(static_value_get<T*>(ptr, item.offset, 0, item.bit_offset), item.type.val_len, as_reference);
        }

        template <typename T>
        ValueItem getTypeRef(char* ptr, const ValueInfo& item) {
            return ValueItem(static_value_get_ref<T>(ptr, item.offset, 0, 0), as_reference);
        }

        ValueItem _static_value_get_ref(char* ptr, const ValueInfo& item);
        ValueItem _static_value_get(char* ptr, const ValueInfo& item);

        void _static_value_set_ref(char* ptr, const ValueInfo& item, ValueItem&);
        void _static_value_set(char* ptr, const ValueInfo& item, ValueItem&);

        void cleanup_item(char* ptr, ValueInfo& item);
        int8_t compare_items(char* ptr_a, const ValueInfo& item_a, char* ptr_b, const ValueInfo& item_b);
        void copy_items(char* ptr_a, char* ptr_b, ValueInfo* items, size_t items_size);
        void copy_items_abstract(char* ptr_a, ValueInfo* items_a, size_t items_a_size, char* ptr_b, ValueInfo* items_b, size_t items_b_size);
    }

    struct MethodInfo {
        struct Optional {
            list_array<ValueMeta> return_values;
            list_array<list_array<std::pair<ValueMeta, art::ustring>>> arguments;
            list_array<StructureTag> tags;
        };

        art::shared_ptr<FuncEnvironment> ref;
        art::ustring name;
        art::ustring owner_name;
        Optional* optional;
        ClassAccess access : 2;
        bool deletable : 1;
        MethodInfo();
        MethodInfo(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name);
        MethodInfo(const art::ustring& name, art::shared_ptr<FuncEnvironment> method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name);

        ~MethodInfo();
        MethodInfo(const MethodInfo& copy);
        MethodInfo(MethodInfo&& move);
        MethodInfo& operator=(const MethodInfo& copy);
        MethodInfo& operator=(MethodInfo&& move);
    };

    struct AttachAVirtualTable {
        Environment destructor; //args: Structure* structure
        Environment copy;       //args: ValueItem self, ValueItem src
        Environment move;       //args: Structure* dst, Structure* src, bool at_construct
        Environment compare;    //args: Structure* first, Structure* second, return: -1 if first < second, 0 if first == second, 1 if first > second
        size_t structure_bytes;
        size_t call_table_size;
        size_t value_table_size;
        bool allow_auto_copy : 1; //allow to copy values by value table if copy operation not implemented(null)
        char data[];

        //{
        //  Environment[call_table_size] table;
        //  MethodInfo [call_table_size] table_additional_info;
        //  ValueInfo [value_table_size] table_additional_info;
        //	art::shared_ptr<FuncEnvironment> holder_destructor;
        //	art::shared_ptr<FuncEnvironment> holder_copy;
        //	art::shared_ptr<FuncEnvironment> holder_move;
        //  art::ustring name;
        //	list_array<StructureTag>* tags;//can be null
        //}
        list_array<StructureTag>* getStructureTags();
        list_array<MethodTag>* getMethodTags(uint64_t index);
        list_array<MethodTag>* getMethodTags(const art::ustring& name, ClassAccess access);

        list_array<list_array<std::pair<ValueMeta, art::ustring>>>* getMethodArguments(uint64_t index);
        list_array<list_array<std::pair<ValueMeta, art::ustring>>>* getMethodArguments(const art::ustring& name, ClassAccess access);

        list_array<ValueMeta>* getMethodReturnValues(uint64_t index);
        list_array<ValueMeta>* getMethodReturnValues(const art::ustring& name, ClassAccess access);

        MethodInfo* getMethodsInfo(uint64_t& size);
        const MethodInfo* getMethodsInfo(uint64_t& size) const;
        MethodInfo& getMethodInfo(uint64_t index);
        MethodInfo& getMethodInfo(const art::ustring& name, ClassAccess access);
        const MethodInfo& getMethodInfo(uint64_t index) const;
        const MethodInfo& getMethodInfo(const art::ustring& name, ClassAccess access) const;

        Environment* getMethods(uint64_t& size);
        Environment getMethod(uint64_t index) const;
        Environment getMethod(const art::ustring& name, ClassAccess access) const;

        uint64_t getMethodIndex(const art::ustring& name, ClassAccess access) const;
        bool hasMethod(const art::ustring& name, ClassAccess access) const;

        static AttachAVirtualTable* create(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy);
        static void destroy(AttachAVirtualTable* table);

        art::ustring getName() const;
        void setName(const art::ustring& name);

    #pragma region Values
        ValueInfo* getValuesInfo(uint64_t& size);
        ValueInfo& getValueInfo(uint64_t index);
        const ValueInfo& getValueInfo(uint64_t index) const;
        ValueInfo& getValueInfo(const art::ustring& name, ClassAccess access);
        const ValueInfo& getValueInfo(const art::ustring& name, ClassAccess access) const;
        list_array<ValueTag>* getValueTags(uint64_t index);
        list_array<ValueTag>* getValueTags(const art::ustring& name, ClassAccess access);

        ValueItem getValue(void* self, uint64_t index) const;
        ValueItem getValue(void* self, const art::ustring& name, ClassAccess access) const;

        ValueItem getValueRef(void* self, uint64_t index) const;
        ValueItem getValueRef(void* self, const art::ustring& name, ClassAccess access) const;

        void setValue(void* self, uint64_t index, ValueItem& value) const;
        void setValue(void* self, const art::ustring& name, ClassAccess access, ValueItem& value) const;

        bool hasValue(const art::ustring& name, ClassAccess access) const;
        uint64_t getValueIndex(const art::ustring& name, ClassAccess access) const;
    #pragma endregion


    #pragma region static_values
        StructStaticValue* getStaticValue(const art::ustring& name, ClassAccess access, bool add_new);
        bool hasStaticValue(const art::ustring& name, ClassAccess access) const;
        list_array<StructStaticValue>& staticValues();
        list_array<StructureTag>* getStaticValueTags(const art::ustring& name, ClassAccess access);
    #pragma endregion

    private:
        struct AfterMethods {
            art::ustring name;
            list_array<StructStaticValue> static_values;
            list_array<StructureTag>* tags;
            art::shared_ptr<FuncEnvironment> destructor;
            art::shared_ptr<FuncEnvironment> copy;
            art::shared_ptr<FuncEnvironment> move;
            art::shared_ptr<FuncEnvironment> compare;
        };

        MethodInfo* getMethodsInfo() const;
        ValueInfo* getValuesInfo() const;
        AttachAVirtualTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy);
        ~AttachAVirtualTable();

    public:
        AfterMethods* getAfterMethods();
        const AfterMethods* getAfterMethods() const;
    };

    struct AttachADynamicVirtualTable {
        art::shared_ptr<FuncEnvironment> destructor; //args: Structure* structure
        art::shared_ptr<FuncEnvironment> copy;       //args: Structure* dst, Structure* src, bool at_construct
        art::shared_ptr<FuncEnvironment> move;       //args: Structure* dst, Structure* src, bool at_construct
        art::shared_ptr<FuncEnvironment> compare;    //args: Structure* first, Structure* second, return: -1 if first < second, 0 if first == second, 1 if first > second
        list_array<MethodInfo> methods;
        list_array<ValueInfo> values;
        list_array<StructureTag>* tags;
        list_array<StructStaticValue> static_values;
        art::ustring name;
        size_t structure_bytes;
        bool allow_auto_copy : 1; //allow to copy values by value table if copy operation not implemented(null)
        AttachADynamicVirtualTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, size_t structure_bytes, bool allow_auto_copy);
        ~AttachADynamicVirtualTable();
        AttachADynamicVirtualTable(const AttachADynamicVirtualTable&);

    #pragma region Methods
        list_array<MethodTag>* getMethodTags(uint64_t index);
        list_array<MethodTag>* getMethodTags(const art::ustring& name, ClassAccess access);

        list_array<list_array<std::pair<ValueMeta, art::ustring>>>* getMethodArguments(uint64_t index);
        list_array<list_array<std::pair<ValueMeta, art::ustring>>>* getMethodArguments(const art::ustring& name, ClassAccess access);

        list_array<ValueMeta>* getMethodReturnValues(uint64_t index);
        list_array<ValueMeta>* getMethodReturnValues(const art::ustring& name, ClassAccess access);

        MethodInfo* getMethodsInfo(uint64_t& size);
        MethodInfo& getMethodInfo(uint64_t index);
        MethodInfo& getMethodInfo(const art::ustring& name, ClassAccess access);
        const MethodInfo& getMethodInfo(uint64_t index) const;
        const MethodInfo& getMethodInfo(const art::ustring& name, ClassAccess access) const;

        Environment getMethod(uint64_t index) const;
        Environment getMethod(const art::ustring& name, ClassAccess access) const;

        void addMethod(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name);
        void addMethod(const art::ustring& name, const art::shared_ptr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name);

        void removeMethod(const art::ustring& name, ClassAccess access);
        bool hasMethod(const art::ustring& name, ClassAccess access) const;
        uint64_t getMethodIndex(const art::ustring& name, ClassAccess access) const;

        void deriveMethods(AttachADynamicVirtualTable& parent, bool as_friend = false);
        void deriveMethods(AttachAVirtualTable& parent, bool as_friend = false);
    #pragma endregion
    #pragma region Values
        ValueInfo* getValuesInfo(uint64_t& size);
        ValueInfo& getValueInfo(uint64_t index);
        const ValueInfo& getValueInfo(uint64_t index) const;
        ValueInfo& getValueInfo(const art::ustring& name, ClassAccess access);
        const ValueInfo& getValueInfo(const art::ustring& name, ClassAccess access) const;
        list_array<ValueTag>* getValueTags(uint64_t index);
        list_array<ValueTag>* getValueTags(const art::ustring& name, ClassAccess access);

        ValueItem getValue(void* self, uint64_t index) const;
        ValueItem getValue(void* self, const art::ustring& name, ClassAccess access) const;

        ValueItem getValueRef(void* self, uint64_t index) const;
        ValueItem getValueRef(void* self, const art::ustring& name, ClassAccess access) const;

        void setValue(void* self, uint64_t index, ValueItem& value) const;
        void setValue(void* self, const art::ustring& name, ClassAccess access, ValueItem& value) const;

        void addValue(const ValueInfo&);

        void removeValue(const art::ustring& name, ClassAccess access);
        bool hasValue(const art::ustring& name, ClassAccess access) const;
        uint64_t getValueIndex(const art::ustring& name, ClassAccess access) const;

        void deriveValues(AttachADynamicVirtualTable& parent, bool as_friend = false);
        void deriveValues(AttachAVirtualTable& parent, bool as_friend = false);
    #pragma endregion

    #pragma region static_values
        StructStaticValue* getStaticValue(const art::ustring& name, ClassAccess access, bool add_new);
        bool hasStaticValue(const art::ustring& name, ClassAccess access) const;
        list_array<StructStaticValue>& staticValues();
        list_array<StructureTag>* getStaticValueTags(const art::ustring& name, ClassAccess access);
    #pragma endregion

        list_array<StructureTag>* getStructureTags();
        void derive(AttachADynamicVirtualTable& parent, bool as_friend = false);
        void derive(AttachAVirtualTable& parent, bool as_friend = false);
        void addTag(const art::ustring& name, const ValueItem& value);
        void addTag(const art::ustring& name, ValueItem&& value);
        void removeTag(const art::ustring& name);
    };

    //static values can be implemented by builder, allocate somewhere in memory and put references to functions, not structure
    class Structure {
    public:
        enum class VTableMode : uint8_t {
            AttachAVirtualTable = 0,
            AttachADynamicVirtualTable = 1, //destructor will delete the vtable
            ___unused = 2,
            undefined = 3
        };
        //return true if allowed
        static bool checkAccess(ClassAccess access, ClassAccess access_to_check);
        static AttachAVirtualTable* createAAVTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, const list_array<std::tuple<void*, VTableMode>>& derive_vtables, size_t structure_bytes, bool allow_auto_copy);
        static AttachADynamicVirtualTable* createAADVTable(list_array<MethodInfo>& methods, list_array<ValueInfo>& values, art::shared_ptr<FuncEnvironment> destructor, art::shared_ptr<FuncEnvironment> copy, art::shared_ptr<FuncEnvironment> move, art::shared_ptr<FuncEnvironment> compare, const list_array<std::tuple<void*, VTableMode>>& derive_vtables, size_t structure_bytes, bool allow_auto_copy);
        static void destroyVTable(void* table, VTableMode mode);


        Structure(void* structure_refrence, void* vtable, VTableMode table_mode, void (*self_cleanup)(void* self));
        ~Structure() noexcept(false);


        ValueItem static_value_get(size_t value_data_index) const;
        ValueItem static_value_get_ref(size_t value_data_index);
        void static_value_set(size_t value_data_index, ValueItem value);
        ValueItem dynamic_value_get(const art::ustring& name, ClassAccess access) const;
        ValueItem dynamic_value_get_ref(const art::ustring& name, ClassAccess access);
        void dynamic_value_set(const art::ustring& name, ClassAccess access, ValueItem value);

        uint64_t table_get_id(const art::ustring& name, ClassAccess access) const;
        Environment table_get(uint64_t fn_id) const;
        Environment table_get_dynamic(const art::ustring& name, ClassAccess access) const; //table_get(table_get_id(name, access))

        void add_method(const art::ustring& name, Environment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name);                             //only for AttachADynamicVirtualTable
        void add_method(const art::ustring& name, const art::shared_ptr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<std::pair<ValueMeta, art::ustring>>>& arguments, const list_array<MethodTag>& tags, const art::ustring& owner_name); //only for AttachADynamicVirtualTable

        bool has_method(const art::ustring& name, ClassAccess access) const;
        void remove_method(const art::ustring& name, ClassAccess access);
        art::shared_ptr<FuncEnvironment> get_method(uint64_t fn_id) const;
        art::shared_ptr<FuncEnvironment> get_method_dynamic(const art::ustring& name, ClassAccess access) const;

        void table_derive(void* vtable, VTableMode vtable_mode); //only for AttachADynamicVirtualTable
        void change_table(void* vtable, VTableMode vtable_mode); //only for AttachADynamicVirtualTable, destroy old vtable and use new one

        static void destruct(Structure* structure);
        static void copy(Structure* dst, Structure* src, bool at_construct);
        static Structure* copy(Structure* src);
        static void move(Structure* dst, Structure* src, bool at_construct);
        //static Structure* move(Structure* src);
        static int8_t compare(Structure* a, Structure* b);           //vtable
        static int8_t compare_reference(Structure* a, Structure* b); //reference compare
        static int8_t compare_object(Structure* a, Structure* b);    //compare by Item*`s
        static int8_t compare_full(Structure* a, Structure* b);      //compare && compare_object

        art::ustring get_name() const;

    #pragma region static_values
        StructStaticValue* getStaticValue(const art::ustring& name, ClassAccess access, bool add_new);
        bool hasStaticValue(const art::ustring& name, ClassAccess access) const;
        list_array<StructStaticValue>* staticValues();
        list_array<StructureTag>* getStaticValueTags(const art::ustring& name, ClassAccess access);
    #pragma endregion


        VTableMode vtable_mode : 2 = VTableMode::undefined;
        void* self;
        void (*self_cleanup)(void* self);
        void* vtable;
    };
}
#endif /* SRC_RUN_TIME_ATTACHA_ABI_STRUCTS */
