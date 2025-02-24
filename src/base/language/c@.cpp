// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/language/c@.hpp>
#include <boost/tokenizer.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/AttachA_CXX_struct.hpp>
#include <run_time/library/cxx/files.hpp>
#include <utf8cpp/utf8.h>
#include <variant>
using namespace art;

#define CXX_INTERFACE_ADD_TYPE(Class_)                                                   \
    bool implement_##Class_ = [] {                                                       \
        art::CXX::Interface::typeVTable<Class_>() = CXX::Interface::createTable<Class_>( \
            #Class_                                                                      \
        );                                                                               \
        return true;                                                                     \
    }()

template <class class_name>
ValueItem cxx_construct(class_name&& val) {
    return art::CXX::Interface::constructStructure<class_name>(
        (AttachAVirtualTable*)CXX::Interface::typeVTable<class_name>(),
        std::forward<class_name>(val)
    );
}

template <class class_name>
class_name cxx_take(ValueItem& val) {
    return std::move(art::CXX::Interface::getExtractAsStatic<class_name>(val));
}


namespace language_parsers {
#pragma region lang_decl
    constexpr char language_implantation_declaration[] =
        "ignored_chars = [ \\n\\t\\r]\n"
        "allowed_symbol_chars = [\\S]\n"
        "delimiting_chars = [+-*/%()[]{};:,.?!=<>&|^~\\\\\'\"`#@$ \\N]\n"
        "keep_delimiting_chars = [[+=][-=][*=][/=][%=][&=][|=][^=][~=][<<=][>>=][!=][<=][>=][||][&&][==][++][--]]\n"
        "namespace_symbol_sequence = :\n"
        "language_name = c@\n"
        "language_full_name = C async\n"
        "tokens = [[fn][gen][async][annotation][const][final]]\n"
        "tokens = [[signed][unsigned][static][checked][auto][any][char][double][float][short][int][long][byte][string][arr][map][hash_set][time_point][void]]\n"
        "tokens = [[return][yield][consume]]\n"
        "tokens = [[if][else][goto][while][do][for][loop][switch][case][break][match][continue][default]]\n"
        "tokens = [[struct][class][union][enum][interface][flags]]\n"
        "tokens = [[extern][sizeof][typedef][using][namespace][typename][decltype][static_assert][constexpr][import][export]]\n"
        "tokens = [[this][base][public][protected][private][internal][implicit][explicit][sealed][virtual][override][operator][constructor][destructor]]\n"
        "tokens = [[try][catch][throw][finally][filter][exception][in][out][ref][inline][noexcept][null][true][false]]\n"
        "token#_assign = [=]\n"
        "token#_plus = [+]\n"
        "token#_minus = [-]\n"
        "token#_multiply = [*]\n"
        "token#_divide = [/]\n"
        "token#_rest = [%]\n"
        "token#_inc = [++]\n"
        "token#_dec = [--]\n"
        "token#_assign_plus = [+=]\n"
        "token#_assign_minus = [-=]\n"
        "token#_assign_multiply = [*=]\n"
        "token#_assign_divide = [/=]\n"
        "token#_assign_rest = [%=]\n"
        "token#_equal = [==]\n"
        "token#_not_equal = [!=]\n"
        "token#_bigger = [>]\n"
        "token#_lower = [<]\n"
        "token#_bigger_or_equal = [>=]\n"
        "token#_lower_or_equal = [<=]\n"
        "token#_logical_and = [&&]\n"
        "token#_logical_or = [||]\n"
        "token#_logical_not = [!]\n"
        "token#_binary_and = [&]\n"
        "token#_binary_or = [|]\n"
        "token#_binary_xor = [^]\n"
        "token#_binary_invert = [~]\n"
        "token#_binary_left_shift = [<<]\n"
        "token#_binary_right_shift = [>>]\n"
        "token#_binary_assign_left_shift = [<<=]\n"
        "token#_binary_assign_right_shift = [>>=]\n"
        "token#_binary_assign_and = [&=]\n"
        "token#_binary_assign_or = [|=]\n"
        "token#_binary_assign_xor = [^=]\n"
        "token#_binary_assign_invert = [~=]\n"
        "token#block_begin_operator = [(]\n"
        "token#block_end_operator = [)]\n"
        "token#block_begin_index = [[]\n"
        "token#block_end_index = []]\n"
        "token#block_begin_scope = [{]\n"
        "token#block_end_scope = [}]\n"
        "token#delimiter_question = [?]\n"
        "token#delimiter_exclamation = [!]\n"
        "token#delimiter_left_slash = [/]\n"
        "token#delimiter_apostrophe = [']\n"
        "token#delimiter_lying_apostrophe = [`]\n"
        "token#delimiter_quotation = [\"]\n"
        "token#delimiter_coma = [,]\n"
        "token#delimiter_dot = [.]"
        "token#delimiter_colon = [:]\n"
        "token#delimiter_semicolon = [;]\n"
        "token#delimiter_hash = [#]\n"
        "token#delimiter_dollar = [$]\n"
        "token#delimiter_annotation = [@]\n"
        "token#lying_slash = [\\\\]\n"

