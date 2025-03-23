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
    template <size_t N>
    static constexpr inline bool operator==(std::string_view view, const char (&str)[N]) {
        return view == std::string_view(str);
    }

#pragma region lang_decl
    constexpr char language_implantation_declaration[] =
        "ignored_chars = [ \\n\\t\\r]\n"
        "allowed_symbol_chars = [\\S]\n"
        "delimiting_chars = [+-*/%()[]{};:,.?!=<>&|^~\\\\\'\"`#@$ \\N]\n"
        "keep_delimiting_chars = [[+=][-=][*=][/=][%=][&=][|=][^=][~=][<<=][>>=][!=][<=][>=][||][&&][==][++][--]]\n"
        "namespace_symbol_sequence = :\n"
        "language_name = c@\n"
        "language_full_name = C async\n"
        "language_extensions = [[ca]]\n"
        "tokens = [[fn][gen][async][annotation][const][final]]\n"
        "tokens = [[signed][unsigned][static][checked][auto][any][char][double][float][short][int][long][byte][string][arr][map][hash_set][time_point][void]]\n"
        "tokens = [[return][yield][consume]]\n"
        "tokens = [[if][else][goto][while][do][for][loop][switch][case][break][match][continue][default]]\n"
        "tokens = [[struct][class][union][enum][interface][flags]]\n"
        "tokens = [[using][namespace]]\n"
        "tokens = [[this][base][public][protected][private][internal][implicit][explicit][sealed][virtual][override][operator][constructor][destructor]]\n"
        "tokens = [[try][catch][throw][finally][filter][exception][in][out][ref][inline][null][true][false]]\n"
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

        "entry_components = [[global_scope]]\n"
        "header_end\n"
        "global_scope = {struct_decl}\n"
        "global_scope = {enum_decl}\n"
        "global_scope = {class_decl}\n"
        "global_scope = {annotation_decl}\n"
        "global_scope = {fn_decl}\n"
        "global_scope = {namespace_decl}\n"
        "global_scope = {using_decl}\n"
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
        "type_name$modifier = {token_map} [{token__lower} {type_name} [{token_delimiter_coma} {type_name}] {token__bigger}]\n"
        "type_name$modifier = {token_hash_set} [{token__lower} {type_name} {token__bigger}]\n"
        "type_name = {token_void}\n"
        "type_name$modifier = {namespaced_symbol}\n"
        "#type_name@modifier = {token_const} $[..]\n"
        "#type_name@num@unsigned = {token_unsigned} $[..]\n"
        "#type_name@num@unsigned = {token_signed} $[..]\n"
        "#type_name@num@unsigned = {token_checked} $[..]\n"
        "#type_name@modifier = {token_final} $[..]\n"
        "#type_name@modifier$static = {token_static} $[..]\n"

        "namespace_decl = {token_namespace} {namespaced_symbol} {global_scope}\n"
        "using_decl = {token_using} {token_namespace} {namespaced_symbol} {token_delimiter_semicolon}\n"

        //"template_decl_item = {token_class} {namespaced_symbol} [{token_equal} {type_name}]\n"
        //"template_decl_item = {type_name} {namespaced_symbol} [{token_equal} {calculated_vars}]\n"
        //"template_decl_inner = {template_decl_item} [{token_delimiter_coma} {template_decl_inner}]\n"
        //"template_decl = {token_template} {token__lower} {template_decl_inner} {token__bigger}\n"

        //"template = {token__lower} [{template_inner}] {token__bigger}\n"
        //"template_inner = {type_name}|{calculated_vars} [{token_delimiter_coma} {template_inner}]\n"


        "fn_type = {token_fn}|{token_gen}|{token_async}\n"
        "args = {token_block_begin_operator} {args_inner} {token_block_end_operator}\n"
        "args_inner = {arg} [{token_delimiter_coma} {args_inner}]\n"
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
        "annotation_use = {namespaced_symbol}\n"
        "annotation_use = {namespaced_symbol} {token_block_begin_operator} {annotation_args} {token_block_end_operator}\n"
        "annotation_args = {calculated_vars} [{token_delimiter_coma} {annotation_args}]\n"
        "annotation_uses = {annotation_use} [{token_delimiter_coma} {annotation_uses}]\n"

        "fn_decl = [{annotation}] [{token_inline}] {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {body}\n"
        "fn_decl = [{annotation}] [{token_inline}] {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {token_delimiter_semicolon}\n"
        "fn_decl = [{annotation}] [{token_inline}] {fn_type} {symbol} {args} {body}\n"
        "fn_decl = [{annotation}] [{token_inline}] {fn_type} {symbol} {args} {token_delimiter_semicolon}\n"

        "annotation_decl = {token_annotation} {symbol} {annotation_decl_types} {args} {body}\n"
        "annotation_decl = {token_annotation} {symbol} {annotation_decl_types} {body}\n"
        "annotation_decl_types = {fn_type}|@[type]|{token_class}|{token_enum}|{token_struct} [{token_divide} {annotation_decl_types}]\n"

        "struct_decl = [{annotation}] {token_struct} {symbol} {struct_body} [{symbol}] {token_delimiter_semicolon}\n"
        "struct_decl = [{annotation}] {token_struct} {symbol} {token_delimiter_semicolon}\n"
        "struct_decl = [{annotation}] {token_struct} {symbol} {struct_short} {token_delimiter_semicolon}\n"
        "struct_decl = [{annotation}] {token_struct} {symbol} {token_delimiter_colon} {struct_follow} {struct_body} [{symbol}] {token_delimiter_semicolon}\n"
        "struct_decl = [{annotation}] {token_struct} {symbol} {token_delimiter_colon} {struct_follow} {token_delimiter_semicolon}\n"
        "struct_decl = [{annotation}] {token_struct} {symbol} {struct_short} {token_delimiter_colon} {struct_follow} {token_delimiter_semicolon}\n"

        "struct_short = {args}\n"
        "struct_follow = {namespaced_symbol} [{token_delimiter_coma} {struct_follow}]\n"
        "struct_body = {token_block_begin_scope} {struct_body_item} {token_block_end_scope}\n"
        "struct_body_item = {struct_type} {symbol} {token_delimiter_semicolon} [{struct_body_item}]\n"

        "enum_decl = [{annotation}] {token_enum} {symbol} {enum_body} {token_delimiter_semicolon}\n"
        "enum_decl = [{annotation}] {token_enum} {symbol} {token_block_begin_operator} {enum_short} {token_block_end_operator} {token_delimiter_semicolon}\n"

        "enum_body = {token_block_begin_scope} {enum_body_inner} {token_block_end_scope}\n"
        "enum_body_inner = {symbol} [{token_delimiter_coma} {enum_body_inner}]|[{token_delimiter_semicolon} {extended_enum_body}]\n"
        "extended_enum_body = {enum_fn_decl} [{extended_enum_body}]\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{token_inline}] {fn_type} {symbol} {args} {token__bigger} {return_type} {body}\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{token_inline}] {fn_type} {symbol} {args} {token__bigger} {return_types} {body}\n"
        "enum_fn_decl = [{annotation}] [{local_protection}] [{token_inline}] {fn_type} {symbol} {args} {body}\n"
        "enum_short = {symbol} [{token_delimiter_coma} {enum_short}]\n"

        "class_decl = [{annotation}] {token_class} {symbol} {class_body} {token_delimiter_semicolon}\n"
        "class_decl = [{annotation}] {token_class} {symbol} {token_delimiter_colon} {class_follow} {class_body} {token_delimiter_semicolon}\n"
        "class_decl = [{annotation}] {token_class} {symbol} {token_delimiter_colon} {class_follow} {token_delimiter_semicolon}\n"

        "overload_mode = [{token_sealed}|{token_virtual}|{token_override}]\n"

        "class_body = {protection} [{class_body}]\n"
        "class_body = {class_operator} [{class_body}]\n"
        "class_body = {class_constructor} [{class_body}]\n"
        "class_body = {class_destructor} [{class_body}]\n"
        "class_body = {class_function} [{class_body}]\n"
        "class_body = {class_value} [{class_body}]\n"
        "class_follow = [{local_protection}] {namespaced_symbol} [{token_delimiter_coma} {class_follow}]\n"
        "class_value = [{annotation}] [{local_protection}] {struct_type} {symbol} {token_delimiter_semicolon}\n"
        "class_function = [{annotation}] [{local_protection}] [{token_inline}] {overload_mode} {fn_type} {symbol} {args} {token__bigger} {return_type}|{return_types} {body} {token_delimiter_semicolon}\n"
        "class_function = [{annotation}] [{local_protection}] [{token_inline}] {overload_mode} {fn_type} {symbol} {args} {body}\n"

        "class_operator = [{annotation}] [{local_protection}] [{token_explicit}|{token_implicit}] {overload_mode} {token_operator} {operators} {args} {token__bigger} {return_type}|{return_types} {body}\n"
        "class_operator = [{annotation}] [{local_protection}] [{token_explicit}|{token_implicit}] {overload_mode} {token_operator} {operators} {args} {body}\n"
        "class_constructor = [{annotation}] [{local_protection}] [{token_explicit}|{token_implicit}] {token_constructor} {args} {body}\n"
        "class_destructor = [{annotation}] {overload_mode} {token_destructor} {args} {body}\n"

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

        "equation = {namespaced_symbol}\n"
        "equation = {namespaced_symbol} {token_block_begin_operator} [{equation_call}] {token_block_end_operator}\n"
        "equation = {calculated_vars}\n"
        "equation = {token_exception}\n"
        "equation = {equation} {two_way_operator} {equation}\n"
        "equation = {equation} {token__inc}\n"
        "equation = {equation} {token__dec}\n"
        "equation = {token__inc} {equation}\n"
        "equation = {token__dec} {equation}\n"
        "equation = {token__logical_not} {equation}\n"

        "equation_call = {equation} [{token_delimiter_coma} {equation_call}]\n"

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
        "body_item = {type} {symbol} {token_delimiter_semicolon}\n"
        "body_item = {type} {symbol} {token_block_begin_operator} [{equation_call}] {token_block_end_operator} {token_delimiter_semicolon}\n"
        "body_item = {type} {symbol} {token__assign} {equation} {token_delimiter_semicolon}\n"
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
        "if_equation = {type} {symbol} [{token__assign} {equation}] {token_delimiter_semicolon} {if_equation}\n"
        "if_equation = {equation}\n"
        "else_sequence = {token_else} {{token_if} {if_condition} {body} [{else_sequence}]}|{body}\n"
        "try_catch_seq = {token_catch} {catch_body} [{try_catch_seq}]\n"
        "try_catch_seq = {token_finally} {filter_body} [{try_catch_seq}]\n"
        "try_catch_seq = {token_filter} {filter_body} [{try_catch_seq}]\n"


        ;