        "value_processing#string_escape = {token_lying_slash}\n"
        "value_processing#string_scope = {token_delimiter_lying_apostrophe} $[..] {token_delimiter_lying_apostrophe}\n"
        "value_processing#string_line = {token_delimiter_quotation} $[..] {token_delimiter_quotation}\n"
        "value_processing#char = {token_delimiter_apostrophe} $[..] {token_delimiter_apostrophe}\n"
        "value_processing#hex_number_enable\n"
        "value_processing#octal_number_enable\n"
        "value_processing#binary_number_enable\n"
        "value_processing#decimal_dot = {token_delimiter_dot}\n"


        "compound_token#file = {value_string}@[file]\n"
        "compound_token#utf8_file = {value_string}@[utf8file]\n"
        "compound_token#utf16_file = {value_string}@[utf16file]\n"
        "compound_token#utf32_file = {value_string}@[utf32file]\n"
        "compound_token#ascii_file = {value_string}@[asciifile]\n"


        "header_end\n"
        "calculated_vars = {value_double}|{value_long}|{value_string}|{value_char}|{ctoken_file}|{ctoken_utf8_file}|{ctoken_utf16_file}|{ctoken_utf32_file}|{ctoken_ascii_file}|{token_null}|{token_true}|{token_false}\n"
        "type_name$auto$modifier = {token_auto}\n"
        "type_name$modifier = {token_any}\n"
        "type_name$modifier = {token_char}\n"
        "type_name$num$modifier = {token_time_point}\n"
        "type_name$num$modifier = {token_double} [{token_block_begin_index} {value_long} {token_block_end_index}]\n"
        "type_name$num$modifier = {token_float} [{token_block_begin_index} {value_long} {token_block_end_index}]\n"
        "type_name$num$unsigned$cut$modifier = {token_short}\n"
        "type_name$num$unsigned$cut$modifier = {token_int}\n"
        "type_name$num$unsigned$cut$modifier = {token_long}\n"
        "type_name$num$unsigned$cut$modifier = {token_byte}\n"
        "type_name$num$unsigned$modifier = {token_short} {{token_block_begin_index} {value_long} {token_block_end_index}}\n"
        "type_name$num$unsigned$modifier = {token_int} {{token_block_begin_index} {value_long} {token_block_end_index}}\n"
        "type_name$num$unsigned$modifier = {token_long} {{token_block_begin_index} {value_long} {token_block_end_index}}\n"
        "type_name$num$unsigned$modifier = {token_byte} {{token_block_begin_index} {value_long} {token_block_end_index}}\n"
        "type_name$modifier = {token_string}\n"
        "type_name$modifier = {token_arr} [{token__lower} {type_name} {token__bigger}]\n"
        "type_name$modifier = {token_map} [{token__lower} {type_name} [{token_coma} {type_name}] {token__bigger}]\n"
        "type_name$modifier = {token_hash_set} [{token__lower} {type_name} {token__bigger}]\n"
        "type_name = {token_void}\n"
        "type_name$modifier = {symbol} [{template}]\n"
        "#type_name@modifier = {token_const} $[..]\n"
        "#type_name@num@unsigned = {token_unsigned} $[..]\n"
        "#type_name@num@unsigned = {token_signed} $[..]\n"
        "#type_name@num@unsigned = {token_checked} $[..]\n"
        "#type_name@modifier = {token_final} $[..]\n"
        "#type_name@modifier$static = {token_static} $[..]\n"

        "template_decl_item = {token_class} {symbol} [{token_equal} {type_name}]\n"
        "template_decl_item = {type_name} {symbol} [{token_equal} {calculated_vars}]\n"
        "template_decl_inner = {template_decl_item} [{token_coma} {template_decl_inner}]\n"
        "template_decl = {token_template} {token__lower} {template_decl_inner} {token__bigger}\n"

        "template = {token__lower} [{template_inner}] {token__bigger}\n"
        "template_inner = {type_name}|{calculated_vars} [{token_coma} {template_inner}]\n"


        "fn_type = {token_fn}|{token_gen}|{token_async}\n"
        "args = {token_block_begin_operator} {args_inner} {token_block_end_operator}\n"
        "args_inner = {arg} [{token_coma} {args_inner}]\n"
        "arg = {type} {arg_name}\n"
        "type = [{annotation}] {type_name}\n"
        "return_type = [{annotation}] {type_name}\n"
        "return_types = {return_type} [{token_divide} {return_types}]\n"


        "struct_type = [{annotation}] {type_name}\n"
        "struct_type#type_name@cut^@static = $[..] {token_delimiter_colon} {value_long}\n"


        "protection = {local_protection} {token_delimiter_colon}\n"
        "local_protection = {token_public}\n"
        "local_protection = {token_private}\n"
        "local_protection = {token_protected}\n"
        "local_protection = {token_internal}\n"

        "annotation = {token_delimiter_annotation} {annotation_use}\n"
        "annotation = {token_delimiter_annotation} {token_block_begin_index} {annotation_uses} {token_block_end_index}\n"
        "annotation_use = {symbol}\n"
        "annotation_use = {symbol} {token_block_begin_operator} {calculated_vars} {token_block_end_operator}\n"
        "annotation_uses = {annotation_use} [{token_coma} {annotation_uses}]\n"

        "fn_decl = [{annotation}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {body}\n"
        "fn_decl = [{annotation}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {token_delimiter_semicolon}\n"
        "fn_decl = [{annotation}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {body}\n"
        "fn_decl = [{annotation}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {token_delimiter_semicolon}\n"

        "template_fn_decl = {template_decl} {fn_decl}\n"

        "annotation_decl = {token_annotation} {symbol} {annotation_decl_fn_types} {args} {body}\n"
        "annotation_decl = {token_annotation} {symbol} {annotation_decl_fn_types} {body}\n"
        "annotation_decl_fn_types = {fn_type} [{token_divide} {annotation_decl_fn_types}]\n"

        "struct_decl = {token_struct} {symbol} {struct_body} [{symbol} {token_delimiter_semicolon}]\n"
        "struct_decl = {token_struct} {symbol} {token_delimiter_semicolon}\n"
        "struct_decl = {token_struct} {symbol} {struct_short} {token_delimiter_semicolon}\n"
        "struct_decl = {token_struct} {symbol} {token_delimiter_colon} {struct_follow} {struct_body} [{symbol} {token_delimiter_semicolon}]\n"
        "struct_decl = {token_struct} {symbol} {token_delimiter_colon} {struct_follow} {token_delimiter_semicolon}\n"
        "struct_decl = {token_struct} {symbol} {struct_short} {token_delimiter_colon} {struct_follow} {token_delimiter_semicolon}\n"
        "anonymous_struct_decl = {token_struct} {struct_body} [{symbol}]\n"
        "anonymous_struct_decl = {token_struct} {struct_short}\n"
        "template_struct_decl = {template_decl} {struct_decl}\n"

        "struct_short = {args}\n"
        "struct_follow = {symbol} [{token_coma} {struct_follow}]\n"
        "struct_body = {struct_type} {symbol} {token_delimiter_semicolon} [{struct_body}]\n"

        "enum_decl = {token_enum} {symbol} {enum_body} {token_delimiter_semicolon}\n"
        "enum_decl = {token_enum} {symbol} {token_block_begin_operator} {enum_short} {token_block_end_operator} {token_delimiter_semicolon}\n"
        "template_enum_decl = {template_decl} {enum_decl}\n"

        "enum_body = {token_block_begin_scope} {enum_body_inner} {token_block_end_scope}\n"
        "enum_body_inner = [{annotation}] {symbol} [{token_coma} {enum_body_inner}]|[{token_delimiter_semicolon} {extended_enum_body}]\n"
        "extended_enum_body = {enum_fn_decl} [{extended_enum_body}]\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {token__bigger} {return_type} {body}\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {token__bigger} {return_types} {body}\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{tok_inline}] [{tok_noexcept}] {fn_type} {symbol} {args} {body}\n"
        "enum_short = [{annotation}] {symbol} [{token_coma} {enum_short}]\n"

        "class_decl = {token_class} {symbol} {class_body}\n"
        "class_decl = {token_class} {symbol} {token_delimiter_semicolon}\n"
        "class_decl = {token_class} {symbol} {token_delimiter_colon} {class_follow} {class_body} \n"
        "class_decl = {token_class} {symbol} {token_delimiter_colon} {class_follow} {token_delimiter_semicolon}\n"
        "template_class_decl = {template_decl} {class_decl}\n"

        "overload_mode = [{token_sealed}|{token_virtual}|{token_override}]\n"