#pragma endregion

    using namespace art::language::helpers;

    std::vector<uint8_t> read_whole_file(component_data& data) {
        auto file_path = (art::ustring)data.args[0]->value;
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

    void compile_body(
        art::shared_ptr<FuncEnviroBuilder>& fn,
        component_data& data,
        std::string_view class_name
    );

    namespace util {
        struct namespaced_symbol : public std::vector<std::string> {
            namespaced_symbol() {}

            namespaced_symbol(const ValueItem& item)
                : std::vector<std::string>(
                      ((list_array<ValueItem>)item)
                          .convert_fn(
                              [](auto& it) {
                                  return (art::ustring)it;
                              }
                          )
                          .to_container<std::vector<std::string>>()
                  ) {}
        };
    }

    struct empty {
        static void process(component_data& data) {}
    };

    struct ctoken_file {
        static void process(component_data& data) {
            auto file_path = (art::ustring)data.args[0]->value;
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
        }
    };

    struct ctoken_utf8_file {
        static void process(component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            std::string str;
            utf8::replace_invalid(buffer.begin(), buffer.end(), std::back_inserter(str));
            data.value = ustring(std::move(str));
        }
    };

    struct ctoken_utf16_file {
        static void process(component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            if (buffer.size() % 2)
                throw InvalidInput("Excepted utf16 file, but size is not even");
            std::string str;
            utf8::unchecked::utf16to8((char16_t*)buffer.data(), (char16_t*)(buffer.data() + buffer.size()), std::back_inserter(str));
            data.value = ustring(std::move(str));
        }
    };

    struct ctoken_utf32_file {
        static void process(component_data& data) {
            std::vector<uint8_t> buffer = read_whole_file(data);
            if (buffer.size() % 4)
                throw InvalidInput("Excepted utf32 file, but size is not even");
            std::string str;
            utf8::unchecked::utf32to8((char32_t*)buffer.data(), (char32_t*)(buffer.data() + buffer.size()), std::back_inserter(str));
            data.value = ustring(std::move(str));
        }
    };

    struct ctoken_ascii_file {
        static void process(component_data& data) {
            //std::vector<uint8_t> buffer = read_whole_file(data);
            std::string str;
            //TODO implement conversion from ascii7 to utf8
            data.value = ustring(std::move(str));
        }
    };

    struct calculated_vars {
        static void process(component_data& data) {
            auto& comp = data.args[0];
            if (comp->token_name == std::string_view("token_null"))
                data.value = nullptr;
            else if (comp->token_name == std::string_view("token_true"))
                data.value = true;
            else if (comp->token_name == std::string_view("token_false"))
                data.value = false;
            else
                data.value = std::move(data.args[0]->value);
        }
    };

    struct annotation_use {
        util::namespaced_symbol symbol;
        std::vector<ValueItem> args;
        size_t line, column;

        static void process(component_data& data) {
            annotation_use res;
            res.line = data.line;
            res.column = data.column;
            if (data.args.size() >= 1)
                res.symbol = data.args[0]->value;
            if (data.args.size() == 4) {
                list_array<ValueItem> items;
                list_array<art::shared_ptr<component_data>>* iter = &data.args[2]->args;
                while (iter) {
                    items.push_back(std::move((*iter)[0]->value));
                    if (iter->size() == 1)
                        break;
                    iter = &(*iter)[1]->args[1]->args;
                }
                res.args = items.take().to_container<std::vector<ValueItem>>();
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    struct annotation {
        std::vector<annotation_use> items;

        static void process(component_data& data) {
            if (data.args.size() == 2)
                data.value = cxx_construct(annotation{.items = {cxx_take<annotation_use>(data.args[1]->value)}});
            else if (data.args.size() == 4) {
                list_array<annotation_use> items;
                list_array<art::shared_ptr<component_data>>* iter = &data.args[2]->args;
                while (iter) {
                    items.push_back(cxx_take<annotation_use>((*iter)[0]->value));
                    if (iter->size() == 1)
                        break;
                    iter = &(*iter)[1]->args[1]->args;
                }
                data.value = cxx_construct(annotation{.items = items.take().to_container<std::vector<annotation_use>>()});
            }
        }
    };

    struct type_name {
        util::namespaced_symbol symbol;
        ValueMeta type;
        bool as_final = false;
        bool as_static = false;
        bool do_check = false;
        bool late_type_calculation = false;
        uint8_t cut = 0;
        art::shared_ptr<type_name> inner_type;     //used for arr, map and set
        art::shared_ptr<type_name> represent_type; //used for map
        size_t line, column;

        static void process(component_data& data) {
            type_name res;
            if (data.args.size() == 1) {
                auto type_nam_ = data.args[0]->token_name;
                if (type_nam_ == std::string_view("token_auto")) {
                    res = type_name{.late_type_calculation = true};
                } else if (type_nam_ == std::string_view("token_any")) {
                    res = type_name{.type = VType::any_obj};
                } else if (type_nam_ == std::string_view("token_char")) {
                    res = type_name{.type = VType::character};
                } else if (type_nam_ == std::string_view("token_time_point")) {
                    res = type_name{.type = VType::time_point};
                } else if (type_nam_ == std::string_view("token_double")) {
                    res = type_name{.type = VType::doub};
                } else if (type_nam_ == std::string_view("token_float")) {
                    res = type_name{.type = VType::flo};
                } else if (type_nam_ == std::string_view("token_short")) {
                    res = type_name{.type = VType::i16};
                } else if (type_nam_ == std::string_view("token_int")) {
                    res = type_name{.type = VType::i32};
                } else if (type_nam_ == std::string_view("token_long")) {
                    res = type_name{.type = VType::i64};
                } else if (type_nam_ == std::string_view("token_byte")) {
                    res = type_name{.type = VType::i8};
                } else if (type_nam_ == std::string_view("token_string")) {
                    res = type_name{.type = VType::string};
                } else if (type_nam_ == std::string_view("token_arr")) {
                    res = type_name{.type = VType::uarr, .inner_type = new type_name{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_map")) {
                    res = type_name{.type = VType::map, .inner_type = new type_name{.type = VType::any_obj}, .represent_type = new type_name{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_hash_set")) {
                    res = type_name{.type = VType::set, .inner_type = new type_name{.type = VType::any_obj}};
                } else if (type_nam_ == std::string_view("token_void")) {
                    res = type_name{.type = VType::noting};
                } else if (type_nam_ == std::string_view("namespaced_symbol")) {
                    res = type_name{.symbol = data.args[0]->value, .type = VType::struct_};
                } else
                    throw art::InvalidSyntaxException("Unrecognized type name: " + std::string(type_nam_));
            } else if (data.args.size() == 2) {
                auto type_nam_ = data.args[0]->token_name;
                if (type_nam_ == std::string_view("token_arr")) {
                    auto& inner = data.args[1]->args;
                    res = type_name{.type = VType::uarr, .inner_type = new type_name(cxx_take<type_name>(inner[1]->value))};
                } else if (type_nam_ == std::string_view("token_map")) {
                    auto& inner = data.args[1]->args;
                    art::shared_ptr<type_name> inner_type = new type_name(cxx_take<type_name>(inner[1]->value));
                    art::shared_ptr<type_name> represent_type;
                    if (inner.size() == 4) {
                        represent_type = new type_name(cxx_take<type_name>(inner[1]->args.at(1)->value));
                    } else
                        represent_type = new type_name{.type = VType::any_obj};
                    res = type_name{.type = VType::map, .inner_type = std::move(inner_type), .represent_type = std::move(represent_type)};
                } else if (type_nam_ == std::string_view("token_hash_set")) {
                    auto& inner = data.args[1]->args;
                    res = type_name{.type = VType::set, .inner_type = new type_name(cxx_take<type_name>(inner[1]->value))};
                } else if (type_nam_ == std::string_view("token_const")) {
                    type_name td = cxx_take<type_name>(data.args[1]->value);
                    td.type.allow_edit = false;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_unsigned")) {
                    type_name td = cxx_take<type_name>(data.args[1]->value);
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
                    type_name td = cxx_take<type_name>(data.args[1]->value);
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
                    type_name td = cxx_take<type_name>(data.args[1]->value);
                    td.do_check = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_final")) {
                    type_name td = cxx_take<type_name>(data.args[1]->value);
                    td.as_final = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_static")) {
                    type_name td = cxx_take<type_name>(data.args[1]->value);
                    td.as_static = true;
                    res = std::move(td);
                } else if (type_nam_ == std::string_view("token_double")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_doub, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else if (type_nam_ == std::string_view("token_float")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_flo, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else if (type_nam_ == std::string_view("token_short")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_i16, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else if (type_nam_ == std::string_view("token_int")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_i32, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else if (type_nam_ == std::string_view("token_long")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_i64, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else if (type_nam_ == std::string_view("token_byte")) {
                    res = type_name{.type = ValueMeta(VType::raw_arr_i8, false, true, (uint32_t)data.args[1]->args[1]->value)};
                } else
                    throw art::InvalidSyntaxException("Unrecognized type name: " + std::string(type_nam_));
            } else if (data.args.size() == 3) {
                auto type_nam_ = data.args[0]->token_name;
                if (type_nam_ == std::string_view("token_short"))
                    res = type_name{.type = VType::i16};
                else if (type_nam_ == std::string_view("token_int"))
                    res = type_name{.type = VType::i32};
                else if (type_nam_ == std::string_view("token_long"))
                    res = type_name{.type = VType::i64};
                else if (type_nam_ == std::string_view("token_byte"))
                    res = type_name{.type = VType::i8};
                else
                    throw art::InvalidSyntaxException("Unrecognized invalid type name: " + std::string(type_nam_));
                res.cut = (uint8_t)data.args[2]->value;
            } else
                throw art::InvalidSyntaxException("Invalid type declaration");
            res.line = data.line;
            res.column = data.column;
            data.value = cxx_construct(std::move(res));
        }
    };

    struct type_ { //type
        type_name type;
        annotation annotations;

        void process(component_data& data) {
            if (data.args.size() > 1) {
                type_ res;
                res.annotations = cxx_take<annotation>(data.args[0]->value);
                res.type = cxx_take<type_name>(data.args[1]->value);
                data.value = cxx_construct(std::move(res));
            } else
                data.value = cxx_construct(type_{.type = cxx_take<type_name>(data.args[0]->value)});
        }
    };

    //struct return_type : public type_ {}; //return_type

    //struct struct_type : public type_ {}; //struct_type

    struct return_types { //return_types
        list_array<type_> types;

        void process(component_data& data) {
            list_array<type_> types;
            types.push_back(cxx_take<type_>(data.args[0]->value));
            if (data.args.size() > 1) {
                auto& add_types = data.args[1]->args;
                types.push_back(cxx_take<return_types>(add_types[1]->value).types);
            }
            return_types res;
            res.types = std::move(types);
            data.value = cxx_construct(std::move(res));
        }
    };

    struct local_protection { //local_protection

        enum {
            public_,
            private_,
            protected_,
            internal_,
        } val;

        local_protection()
            : val(public_) {}

        local_protection(const ValueItem& it)
            : val((decltype(val))(int)it) {}

        static void process(component_data& data) {
            if (data.token_name == "token_public")
                data.value = (int)public_;
            else if (data.token_name == "token_private")
                data.value = (int)private_;
            else if (data.token_name == "token_protected")
                data.value = (int)protected_;
            else if (data.token_name == "token_internal")
                data.value = (int)internal_;
            else
                throw art::InvalidSyntaxException("Unrecognized protection: " + std::string(data.token_name));
        }
    };

    struct protection { //protection
        local_protection prot;

        static void process(component_data& data) {
            data.value = cxx_construct(protection{.prot = data.args[0]->value});
        }
    };

    struct fn_type {
        enum {
            fn_,
            gen_,
            async_,
        } val;

        fn_type()
            : val(fn_) {}

        fn_type(const ValueItem& it)
            : val((decltype(val))(int)it) {}

        static void process(component_data& data) {
            auto& str = (art::ustring&)data.args[0]->value;
            if (str == "fn")
                data.value = (int)fn_;
            else if (str == "gen")
                data.value = (int)gen_;
            else if (str == "async")
                data.value = (int)async_;
            else
                throw art::InvalidSyntaxException("Excepted fn, gen or async in " + std::to_string(data.line) + ":" + std::to_string(data.column));
        }
    };

    struct arguments_ { //args
        std::vector<std::pair<std::string, type_>> arg;

        static void process(component_data& data) {
            list_array<std::pair<std::string, type_>> arg;
            list_array<art::shared_ptr<component_data>>* iter = &data.args[1]->args;
            while (iter) {
                auto& item = (*iter)[0]->args;

                arg.push_back(std::make_pair((art::ustring)item[1]->value, type_(cxx_take<type_>(item[0]->value))));
                if (iter->size() == 1)
                    break;
                iter = &(*iter)[1]->args[1]->args;
            }
            arguments_ res;
            res.arg = arg.take().to_container<std::vector<std::pair<std::string, type_>>>();
            data.value = cxx_construct(std::move(res));
        }
    };

    struct fn_decl {
        annotation annotations;
        bool use_inlining = false;
        fn_type type;
        std::string symbol;
        arguments_ args;
        return_types returns;
        art::shared_ptr<component_data> body;

        static void process(component_data& data) {
            fn_decl res;
            for (auto& it : data.args) {
                if (it->token_name == "annotation")
                    res.annotations = cxx_take<annotation>(it->value);
                else if (it->token_name == "token_inline")
                    res.use_inlining = true;
                else if (it->token_name == "fn_type")
                    res.type = it->value;
                else if (it->token_name == "symbol")
                    res.symbol = (art::ustring)it->value;
                else if (it->token_name == "args")
                    res.args = cxx_take<arguments_>(it->value);
                else if (it->token_name == "return_type")
                    res.returns.types.push_back(cxx_take<type_>(it->value));
                else if (it->token_name == "return_types")
                    res.returns = cxx_take<return_types>(it->value);
                else if (it->token_name == "body")
                    res.body = it;
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    struct annotation_decl {
        struct annotation_decl_types {
            enum {
                fn_ = 1,
                gen_ = 2,
                async_ = 4,
                type_ = 8,
                class_ = 16,
                struct_ = 32,
                enum_ = 64
            };

            uint8_t flags = 0;

            annotation_decl_types() {}

            annotation_decl_types(const ValueItem& it)
                : flags(it) {}

            static void process(component_data& data) {
                annotation_decl_types res;
                {
                    auto& it = data.args[0];
                    if (it->token_name == "token_fn")
                        res.flags |= fn_;
                    else if (it->token_name == "token_gen")
                        res.flags |= gen_;
                    else if (it->token_name == "token_async")
                        res.flags |= async_;
                    else if (it->token_name == "type")
                        res.flags |= type_;
                    else if (it->token_name == "token_class")
                        res.flags |= class_;
                    else if (it->token_name == "token_struct")
                        res.flags |= struct_;
                    else if (it->token_name == "token_enum")
                        res.flags |= enum_;
                }
                if (data.args.size() > 1)
                    res.flags |= (uint8_t)data.args[1]->args[1]->value;
                data.value = cxx_construct(std::move(res));
            }
        };

        std::string symbol;
        annotation_decl_types enabled_types;
        arguments_ args;
        art::shared_ptr<component_data> body;

        static void process(component_data& data) {
            annotation_decl res;
            for (auto& it : data.args) {
                if (it->token_name == "symbol")
                    res.symbol = (art::ustring)it->value;
                else if (it->token_name == "annotation_decl_types")
                    res.enabled_types = it->value;
                else if (it->token_name == "args")
                    res.args = cxx_take<arguments_>(it->value);
                else if (it->token_name == "body")
                    res.body = it;
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    struct struct_decl {
        struct struct_body {
            arguments_ types;

            static void process(component_data& data) {
                list_array<std::pair<std::string, type_>> arg;
                list_array<art::shared_ptr<component_data>>* iter = &data.args[1]->args;
                while (iter) {
                    arg.push_back(std::make_pair((art::ustring)(*iter)[1]->value, type_(cxx_take<type_>((*iter)[0]->value))));
                    if (iter->size() > 3)
                        break;
                    iter = &(*iter)[3]->args;
                }
                arguments_ res;
                res.arg = arg.take().to_container<std::vector<std::pair<std::string, type_>>>();
                data.value = cxx_construct(struct_body{std::move(res)});
            }
        };

        std::string symbol;
        annotation annotations;
        std::vector<std::string> follows_structs;
        arguments_ types;
        std::optional<std::string> inline_set;

        static void process(component_data& data) {
            struct_decl res;
            for (auto& it : data.args) {
                if (it->token_name == "annotation")
                    res.annotations = cxx_take<annotation>(it->value);
                else if (it->token_name == "symbol")
                    res.symbol = (art::ustring)it->value;
                else if (it->token_name == "struct_body")
                    res.types = cxx_take<struct_body>(it->value).types;
                else if (it->token_name == "struct_short")
                    res.types = cxx_take<arguments_>(it->value);
                else if (it->token_name == "struct_follow") {
                    list_array<std::string> follows_structs;
                    list_array<art::shared_ptr<component_data>>* iter = &it->args;
                    while (iter) {
                        follows_structs.push_back((art::ustring)(*iter)[0]->value);
                        if (iter->size() > 2)
                            break;
                        iter = &(*iter)[2]->args;
                    }
                    res.follows_structs = follows_structs.take().to_container<std::vector<std::string>>();
                } else if (it->token_name == "token_delimiter_colon") {
                } else if (it->token_name == "token_delimiter_semicolon") {
                } else if (it->args.size())
                    res.inline_set = (art::ustring)it->args[0]->value;
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    struct enum_decl {
        struct enum_fn_decl {
            annotation annotations;
            bool use_inlining = false;
            fn_type type;
            std::string symbol;
            arguments_ args;
            return_types returns;
            art::shared_ptr<component_data> body;
            local_protection lp;

            static void process(component_data& data) {
                enum_fn_decl res;
                for (auto& it : data.args) {
                    if (it->token_name == "annotation")
                        res.annotations = cxx_take<annotation>(it->value);
                    else if (it->token_name == "local_protection")
                        res.lp = it->value;
                    else if (it->token_name == "token_inline")
                        res.use_inlining = true;
                    else if (it->token_name == "fn_type")
                        res.type = it->value;
                    else if (it->token_name == "symbol")
                        res.symbol = (art::ustring)it->value;
                    else if (it->token_name == "args")
                        res.args = cxx_take<arguments_>(it->value);
                    else if (it->token_name == "return_type")
                        res.returns.types.push_back(cxx_take<type_>(it->value));
                    else if (it->token_name == "return_types")
                        res.returns = cxx_take<return_types>(it->value);
                    else if (it->token_name == "body")
                        res.body = it;
                }
                data.value = cxx_construct(std::move(res));
            }
        };

        std::string symbol;
        annotation annotations;
        std::vector<std::string> values;
        std::vector<enum_fn_decl> functions;

        static void process(component_data& data) {
            enum_decl decl;
            list_array<std::string> values;
            int_fast8_t offset = 0;
            if (data.args[0]->token_name == "annotation") {
                offset = 1;
                decl.annotations = cxx_take<annotation>(data.args[0]->value);
            }
            decl.symbol = (art::ustring)data.args[offset + 1]->value;
            if (data.args[offset + 2]->token_name == "enum_body") {
                list_array<enum_fn_decl> functions;
                bool parse_functions = false;
                list_array<art::shared_ptr<component_data>>* iter = &data.args[offset + 2]->args[1]->args;
                while (iter) {
                    auto& item = *iter;
                    values.push_back((art::ustring)item[0]->value);
                    if (item.size() == 1)
                        break;
                    if (item[1]->args[1]->token_name == "extended_enum_body") {
                        parse_functions = true;
                        iter = &item[1]->args[1]->args;
                        break;
                    } else
                        iter = &item[1]->args[1]->args;
                }
                if (parse_functions) {
                    while (iter) {
                        auto& item = *iter;
                        functions.push_back(cxx_take<enum_fn_decl>(item[0]->value));
                        if (item.size() == 1)
                            break;
                        iter = &item[1]->args;
                    }
                }
                decl.functions = functions.take().to_container<std::vector<enum_fn_decl>>();
            } else {
                list_array<art::shared_ptr<component_data>>* iter = &data.args[offset + 3]->args;
                while (iter) {
                    auto& item = *iter;
                    values.push_back((art::ustring)item[0]->value);
                    if (item.size() == 1)
                        break;
                    iter = &item[1]->args;
                }
            }
            decl.values = values.take().to_container<std::vector<std::string>>();
            data.value = cxx_construct(std::move(decl));
        }
    };

    struct class_decl {
        struct class_body {
            struct overload_mode {
                enum {
                    sealed_,
                    virtual_,
                    override_,
                } val;

                overload_mode()
                    : val(virtual_) {}

                overload_mode(const ValueItem& it)
                    : val((decltype(val))(int)it) {}

                static void process(component_data& data) {
                    if (data.token_name == "sealed")
                        data.value = (int)sealed_;
                    else if (data.token_name == "virtual")
                        data.value = (int)virtual_;
                    else if (data.token_name == "override")
                        data.value = (int)override_;
                    else
                        throw art::InvalidSyntaxException("Unrecognized protection: " + std::string(data.token_name));
                }
            };

            struct class_function {
                annotation annotations;
                bool use_inlining = false;
                fn_type type;
                std::string symbol;
                arguments_ args;
                return_types returns;
                art::shared_ptr<component_data> body;
                overload_mode overload_;
                std::optional<local_protection> lp;

                static void process(component_data& data) {
                    class_function res;
                    for (auto& it : data.args) {
                        if (it->token_name == "annotation")
                            res.annotations = cxx_take<annotation>(it->value);
                        else if (it->token_name == "local_protection")
                            res.lp = it->value;
                        else if (it->token_name == "token_inline")
                            res.use_inlining = true;
                        else if (it->token_name == "overload_mode")
                            res.overload_ = it->value;
                        else if (it->token_name == "fn_type")
                            res.type = it->value;
                        else if (it->token_name == "symbol")
                            res.symbol = (art::ustring)it->value;
                        else if (it->token_name == "args")
                            res.args = cxx_take<arguments_>(it->value);
                        else if (it->token_name == "return_type")
                            res.returns.types.push_back(cxx_take<type_>(it->value));
                        else if (it->token_name == "return_types")
                            res.returns = cxx_take<return_types>(it->value);
                        else if (it->token_name == "body")
                            res.body = it;
                    }
                    data.value = cxx_construct(std::move(res));
                }
            };

            struct class_value {
                annotation annotations;
                type_ type;
                std::string symbol;
                std::optional<local_protection> lp;

                static void process(component_data& data) {
                    class_value res;
                    for (auto& it : data.args) {
                        if (it->token_name == "annotation")
                            res.annotations = cxx_take<annotation>(it->value);
                        else if (it->token_name == "local_protection")
                            res.lp = it->value;
                        else if (it->token_name == "struct_type")
                            res.type = cxx_take<type_>(it->value);
                        else if (it->token_name == "symbol")
                            res.symbol = (art::ustring)it->value;
                    }
                    data.value = cxx_construct(std::move(res));
                }
            };

            struct class_operator : public class_function {
                bool is_explicit = false;

                static void process(component_data& data) {
                    class_operator res;
                    for (auto& it : data.args) {
                        if (it->token_name == "annotation")
                            res.annotations = cxx_take<annotation>(it->value);
                        else if (it->token_name == "local_protection")
                            res.lp = it->value;
                        else if (it->token_name == "token_inline")
                            res.use_inlining = true;
                        else if (it->token_name == "overload_mode")
                            res.overload_ = it->value;
                        else if (it->token_name == "token_explicit")
                            res.is_explicit = true;
                        else if (it->token_name == "fn_type")
                            res.type = it->value;
                        else if (it->token_name == "operators")
                            res.symbol = (art::ustring)it->value;
                        else if (it->token_name == "args")
                            res.args = cxx_take<arguments_>(it->value);
                        else if (it->token_name == "return_type")
                            res.returns.types.push_back(cxx_take<type_>(it->value));
                        else if (it->token_name == "return_types")
                            res.returns = cxx_take<return_types>(it->value);
                        else if (it->token_name == "body")
                            res.body = it;
                    }
                    data.value = cxx_construct(std::move(res));
                }
            };

            struct class_constructor : public class_function {
                bool is_explicit = false;

                static void process(component_data& data) {
                    class_constructor res;
                    for (auto& it : data.args) {
                        if (it->token_name == "annotation")
                            res.annotations = cxx_take<annotation>(it->value);
                        else if (it->token_name == "token_explicit")
                            res.is_explicit = true;
                        else if (it->token_name == "local_protection")
                            res.lp = it->value;
                        else if (it->token_name == "args")
                            res.args = cxx_take<arguments_>(it->value);
                        else if (it->token_name == "body")
                            res.body = it;
                    }
                    res.type = fn_type{fn_type::fn_};
                    data.value = cxx_construct(std::move(res));
                }
            };

            struct class_destructor : public class_function {
                static void process(component_data& data) {
                    class_destructor res;
                    for (auto& it : data.args) {
                        if (it->token_name == "annotation")
                            res.annotations = cxx_take<annotation>(it->value);
                        else if (it->token_name == "args")
                            res.args = cxx_take<arguments_>(it->value);
                        else if (it->token_name == "body")
                            res.body = it;
                    }
                    res.type = fn_type{fn_type::fn_};
                    data.value = cxx_construct(std::move(res));
                }
            };

            std::variant<class_function, class_value, protection, class_operator, class_constructor, class_destructor> self;

            static void process(component_data& data) {
                std::unique_ptr<class_body> self;
                auto& name = data.args[0]->token_name;

                if (name == "class_function")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<class_function>(data.args[0]->value)});
                else if (name == "class_value")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<class_value>(data.args[0]->value)});
                else if (name == "protection")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<protection>(data.args[0]->value)});
                else if (name == "class_operator")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<class_operator>(data.args[0]->value)});
                else if (name == "class_constructor")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<class_constructor>(data.args[0]->value)});
                else if (name == "class_destructor")
                    self = std::make_unique<class_body>(class_body{.self = cxx_take<class_destructor>(data.args[0]->value)});
                else
                    throw NotImplementedException();
                data.value = cxx_construct<class_body>(std::move(*self));
            }
        };

        struct class_follow_item {
            std::optional<local_protection> protection;
            std::string symbol;
        };

        std::string symbol;
        annotation annotations;
        list_array<class_body> body;
        list_array<class_follow_item> follows;

        static void process(component_data& data) {
            class_decl res;
            for (auto& it : data.args) {
                if (it->token_name == "annotation")
                    res.annotations = cxx_take<annotation>(it->value);
                else if (it->token_name == "symbol")
                    res.symbol = (art::ustring)it->value;
                else if (it->token_name == "class_body") {
                    std::shared_ptr<component_data> cc = it;
                    while (true) {
                        res.body.push_back(cxx_take<class_body>(it->value));
                        if (cc->args.size() == 2)
                            cc = cc->args[1];
                        else
                            break;
                    }
                } else if (it->token_name == "class_follow") {
                    list_array<art::shared_ptr<component_data>>* iter = &it->args;
                    while (iter) {
                        auto& item = *iter;
                        if (item[0]->token_name == "local_protection") {
                            class_follow_item f_item;
                            f_item.protection = item[0]->value;
                            f_item.symbol = (art::ustring)item[1]->value;
                            res.follows.push_back(std::move(f_item));
                            if (iter->size() > 3)
                                break;
                            iter = &(*iter)[2]->args[1]->args;
                        } else {
                            res.follows.push_back(class_follow_item{.symbol = (art::ustring)item[0]->value});
                            if (iter->size() > 2)
                                break;
                            iter = &(*iter)[1]->args[1]->args;
                        }
                    }
                }
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    struct two_way_operator {
        static void process(component_data& data) {
            data.value = (art::ustring)data.args[0]->raw_token;
        }
    };

    struct one_way_operator : public two_way_operator {};

    struct operators {
        static void process(component_data& data) {
            data.value = std::move(data.args[0]->value);
        }
    };

    struct equation {
        struct get_value {
            util::namespaced_symbol symbol;
        };

        struct function_call {
            util::namespaced_symbol symbol;
            list_array<art::shared_ptr<equation>> args;
        };

        struct provided_value {
            ValueItem value;
        };

        struct get_exception_value {
        };

        struct two_way_operation {
            art::shared_ptr<equation> left;
            art::shared_ptr<equation> right;
            std::string operation;
        };

        struct increment {
            art::shared_ptr<equation> item;
            bool as_left;
        };

        struct decrement {
            art::shared_ptr<equation> item;
            bool as_left;
        };

        struct negation {
            art::shared_ptr<equation> item;
        };

        std::variant<get_value, function_call, provided_value, get_exception_value, two_way_operation, increment, decrement, negation> value;

        static void process(component_data& data) {
            equation res;

            if (data.args.size() == 1) {
                if (data.args[0]->token_name == "namespaced_symbol")
                    res.value = get_value{data.args[0]->value};
                else if (data.args[0]->token_name == "calculated_vars")
                    res.value = provided_value{data.args[0]->value};
                else if (data.args[0]->token_name == "token_exception")
                    res.value = get_exception_value{};
            } else if (data.args.size() == 2) {
                if (data.args[0]->token_name == "equation") {
                    if (data.args[1]->token_name == "token__inc")
                        res.value = increment{.item = new equation(cxx_take<equation>(data.args[0]->value)), .as_left = false};
                    else if (data.args[1]->token_name == "token__dec")
                        res.value = decrement{.item = new equation(cxx_take<equation>(data.args[0]->value)), .as_left = false};
                } else if (data.args[0]->token_name == "token__inc")
                    res.value = increment{.item = new equation(cxx_take<equation>(data.args[1]->value)), .as_left = true};
                else if (data.args[0]->token_name == "token__dec")
                    res.value = decrement{.item = new equation(cxx_take<equation>(data.args[1]->value)), .as_left = true};
                else if (data.args[0]->token_name == "token__logical_not")
                    res.value = negation{.item = new equation(cxx_take<equation>(data.args[1]->value))};
            } else {
                if (data.args[0]->token_name == "equation") {
                    res.value = two_way_operation{
                        .left = new equation(cxx_take<equation>(data.args[0]->value)),
                        .right = new equation(cxx_take<equation>(data.args[0]->value)),
                        .operation = (art::ustring)data.args[1]->value
                    };
                } else {
                    function_call fn_call;
                    fn_call.symbol = data.args[0]->value;
                    if (data.args.size() != 3) {
                        std::shared_ptr<component_data> cc = data.args[2];
                        while (true) {
                            fn_call.args.push_back(new equation(cxx_take<equation>(cc->args[0]->value)));
                            if (cc->args.size() == 2)
                                cc = cc->args[1]->args[1];
                            else
                                break;
                        }
                        res.value = std::move(fn_call);
                    }
                }
            }
            data.value = cxx_construct(std::move(res));
        }
    };

    //
    //"equation = {namespaced_symbol}\n"
    //"equation = {namespaced_symbol} {token_block_begin_operator} [{equation_call}] {token_block_end_operator}\n"
    //"equation = {calculated_vars}\n"
    //"equation = {token_exception}\n"
    //"equation = {equation} {two_way_operator} {equation}\n"
    //"equation = {equation} {token__inc}\n"
    //"equation = {equation} {token__dec}\n"
    //"equation = {token__inc} {equation}\n"
    //"equation = {token__dec} {equation}\n"
    //"equation = {token__logical_not} {equation}\n"
    //
    //"equation_call = {equation} [{token_delimiter_coma} {equation_call}]\n"
    //
    //"body = {token_block_begin_scope} {body_inner} {token_block_end_scope}\n"
    //"body_inner = {body_item} [{body_inner}]\n"
    //
    //"catch_body = {token_block_begin_scope} {catch_body_inner} {token_block_end_scope}\n"
    //"catch_body_inner = {body_item} [{catch_body_inner}]\n"
    //
    //"finally_body = {token_block_begin_scope} {finally_body_inner} {token_block_end_scope}\n"
    //"finally_body_inner = {body_item} [{finally_body_inner}]\n"
    //
    //"filter_body = {token_block_begin_scope} {filter_body_inner} {token_block_end_scope}\n"
    //"filter_body_inner = {body_item} [{filter_body_inner}]\n"
    //
    //
    //"body_item = {equation} {token_delimiter_semicolon}\n"
    //"body_item = {token_try} {body} {try_catch_seq}\n"
    //"body_item = {token_throw} {equation} {token_delimiter_semicolon}\n"
    //"body_item = {type} {symbol} {token_delimiter_semicolon}\n"
    //"body_item = {type} {symbol} {token_block_begin_operator} [{equation_call}] {token_block_end_operator} {token_delimiter_semicolon}\n"
    //"body_item = {type} {symbol} {token__assign} {equation} {token_delimiter_semicolon}\n"
    //"body_item = {token_loop} {body}\n"
    //"body_item = {token_for} {for_loop_iterator} {body}\n"
    //"body_item = {token_while} {while_loop} {body}\n"
    //"body_item = {token_do} {body} {while_loop} {token_delimiter_semicolon}\n"
    //"body_item = {token_if} {if_condition} {body} [{else_sequence}]\n"
    //"body_item = {body}\n"
    //"for_loop_iterator = {token_block_begin_operator} {for_loop} {token_block_end_operator}\n"
    //"for_loop = [{type} {symbol} {token__assign} {equation}] {token_delimiter_semicolon} [{equation}] {token_delimiter_semicolon} {equation}\n"
    //"for_loop = {type} {symbol} {token_delimiter_colon} {equation}\n"
    //"while_loop = {token_block_begin_operator} {equation} {token_block_end_operator}\n"
    //"if_condition = {token_block_begin_operator} {if_equation} {token_block_end_operator}\n"
    //"if_equation = {{type} {symbol} [{token__assign} {equation}] {token_delimiter_semicolon} {if_equation}}|{equation}\n"
    //"else_sequence = {token_else} {{token_if} {if_condition} {body} [{else_sequence}]}|{body}\n"
    //"try_catch_seq = {token_catch} {catch_body} [{try_catch_seq}]\n"
    //"try_catch_seq = {token_finally} {filter_body} [{try_catch_seq}]\n"
    //"try_catch_seq = {token_filter} {filter_body} [{try_catch_seq}]\n"


    void c_async::init() {
    }

    c_async::c_async()
        : art::language::helpers::text_language_handler(language_implantation_declaration) {
    }

    art::patch_list c_async::handle_init_complete() {
        return {}; //TODO
    }
}