        "class_body = [{annotation}] [{local_protection}] {struct_type} {symbol} {token_delimiter_semicolon} [{class_body}]\n"                                                                                                        //typ decl
        "class_body = [{annotation}] [{local_protection}] [{tok_inline}] [{tok_noexcept}] {overload_mode} {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {body} {token_delimiter_semicolon} [{class_body}]\n" //fn decl
        "class_body = [{annotation}] [{local_protection}] [{tok_inline}] [{tok_noexcept}] {overload_mode} {fn_type} {symbol} {args} {body} [{class_body}]\n"                                                                          //fn decl
        "class_body = {protection} [{class_body}]\n"
        "class_body = {class_operator} [{class_body}]\n"
        "class_body = {class_constructor} [{class_body}]\n"
        "class_body = {class_destructor} [{class_body}]\n"
        "class_follow = [{local_protection}] {symbol} [{token_coma} {class_follow}]\n"
        "class_operator = [{annotation}] [{token_explicit}|{token_implicit}] {overload_mode} {token_operator} {operators} {args} {token__bigger} {return_type}|{return_types} {body}\n"
        "class_operator = [{annotation}] [{token_explicit}|{token_implicit}] {overload_mode} {token_operator} {operators} {args} {body}\n"
        "class_constructor = [{annotation}] [{token_explicit}|{token_implicit}] {token_constructor} {args} {body}\n"
        "class_constructor = [{annotation}] [{token_explicit}|{token_implicit}] {token_constructor} {args} {token_delimiter_colon} {class_base_constructor} {body}\n"
        "class_destructor = [{annotation}] {overload_mode} {token_destructor} {args} {body}\n"
        "class_base_constructor = {symbol} {class_base_construct}\n"
        "class_base_construct = {token_block_begin_operator} {class_base_construct_inner} {token_block_end_operator}\n"
        "class_base_construct_inner = {symbol} [{token_coma} {class_base_construct_inner}]\n"
        "class_base_construct_inner = {calculated_vars} [{token_coma} {class_base_construct_inner}]\n"

        "two_way_operator = {token__assign}\n"
        "two_way_operator = {token__assign_plus}\n"
        "two_way_operator = {token__assign_minus}\n"
        "two_way_operator = {token__assign_multiply}\n"
        "two_way_operator = {token__assign_divide}\n"
        "two_way_operator = {token__assign_rest}\n"
        "two_way_operator = {token__plus}\n"
        "two_way_operator = {token__minus}\n"
        "two_way_operator = {token__multiply}\n"
        "two_way_operator = {token__divide}\n"
        "two_way_operator = {token__rest}\n"
        "two_way_operator = {token__equal}\n"
        "two_way_operator = {token__not_equal}\n"
        "two_way_operator = {token__bigger}\n"
        "two_way_operator = {token__bigger_or_equal}\n"
        "two_way_operator = {token__lower_or_equal}\n"
        "two_way_operator = {token__logical_and}\n"
        "two_way_operator = {token__logical_or}\n"
        "two_way_operator = {token__binary_and}\n"
        "two_way_operator = {token__binary_or}\n"
        "two_way_operator = {token__binary_invert}\n"
        "two_way_operator = {token__binary_left_shift}\n"
        "two_way_operator = {token__binary_right_shift}\n"
        "two_way_operator = {token__binary_assign_and}\n"
        "two_way_operator = {token__binary_assign_or}\n"
        "two_way_operator = {token__binary_assign_invert}\n"
        "two_way_operator = {token__binary_assign_left_shift}\n"
        "two_way_operator = {token__binary_assign_right_shift}\n"

        "one_way_operator = {token__inc}\n"
        "one_way_operator = {token__dec}\n"
        "one_way_operator = {token__logical_not}\n"

        "operators = {two_way_operator}|{one_way_operator}\n"

        "equation = {symbol}\n"
        "equation = {calculated_vars}\n"
        "equation = {token_exception}\n"
        "equation = {equation} {token_dot} {symbol}\n"
        "equation = {equation} {token_dot} {symbol} {token_block_begin_operator} [{equation_call}] {token_block_end_operator}\n"
        "equation = {equation} {two_way_operator} {equation}\n"
        "equation = {equation} {token__inc}\n"
        "equation = {equation} {token__dec}\n"
        "equation = {token__inc} {equation}\n"
        "equation = {token__dec} {equation}\n"
        "equation = {token__logical_not} {equation}\n"

        "equation_call = {equation} [{token_coma} {equaion_cal}]\n "

        "body = {token_block_begin_scope} {body_inner} {token_block_end_scope}\n"
        "body_inner = {body_item} [{body_inner}]\n"

        "catch_body = {token_block_begin_scope} {catch_body_inner} {token_block_end_scope}\n"
        "catch_body_inner = {body_item} [{catch_body_inner}]\n"

        "finally_body = {token_block_begin_scope} {finally_body_inner} {token_block_end_scope}\n"
        "finally_body_inner = {body_item} [{finally_body_inner}]\n"

        "filter_body = {token_block_begin_scope} {filter_body_inner} {token_block_end_scope}\n"
        "filter_body_inner = {body_item} [{filter_body_inner}]\n"


        "body_item = {equation} {token_delimiter_semicolon}\n"
        "body_item = {token_try} {body} {try_catch_seq}\n"
        "body_item = {token_throw} {equation} {token_delimiter_semicolon}\n"
        "body_item = {type} {symbol} [{token__assign} {equation}] {token_delimiter_semicolon}\n"
        "body_item = {token_loop} {body}\n"
        "body_item = {token_for} {for_loop_iterator} {body}\n"
        "body_item = {token_while} {while_loop} {body}\n"
        "body_item = {token_do} {body} {while_loop} {token_delimiter_semicolon}\n"
        "body_item = {token_if} {if_condition} {body} [{else_sequence}]\n"
        "body_item = {body}\n"
        "for_loop_iterator = {token_block_begin_operator} {for_loop} {token_block_end_operator}\n"
        "for_loop = [{type} {symbol} {token__assign} {equation}] {token_delimiter_semicolon} [{equation}] {token_delimiter_semicolon} {equation}\n"
        "for_loop = {type} {symbol} {token_delimiter_colon} {equation}\n"
        "while_loop = {token_block_begin_operator} {equation} {token_block_end_operator}\n"
        "if_condition = {token_block_begin_operator} {if_equation} {token_block_end_operator}\n"
        "if_equation = {{type} {symbol} [{token__assign} {equation}] {token_delimiter_semicolon} {if_equation}}|{equation}\n"
        "else_sequence = {token_else} {{token_if} {if_condition} {body} [{else_sequence}]}|{body}\n"
        "try_catch_seq = {token_catch} {catch_body} [{try_catch_seq}]\n"
        "try_catch_seq = {token_finally} {filter_body} [{try_catch_seq}]\n"
        "try_catch_seq = {token_filter} {filter_body} [{try_catch_seq}]\n"


        ;
#pragma endregion

    using namespace art::language::helpers;

    std::vector<uint8_t> read_whole_file(component_data& data) {
        auto file_path = (art::ustring)std::get<list_array<art::shared_ptr<component_data>>>(data.args)[0]->value;
        files::FileHandle file(
            file_path.c_str(),
            file_path.size(),
            files::open_mode::read,
            files::on_open_action::open_exists,
            files::_sync_flags{.sequential_scan = true},
            files::share_mode{true}
        );
        auto sz = (int64_t)file.size();
        int64_t readed = 0;
        std::vector<uint8_t> buffer;
        buffer.resize(sz);
        while (readed < sz)
            readed += file.read(buffer.data(), sz < INT32_MAX ? sz : INT32_MAX);
        return buffer;
    }

    struct annotation_item {
        std::string symbol;
        std::vector<ValueItem> args;
        size_t line, column;
    };

    struct type_data {
        std::string symbol;
        ValueMeta type;
        bool as_final = false;
        bool as_static = false;
        bool do_check = false;
        bool late_type_calculation = false;
        art::shared_ptr<type_data> inner_type;     //used for arr, map and set
        art::shared_ptr<type_data> represent_type; //used for map
        std::variant<nullptr_t, std::vector<std::variant<art::shared_ptr<type_data>, ValueItem>>> generics_or_template;
        size_t line, column;
    };

    struct annotated_type { //type
        type_data type;
        std::vector<annotation_item> annotations;
    };

    struct return_type { //return_type
        type_data type;
        std::vector<annotation_item> annotations;
    };

    struct return_types { //return_types
        std::vector<return_type> types;
    };

    struct struct_type { //struct_type
        type_data type;
        std::vector<annotation_item> annotations;
        uint32_t cut;
    };

    struct local_protection { //local_protection

        enum {
            public_,
            private_,
            protected_,
            internal_,
        } val;
    };

    struct set_default_protection { //protection
        local_protection prot;
    };

    struct template_decl_item {
        std::optional<art::shared_ptr<type_data>> as_value; //if not set then it is a type
        std::string symbol;
        std::variant<art::shared_ptr<type_data>, ValueItem, nullptr_t> default_value;
        size_t line, column;
    };

    struct template_decl {
        std::vector<art::shared_ptr<template_decl_item>> items;
        size_t line, column;
    };

    struct template_use {
        std::vector<std::variant<art::shared_ptr<type_data>, ValueItem>> items;
    };

    struct fn_type {
        enum {
            fn_,
            gen_,
            async_,
        } val;
    };

    struct arguments_ {
        std::vector<std::pair<std::string, annotated_type>> arg;
    };

    void c_async::init() {
        CXX_INTERFACE_ADD_TYPE(type_data);
        CXX_INTERFACE_ADD_TYPE(annotated_type);
        CXX_INTERFACE_ADD_TYPE(return_type);
        CXX_INTERFACE_ADD_TYPE(return_types);
        CXX_INTERFACE_ADD_TYPE(struct_type);
        CXX_INTERFACE_ADD_TYPE(local_protection);
        CXX_INTERFACE_ADD_TYPE(set_default_protection);
        CXX_INTERFACE_ADD_TYPE(template_decl_item);
        CXX_INTERFACE_ADD_TYPE(template_decl);
        CXX_INTERFACE_ADD_TYPE(template_use);
        CXX_INTERFACE_ADD_TYPE(fn_type);
        CXX_INTERFACE_ADD_TYPE(arguments_);
    }
    c_async::c_async()
        : art::language::helpers::text_language_handler(language_implantation_declaration) {

        register_processing_handler("ctoken_file", [](component_data& data) {
            auto file_path = (art::ustring)std::get<list_array<art::shared_ptr<component_data>>>(data.args)[0]->value;
            files::FileHandle file(
                file_path.c_str(),
                file_path.size(),
                files::open_mode::read,
                files::on_open_action::open_exists,
                files::_sync_flags{.sequential_scan = true},
                files::share_mode{true}
            );
            auto sz = (int64_t)file.size();
            if (sz <= UINT32_MAX) {
                art::array_t<uint8_t> buffer(new uint8_t[sz], sz);
                file.read(buffer.begin(), sz);
                data.value = std::move(buffer);
            } else {
                std::vector<uint8_t> buffer;
                int64_t readed = 0;
                buffer.resize(sz);
                while (readed < sz)
                    readed += file.read(buffer.data(), sz < INT32_MAX ? sz : INT32_MAX);
                data.value = list_array<ValueItem>(buffer.begin(), buffer.end());
            }
        });
        register_processing_handler("ctoken_utf8_file", [](component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            std::string str;
            utf8::replace_invalid(buffer.begin(), buffer.end(), std::back_inserter(str));
            data.value = ustring(std::move(str));
        });
        register_processing_handler("ctoken_utf16_file", [](component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            if (buffer.size() % 2)
                throw InvalidInput("Excepted utf16 file, but size is not even");
            std::string str;
            utf8::unchecked::utf16to8((char16_t*)buffer.data(), (char16_t*)(buffer.data() + buffer.size()), std::back_inserter(str));
            data.value = ustring(std::move(str));
        });
        register_processing_handler("ctoken_utf32_file", [](component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            if (buffer.size() % 4)
                throw InvalidInput("Excepted utf32 file, but size is not even");
            std::string str;
            utf8::unchecked::utf32to8((char32_t*)buffer.data(), (char32_t*)(buffer.data() + buffer.size()), std::back_inserter(str));
            data.value = ustring(std::move(str));
        });
        register_processing_handler("ctoken_ascii_file", [](component_data& data) {
            //std::vector<uint8_t> buffer = read_whole_file(data);
            std::string str;
            //TODO implement conversion from ascii to utf8
            data.value = ustring(std::move(str));
        });
        register_processing_handler("calculated_vars", [](component_data& data) {
            auto& comp = std::get<art::shared_ptr<component_data>>(data.args);
            if (comp->token_name == std::string_view("token_null"))
                data.value = nullptr;
            else if (comp->token_name == std::string_view("token_true"))
                data.value = true;
            else if (comp->token_name == std::string_view("token_false"))
                data.value = false;
            else
                data.value = std::move(std::get<art::shared_ptr<component_data>>(data.args)->value);
        });
        register_processing_handler("type_name", [](component_data& data) {
            auto& arr = std::get<list_array<art::shared_ptr<component_data>>>(data.args);
            type_data res;
            if (arr.size() == 1) {
                auto type_nam_ = arr[0]->token_name;
                if (type_nam_ == std::string_view("token_auto")) {
                    res = type_data{.late_type_calculation = true};
                } else if (type_nam_ == std::string_view("token_any")) {
                    res = type_data{.type = VType::any_obj};
                } else if (type_nam_ == std::string_view("token_char")) {
                    res = type_data{.type = VType::character};
                } else if (type_nam_ == std::string_view("token_time_point")) {
                    res = type_data{.type = VType::time_point};
                } else if (type_nam_ == std::string_view("token_double")) {
                    res = type_data{.type = VType::doub};
                } else if (type_nam_ == std::string_view("token_float")) {
                    res = type_data{.type = VType::flo};
                } else if (type_nam_ == std::string_view("token_short")) {
                    res = type_data{.type = VType::i16};
                } else if (type_nam_ == std::string_view("token_int")) {
                    res = type_data{.type = VType::i32};
                } else if (type_nam_ == std::string_view("token_long")) {
                    res = type_data{.type = VType::i64};
                } else if (type_nam_ == std::string_view("token_byte")) {
                    res = type_data{.type = VType::i8};
                } else if (type_nam_ == std::string_view("token_string")) {
                    res = type_data{.type = VType::string};
                } else if (type_nam_ == std::string_view("token_arr")) {
                    res = type_data{.type = VType::uarr, .inner_type = new type_data{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_map")) {
                    res = type_data{.type = VType::map, .inner_type = new type_data{.type = VType::any_obj}, .represent_type = new type_data{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_hash_set")) {
                    res = type_data{.type = VType::set, .inner_type = new type_data{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_void")) {
                    res = type_data{.type = VType::noting};
                } else if (type_nam_ == std::string_view("symbol")) {
                    res = type_data{.symbol = std::get<art::ustring>(arr[0]->args), .type = VType::struct_};
                } else
                    throw art::InvalidSyntaxException("Unrecognized type name: " + std::string(type_nam_));
            } else if (arr.size() == 2) {
                auto type_nam_ = arr[0]->token_name;
                if (type_nam_ == std::string_view("token_arr")) {
                    auto& inner = std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args);
                    res = type_data{.type = VType::uarr, .inner_type = new type_data(cxx_take<type_data>(inner[1]->value))};
                } else if (type_nam_ == std::string_view("token_map")) {
                    auto& inner = std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args);
                    art::shared_ptr<type_data> inner_type = new type_data(cxx_take<type_data>(inner[1]->value));
                    art::shared_ptr<type_data> represent_type;
                    if (inner.size() == 4) {
                        represent_type = new type_data(cxx_take<type_data>(std::get<list_array<art::shared_ptr<component_data>>>(inner[1]->args).at(1)->value));
                    } else
                        represent_type = new type_data{.type = VType::any_obj};
                    res = type_data{.type = VType::map, .inner_type = std::move(inner_type), .represent_type = std::move(represent_type)};
                } else if (type_nam_ == std::string_view("token_hash_set")) {
                    auto& inner = std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args);
                    res = type_data{.type = VType::set, .inner_type = new type_data(cxx_take<type_data>(inner[1]->value))};
                } else if (type_nam_ == std::string_view("token_const")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    td.type.allow_edit = false;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_unsigned")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    switch (td.type.vtype) {
                    case VType::i8:
                        td.type.vtype = VType::ui8;
                        break;
                    case VType::i16:
                        td.type.vtype = VType::ui16;
                        break;
                    case VType::i32:
                        td.type.vtype = VType::ui32;
                        break;
                    case VType::i64:
                        td.type.vtype = VType::ui64;
                        break;
                    case VType::raw_arr_i8:
                        td.type.vtype = VType::raw_arr_ui8;
                        break;
                    case VType::raw_arr_i16:
                        td.type.vtype = VType::raw_arr_ui16;
                        break;
                    case VType::raw_arr_i32:
                        td.type.vtype = VType::raw_arr_ui32;
                        break;
                    case VType::raw_arr_i64:
                        td.type.vtype = VType::raw_arr_ui64;
                        break;
                    default:
                        break;
                    }
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_signed")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    switch (td.type.vtype) {
                    case VType::ui8:
                        td.type.vtype = VType::i8;
                        break;
                    case VType::ui16:
                        td.type.vtype = VType::i16;
                        break;
                    case VType::ui32:
                        td.type.vtype = VType::i32;
                        break;
                    case VType::ui64:
                        td.type.vtype = VType::i64;
                        break;
                    case VType::raw_arr_ui8:
                        td.type.vtype = VType::raw_arr_i8;
                        break;
                    case VType::raw_arr_ui16:
                        td.type.vtype = VType::raw_arr_i16;
                        break;
                    case VType::raw_arr_ui32:
                        td.type.vtype = VType::raw_arr_i32;
                        break;
                    case VType::raw_arr_ui64:
                        td.type.vtype = VType::raw_arr_i64;
                        break;
                    default:
                        break;
                    }
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_checked")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    td.do_check = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_final")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    td.as_final = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_static")) {
                    type_data td = cxx_take<type_data>(arr[1]->value);
                    td.as_static = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_double")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_doub, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("token_float")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_flo, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("token_short")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_i16, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("token_int")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_i32, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("token_long")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_i64, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("token_byte")) {
                    res = type_data{.type = ValueMeta(VType::raw_arr_i8, false, true, (uint32_t)std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args)[1]->value)};
                } else if (type_nam_ == std::string_view("symbol")) {
                    auto& ar = std::get<list_array<art::shared_ptr<component_data>>>(arr[0]->args);
                    res = type_data{.symbol = std::get<art::ustring>(ar[0]->args), .type = VType::struct_, .generics_or_template = std::move(cxx_take<template_use>(ar[1]->value).items)};
                } else
                    throw art::InvalidSyntaxException("Unrecognized type name: " + std::string(type_nam_));
            } else
                throw art::InvalidSyntaxException("Invalid type declaration");
            res.line = data.line;
            res.column = data.column;
            data.value = cxx_construct(std::move(res));
        });
        register_processing_handler("template_decl_item", [](component_data& data) {
            auto& arr = std::get<list_array<art::shared_ptr<component_data>>>(data.args);
            auto& type = std::get<art::shared_ptr<component_data>>(arr[0]->args);
            auto& symbol = std::get<art::shared_ptr<component_data>>(arr[1]->args);
            template_decl_item res;
            if (type->token_name == "type_name")
                res.as_value = new type_data(cxx_take<type_data>(type->value));
            res.symbol = (art::ustring)symbol->value;
            if (arr.size() == 3) {
                auto& default_val = std::get<art::shared_ptr<component_data>>(std::get<list_array<art::shared_ptr<component_data>>>(arr[2]->args)[1]->args);
                if (default_val->token_name == "type_name") {
                    if (res.as_value)
                        throw art::InvalidSyntaxException("Excepted type instead of expression in " + std::to_string(data.line) + ":" + std::to_string(data.column));
                    res.default_value = art::shared_ptr(new type_data(cxx_take<type_data>(default_val->value)));
                } else {
                    if (!res.as_value)
                        throw art::InvalidSyntaxException("Excepted expression instead of type in " + std::to_string(data.line) + ":" + std::to_string(data.column));
                    res.default_value = default_val->value;
                }
            }
            res.line = data.line;
            res.column = data.column;
            data.value = cxx_construct(std::move(res));
        });
        register_processing_handler("template_decl_inner", [](component_data& data) {
        });
        register_processing_handler("template_decl", [](component_data& data) {
            auto& arr = std::get<list_array<art::shared_ptr<component_data>>>(data.args);
            std::vector<art::shared_ptr<template_decl_item>> items;
            list_array<art::shared_ptr<component_data>>* iter = &std::get<list_array<art::shared_ptr<component_data>>>(arr[3]->args);
            while (iter) {
                items.push_back(new template_decl_item(cxx_take<template_decl_item>((*iter)[0]->value)));
                if (iter->size() == 1)
                    break;
                iter = &std::get<list_array<art::shared_ptr<component_data>>>(std::get<list_array<art::shared_ptr<component_data>>>((*iter)[1]->args)[1]->args);
            }
            template_decl res;
            res.items = std::move(items);
            res.line = data.line;
            res.column = data.column;
            data.value = cxx_construct(std::move(res));
        });
        register_processing_handler("template", [](component_data& data) {
            auto& arr = std::get<list_array<art::shared_ptr<component_data>>>(data.args);
            if (arr.size() != 3) {
                data.value = cxx_construct(template_use{});
                return;
            }
            std::vector<std::variant<art::shared_ptr<type_data>, ValueItem>> items;
            list_array<art::shared_ptr<component_data>>* iter = &std::get<list_array<art::shared_ptr<component_data>>>(arr[1]->args);
            while (iter) {
                auto& item = (*iter)[0];
                if (item->token_name == "type_name")
                    items.push_back(art::shared_ptr(new type_data(cxx_take<type_data>(item->value))));
                else
                    items.push_back(item->value);
                if (iter->size() == 1)
                    break;
                iter = &std::get<list_array<art::shared_ptr<component_data>>>(std::get<list_array<art::shared_ptr<component_data>>>((*iter)[1]->args)[1]->args);
            }
            template_use res;
            res.items = std::move(items);
            data.value = cxx_construct(std::move(res));
        });
        register_processing_handler("template_inner", [](component_data& data) {
        });
    }
}