// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/library/cxx/language.hpp>
#include <run_time/tasks.hpp>
#include <sstream>
#include <utf8.h>
#include <util/exceptions.hpp>

namespace art {
    namespace language {
        template <size_t N>
        static constexpr inline bool operator==(std::string_view view, const char (&str)[N]) {
            return view == std::string_view(str);
        }
        namespace helpers {
            auto text_language_handler::token_data::token_chain::get_component() -> component& {
                return std::get<component>(value);
            }

            auto text_language_handler::token_data::token_chain::get_option() -> option& {
                return std::get<option>(value);
            }

            auto text_language_handler::token_data::token_chain::get_variants() -> variants& {
                return std::get<variants>(value);
            }

            auto text_language_handler::token_data::token_chain::get_inline_ref() -> inline_ref& {
                return std::get<inline_ref>(value);
            }

            auto text_language_handler::token_data::token_chain::get_component() const -> const component& {
                return std::get<component>(value);
            }

            auto text_language_handler::token_data::token_chain::get_option() const -> const option& {
                return std::get<option>(value);
            }

            auto text_language_handler::token_data::token_chain::get_variants() const -> const variants& {
                return std::get<variants>(value);
            }

            auto text_language_handler::token_data::token_chain::get_inline_ref() const -> const inline_ref& {
                return std::get<inline_ref>(value);
            }

            bool text_language_handler::token_data::token_chain::is_component() const {
                return std::holds_alternative<component>(value);
            }

            bool text_language_handler::token_data::token_chain::is_option() const {
                return std::holds_alternative<option>(value);
            }

            bool text_language_handler::token_data::token_chain::is_variants() const {
                return std::holds_alternative<variants>(value);
            }

            bool text_language_handler::token_data::token_chain::is_inline_ref() const {
                return std::holds_alternative<inline_ref>(value);
            }

            std::unordered_set<char> allowed_declaration_symbols = [] {
                std::unordered_set<char> result;
                for (char c = 'a'; c <= 'z'; c++)
                    result.insert(c);
                for (char c = 'A'; c <= 'Z'; c++)
                    result.insert(c);
                for (char c = '0'; c <= '9'; c++)
                    result.insert(c);
                result.insert('_');
                result.insert('-');
                result.insert('.');
                result.insert('@');
                result.insert('#');
                result.insert('^');
                result.insert('$');
                return result;
            }();

            std::unordered_set<char> allowed_declaration_symbol_symbols = [] {
                std::unordered_set<char> result;
                for (char c = 'a'; c <= 'z'; c++)
                    result.insert(c);
                for (char c = 'A'; c <= 'Z'; c++)
                    result.insert(c);
                for (char c = '0'; c <= '9'; c++)
                    result.insert(c);
                result.insert('_');
                result.insert('-');
                result.insert('.');
                return result;
            }();
            std::unordered_set<char> disabled_declaration_symbols_in_header = [] {
                std::unordered_set<char> result;
                result.insert('@');
                result.insert('^');
                result.insert('$');
                return result;
            }();

            std::unordered_set<char> allowed_space_symbols = {' ', '\t', '\r'};

            bool get_boolean(std::string_view& token_declaration) {
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1);
                if (token_declaration == std::string_view("true"))
                    return true;
                return false;
            }

            std::string get_chars_process_slash(char ch) {
                std::string res;
                switch (ch) {
                case '\\':
                    res += '\\';
                    break;
                case 'a':
                    res += '\a';
                    break;
                case 'b':
                    res += '\b';
                    break;
                case 'n':
                    res += '\n';
                    break;
                case 'r':
                    res += '\r';
                    break;
                case 't':
                    res += '\t';
                    break;
                case 'e':
                    res += char(101);
                    break;
                case 'p':
                    res += char(112);
                    break;
                case 'f':
                    res += '\f';
                    break;
                case 'v':
                    res += '\v';
                    break;
                case 'B': {
                    for (char c = 'A'; c <= 'Z'; c++)
                        res += c;
                    break;
                }
                case 'L': {
                    for (char c = 'a'; c <= 'z'; c++)
                        res += c;
                    break;
                }
                case 'C': {
                    for (char c = 'a'; c <= 'z'; c++)
                        res += c;
                    for (char c = 'A'; c <= 'Z'; c++)
                        res += c;
                    break;
                }
                case 'N': {
                    for (char c = '0'; c <= '9'; c++)
                        res += c;
                    break;
                }
                case 'S': {
                    for (char c = '0'; c <= '9'; c++)
                        res += c;
                    for (char c = 'a'; c <= 'z'; c++)
                        res += c;
                    for (char c = 'A'; c <= 'Z'; c++)
                        res += c;
                    break;
                }
                default:
                    res += '\\';
                    res += ch;
                }
                return res;
            }

            std::string get_chars(std::string_view& token_declaration) {
                bool as_array = false;
                bool slash = false;
                bool string_completed = false;
                std::string res;
                size_t count = 0;
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1);
                if (token_declaration.starts_with('[')) {
                    token_declaration = token_declaration.substr(1);
                    as_array = true;
                }
                if (as_array) {
                    for (auto ch : token_declaration) {
                        count++;
                        if (slash) {
                            res += get_chars_process_slash(ch);
                            slash = false;
                        } else if (ch == '\\')
                            slash = true;
                        else if (ch == ']') {
                            string_completed = true;
                            break;
                        } else
                            res += ch;
                    }
                } else {
                    for (auto ch : token_declaration) {
                        count++;
                        if (slash) {
                            res += get_chars_process_slash(ch);
                            break;
                        } else if (ch == '\\')
                            slash = true;
                        else {
                            res += ch;
                            break;
                        }
                    }
                    string_completed = true;
                }
                if (!string_completed)
                    throw art::InvalidSyntaxException("Invalid string format");
                if (slash)
                    res += '\\';
                token_declaration = token_declaration.substr(count);
                return res;
            }

            list_array<std::string> get_array_of_chars(std::string_view& token_declaration) {
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1);
                if (token_declaration.starts_with('['))
                    token_declaration = token_declaration.substr(1);
                list_array<std::string> result;
                while (token_declaration.starts_with('[')) {
                    result.push_back(get_chars(token_declaration));
                }
                if (!token_declaration.starts_with(']'))
                    throw art::InvalidSyntaxException("Invalid array of strings format");
                return result;
            }

            void text_language_handler::intrinsics_from_token(std::string token_name) {
                if (in_header_part) {
                    if (token_name == "header_end")
                        in_header_part = false;
                    else if (token_name.starts_with("token#")) {
                        art::shared_ptr<token_data> token = new token_data();
                        token->symbol = token_name.substr(6);
                        token->token_name = "token_" + token->symbol;
                        token->entry_token = true;
                        get_token((std::string_view)token->token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    } else if (token_name.starts_with("tokens#")) {
                        std::string_view tokens_name = std::string_view(token_name).substr(7);
                        size_t pos = tokens_name.find('#');
                        do {
                            std::string_view token_name = tokens_name.substr(0, pos);
                            art::shared_ptr<token_data> token = new token_data();
                            token->symbol = token_name;
                            token->token_name = "token_" + token->symbol;
                            token->entry_token = true;
                            get_token((std::string_view)token->token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                            tokens_name = tokens_name.substr(pos + 1);
                            pos = tokens_name.find('#');
                        } while (pos != std::string::npos);
                    } else if (token_name.starts_with("value_processing#")) {
                        auto config = token_name.substr(17);
                        if (config == "string_escape") {
                            configure_value_processing(config, "@[\\]");
                        } else if (config == "decimal_dot") {
                            configure_value_processing(config, "@[.]");
                        } else if (config == "string_scope") {
                            configure_value_processing(config, "@[\"\"\"] $[..] @[\"\"\"]");
                        } else if (config == "string_line") {
                            configure_value_processing(config, "@[\"] $[..] @[\"]");
                        } else if (config == "char") {
                            configure_value_processing(config, "@['] $[..] @[']");
                        } else if (config == "hex_number_enable") {
                            configure_value_processing(config, "");
                        } else if (config == "octal_number_enable") {
                            configure_value_processing(config, "");
                        } else if (config == "binary_number_enable") {
                            configure_value_processing(config, "");
                        } else
                            throw art::InvalidSyntaxException("Invalid value_processing intrinsics: " + config);
                    } else
                        throw art::InvalidSyntaxException("Invalid intrinsics: " + token_name);
                } else
                    throw art::InvalidSyntaxException("Intrinsics is available only in header part, got: " + token_name);
            }

            auto text_language_handler::process_part_item(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration) -> art::shared_ptr<token_data::token_chain> {
                while (token_declaration.starts_with(' ') || token_declaration.starts_with('\t') || token_declaration.starts_with('\r'))
                    token_declaration = token_declaration.substr(1);
                auto res = [&]() -> art::shared_ptr<token_data::token_chain> {
                    if (token_declaration.starts_with('[') || token_declaration.starts_with('{')) {
                        return process_part_component(selected_tokens, token_declaration);
                    } else if (token_declaration.starts_with("$[..]")) {
                        token_declaration = token_declaration.substr(5);
                        return new token_data::token_chain(token_data::token_chain::inline_ref(new token_data::token_chain::sequence(selected_tokens)));
                    } else if (token_declaration.starts_with("@[")) {
                        token_declaration = token_declaration.substr(1);
                        return new token_data::token_chain(token_data::token_chain::inline_decl(get_chars(token_declaration)));
                    } else
                        return nullptr;
                }();
                while (token_declaration.starts_with(' ') || token_declaration.starts_with('\t') || token_declaration.starts_with('\r'))
                    token_declaration = token_declaration.substr(1);
                return res;
            }

            auto text_language_handler::process_part(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view token_declaration) -> token_data::token_chain::inline_ref {
                std::vector<art::shared_ptr<token_data::token_chain>> tokens;
                while (!token_declaration.empty() && token_declaration[0] != '\n') {
                    if (auto res = process_part_item(selected_tokens, token_declaration))
                        tokens.push_back(res);
                    else
                        return {};
                }
                return token_data::token_chain::inline_ref(new token_data::token_chain::sequence(std::move(tokens)));
            }

            auto text_language_handler::process_part_component(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration) -> art::shared_ptr<token_data::token_chain> {
                std::vector<art::shared_ptr<token_data::token_chain>> variants;
                std::vector<art::shared_ptr<token_data::token_chain>> tokens;
                bool loop = false;
                do {
                    if (loop)
                        token_declaration = token_declaration.substr(1);
                    if (token_declaration.starts_with('{')) {
                        token_declaration = token_declaration.substr(1);
                        if (!allowed_declaration_symbol_symbols.contains(token_declaration[0])) {
                            while (!token_declaration.empty() && token_declaration[0] != '}' && token_declaration[0] != '\n') {
                                if (auto res = process_part_item(selected_tokens, token_declaration)) {
                                    tokens.push_back(res);
                                } else
                                    return nullptr;
                            }
                            variants.push_back(new token_data::token_chain(token_data::token_chain::sequence(std::move(tokens))));
                        } else {
                            auto close = token_declaration.find('}');
                            auto token_name = token_declaration.substr(0, close);
                            if (close != std::string_view::npos)
                                token_declaration = token_declaration.substr(close);
                            else
                                token_declaration = {};
                            variants.push_back(get_token(token_name));
                        }
                        token_declaration = token_declaration.substr(1);
                    } else if (token_declaration.starts_with('[')) {
                        token_declaration = token_declaration.substr(1);
                        while (!token_declaration.empty() && token_declaration[0] != ']' && token_declaration[0] != '\n') {
                            if (auto res = process_part_item(selected_tokens, token_declaration)) {
                                tokens.push_back(res);
                            } else
                                return nullptr;
                        }
                        variants.push_back(new token_data::token_chain(token_data::token_chain::option(std::move(tokens))));
                        token_declaration = token_declaration.substr(1);
                    } else if (token_declaration.starts_with("$[..]")) {
                        token_declaration = token_declaration.substr(5);
                        variants.push_back(new token_data::token_chain(token_data::token_chain::inline_ref(new token_data::token_chain::sequence(selected_tokens))));
                    } else if (token_declaration.starts_with("@[")) {
                        token_declaration = token_declaration.substr(1);
                        variants.push_back(new token_data::token_chain(token_data::token_chain::inline_decl(get_chars(token_declaration))));
                    }
                } while (loop = token_declaration.starts_with('|'));
                if (variants.size() == 1)
                    return variants[0];
                return new token_data::token_chain(token_data::token_chain::variants(std::move(variants)));
            }

            void text_language_handler::process_token_declaration(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, art::shared_ptr<token_data::token_chain>& token_dec, const std::string& token_name, const std::string& token_declaration, const list_array<std::string>& tags) {
                art::shared_ptr<token_data> token = new token_data();
                token->token_name = token_name;
                token->declaration = process_part(selected_tokens, token_declaration);
                token->tags = tags;
                if (token->declaration->empty())
                    throw art::InvalidSyntaxException("Invalid token declaration: " + token_name);
                token_dec->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
            }

            auto text_language_handler::get_token(std::string_view name) -> art::shared_ptr<token_data::token_chain>& {
                if (name.contains('#'))
                    throw art::InvalidSyntaxException("Invalid token name: " + std::string(name));
                auto it = tokens.find(name);
                if (it != tokens.end())
                    return it->second;
                else
                    return tokens[name] = new token_data::token_chain(token_data::token_chain::variants());
            }

            void text_language_handler::address_token(std::string_view raw_name_with_addressing, std::function<void(std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, const list_array<std::string>&, const std::string&>&)>&& callback) {
                list_array<std::string> tags;
                art::shared_ptr<token_data::token_chain> set_to;

                struct processor {
                    std::string name;
                    art::shared_ptr<token_data::token_chain> set_to; //duplicate
                    list_array<art::shared_ptr<token_data::token_chain>> selected_childs;
                };

                list_array<processor> process_from;
                enum class mode_t {
                    init,
                    multi_addressing,  //#
                    tag_filter,        //@
                    negate_tag_filter, //^@
                    define_tag,        //$
                } current_mode = mode_t::init;


                auto filter_childs_by_name = [&](art::shared_ptr<token_data::token_chain>& p, std::string_view name) -> list_array<art::shared_ptr<token_data::token_chain>> {
                    list_array<art::shared_ptr<token_data::token_chain>> res;
                    auto process_childs = [&](this auto& process_childs, token_data::token_chain::inline_ref& p) {
                        auto process_item = [&](this auto& process_item, art::shared_ptr<token_data::token_chain>& v) {
                            std::visit(
                                [&](auto& v2) {
                                    using T = std::decay_t<decltype(v)>;
                                    if constexpr (std::is_same_v<T, token_data::token_chain::component>) {
                                        if (v2->token_name == name)
                                            res.push_back(v);
                                    } else if constexpr (std::is_same_v<T, token_data::token_chain::inline_ref>) {
                                        process_childs(v2);
                                    } else if constexpr (std::is_same_v<T, token_data::token_chain::option>) {
                                        if (v2.size() == 1)
                                            process_item(v2[0]);
                                    }
                                },
                                v->value
                            );
                        };
                        for (auto& v : *p)
                            process_item(v);
                    };

                    auto process_item = [&](this auto& process_item, art::shared_ptr<token_data::token_chain>& it) {
                        std::visit(
                            [&](auto& v) {
                                using T = std::decay_t<decltype(v)>;
                                if constexpr (std::is_same_v<T, token_data::token_chain::component>) {
                                    process_childs(v->declaration);
                                } else if constexpr (std::is_same_v<T, token_data::token_chain::option>) {
                                    if (v.size() == 1)
                                        process_item(v[0]);
                                }
                            },
                            it->value
                        );
                    };
                    std::visit(
                        [&](auto& item) {
                            using T = std::decay_t<decltype(item)>;
                            if constexpr (std::is_same_v<T, token_data::token_chain::variants>) {
                                for (auto& it : item)
                                    process_item(it);
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::component>) {
                                process_childs(item->declaration);
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::option>) {
                                if (item.size() == 1)
                                    process_item(item[0]);
                            }
                        },
                        p->value
                    );

                    return res;
                };

                std::string token_name;
                auto filter_by_tag = [&](this auto& filter_by_tag, const art::shared_ptr<token_data::token_chain>& it) -> bool {
                    return std::visit(
                        [&](auto& v) {
                            using T = std::decay_t<decltype(v)>;
                            if constexpr (std::is_same_v<T, token_data::token_chain::component>) {
                                return v->tags.contains(token_name);
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::option>) {
                                if (v.size() == 1)
                                    return filter_by_tag(v[0]);
                            }
                            return false;
                        },
                        it->value
                    );
                };
                while (!raw_name_with_addressing.empty()) {
                    auto pos = raw_name_with_addressing.find_first_of("@#^$");
                    token_name = raw_name_with_addressing.substr(0, pos);
                    if (pos != std::string::npos)
                        raw_name_with_addressing = raw_name_with_addressing.substr(pos + 1);
                    else
                        raw_name_with_addressing = "";
                    switch (current_mode) {
                    case mode_t::init:
                        if (!token_name.empty())
                            set_to = get_token(token_name);
                        break;
                    case mode_t::multi_addressing: {
                        if (!token_name.empty()) {
                            if (process_from.empty()) {
                                if (!set_to) {
                                    set_to = get_token(token_name);
                                    process_from.push_back({token_name, set_to, filter_childs_by_name(set_to, token_name)});
                                } else
                                    for (auto& it : set_to->get_variants())
                                        process_from.push_back({token_name, it, filter_childs_by_name(it, token_name)});
                            } else {
                                for (auto& [name, ignored, inner_arr] : process_from.take()) {
                                    for (auto& it : inner_arr)
                                        process_from.push_back({token_name, it, filter_childs_by_name(it, token_name)});
                                }
                            }
                        }
                        break;
                    }
                    case mode_t::tag_filter: {
                        if (!token_name.empty())
                            if (!process_from.empty())
                                for (auto& [name, ignored, inner_arr] : process_from)
                                    inner_arr = inner_arr.where(filter_by_tag);
                        break;
                    }
                    case mode_t::negate_tag_filter: {
                        if (!token_name.empty())
                            if (!process_from.empty())
                                for (auto& [name, ignored, inner_arr] : process_from)
                                    inner_arr = inner_arr.where([&](auto& it) { return !filter_by_tag(it); });
                        break;
                    }
                    case mode_t::define_tag: {
                        if (!token_name.empty())
                            tags.push_back(token_name);
                        break;
                    }

                    default:
                        break;
                    }
                    if (raw_name_with_addressing.empty())
                        break;
                    switch (raw_name_with_addressing[0]) {
                    case '#':
                        current_mode = mode_t::multi_addressing;
                        raw_name_with_addressing = raw_name_with_addressing.substr(1);
                        break;
                    case '@':
                        current_mode = mode_t::tag_filter;
                        raw_name_with_addressing = raw_name_with_addressing.substr(1);
                        break;
                    case '^':
                        if (raw_name_with_addressing.size() > 1)
                            if (raw_name_with_addressing[1] == '@') {
                                current_mode = mode_t::negate_tag_filter;
                                raw_name_with_addressing = raw_name_with_addressing.substr(2);
                                break;
                            }
                        throw art::InvalidSyntaxException("Invalid token addressing");
                    case '$':
                        current_mode = mode_t::define_tag;
                        raw_name_with_addressing = raw_name_with_addressing.substr(1);
                        if (raw_name_with_addressing.find_first_of("@#^") != std::string::npos)
                            throw art::InvalidSyntaxException("Tag must declared after addressing");
                        break;
                    };
                }


                if (!process_from.empty()) {
                    for (auto& [name, check, inner_arr] : process_from) {
                        std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, const list_array<std::string>&, const std::string&> tuple_res = {
                            inner_arr.take().to_container<std::vector<art::shared_ptr<token_data::token_chain>>>(),
                            check,
                            tags,
                            name
                        };
                        callback(tuple_res);
                    }
                } else if (set_to) {
                    std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, const list_array<std::string>&, const std::string&> tuple_res = {
                        {},
                        set_to,
                        tags,
                        token_name
                    };
                    callback(tuple_res);
                }
            }

            void text_language_handler::declare_token(const std::string& token_name, const std::string& token_declaration_) {
                std::string_view token_declaration = token_declaration_;
                if (in_header_part) {
                    if (token_name == "ignored_chars") {
                        for (auto ch : get_chars(token_declaration))
                            ignored_chars.insert(ch);
                    } else if (token_name == "delimiting_chars") {
                        for (auto ch : get_chars(token_declaration))
                            delimiting_chars.insert(ch);
                    } else if (token_name == "keep_delimiting_chars") {
                        for (auto& chars : get_array_of_chars(token_declaration)) {
                            if (chars.size())
                                keep_delimiting_chars[chars[0]].push_back(chars);
                        }
                    } else if (token_name == "allowed_symbol_chars") {
                        for (auto ch : get_chars(token_declaration))
                            allowed_symbol_chars.insert(ch);
                    } else if (token_name == "namespace_symbol_sequence") {
                        namespace_symbol_sequence = get_chars(token_declaration);
                        namespace_enabled = true;
                    } else if (token_name == "language_name") {
                        language_name = token_declaration;
                    } else if (token_name == "language_full_name") {
                        language_full_name = token_declaration;
                    } else if (token_name == "language_extensions") {
                        language_extensions = get_array_of_chars(token_declaration);
                    } else if (token_name == "language_version") {
                        language_version = token_declaration;
                    } else if (token_name == "dynamic_patching") {
                        enable_dynamic_patching = get_boolean(token_declaration);
                    } else if (token_name == "token") {
                        art::shared_ptr<token_data> token = new token_data();
                        std::string set_token_name = get_chars(token_declaration);
                        token->token_name = "token_" + set_token_name;
                        token->symbol = set_token_name;
                        token->entry_token = true;
                        get_token("token_" + set_token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    } else if (token_name == "tokens") {
                        for (auto& token_name : get_array_of_chars(token_declaration)) {
                            art::shared_ptr<token_data> token = new token_data();
                            token->token_name = "token_" + token_name;
                            token->symbol = token_name;
                            token->entry_token = true;
                            get_token("token_" + token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                        }
                    } else if (token_name.starts_with("token#")) {
                        std::string set_token_name = "token_" + token_name.substr(6);
                        art::shared_ptr<token_data> token = new token_data();
                        token->token_name = set_token_name;
                        token->symbol = get_chars(token_declaration);
                        token->entry_token = true;
                        if (token->symbol.empty())
                            token->symbol = token_name.substr(6);
                        get_token(set_token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    } else if (token_name.starts_with("value_processing#")) {
                        configure_value_processing(token_name.substr(17), token_declaration_);
                    } else if (token_name == "entry_components") {
                        entry_points += get_array_of_chars(token_declaration);
                    } else
                        art::InvalidSyntaxException("Declaring components is not allowed in header part.");
                } else {
                    if (token_name.find_first_of("@#^$") == std::string::npos) {
                        std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens;
                        process_token_declaration(selected_tokens, get_token(token_name), token_name, token_declaration_);
                    } else {
                        address_token(token_name, [&](std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, const list_array<std::string>&, const std::string&>& result) {
                            auto& [selected_tokens, parent, tags, token_name] = result;
                            process_token_declaration(selected_tokens, parent, token_name, token_declaration_, tags);
                        });
                    }
                }
            }

            void text_language_handler::declare_compound_token(const std::string& token_name, const std::string& token_declaration) {
                if (in_header_part) {
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens;
                    process_token_declaration(selected_tokens, get_token(token_name), token_name, token_declaration);
                }
            }

            void text_language_handler::configure_value_processing(const std::string& config_name, const std::string& token_declaration) {
                if (config_name == "string_escape") {
                    std::string_view vv(token_declaration);
                    string_escape = get_chars(vv)[0];
                } else if (config_name == "string_scope") {
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens{
                        new token_data::token_chain{
                            token_data::token_chain::process_string_scope()
                        }
                    };
                    process_token_declaration(selected_tokens, get_token("value_string"), "value_string", token_declaration);
                } else if (config_name == "decimal_dot") {
                    art::shared_ptr<token_data> token = new token_data();
                    token->token_name = "value_double";
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens;
                    token->declaration = process_part(selected_tokens, token_declaration);
                    token->declaration->insert(token->declaration->begin(), get_token("value_long"));
                    token->declaration->push_back(get_token("value_long"));
                    get_token("value_double")->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));

                    register_processing_handler("value_double", [](component_data& data) {
                        data.value = std::stod((art::ustring)data.args[0]->value + '.' + (art::ustring)data.args[1]->value);
                    });
                } else if (config_name == "string_line") {
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens{
                        new token_data::token_chain{
                            token_data::token_chain::process_string_line()
                        }
                    };
                    process_token_declaration(selected_tokens, get_token("value_string"), "value_string", token_declaration);
                } else if (config_name == "char") {
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens{
                        new token_data::token_chain{
                            token_data::token_chain::process_char()
                        }
                    };
                    process_token_declaration(selected_tokens, get_token("value_char"), "value_char", token_declaration);
                    register_processing_handler("value_char", [](component_data& data) {
                        for (auto& it : data.args) {
                            if (it->value.meta.vtype == VType::character)
                                return (char32_t)it->value;
                            else if (it->value.meta.vtype == VType::string)
                                return it->value.retrieve_ref<art::ustring>().get(0);
                        }
                        throw art::InvalidSyntaxException("Invalid value_char intrinsics declaration");
                    });
                } else if (config_name == "hex_number_enable") {
                    art::shared_ptr<token_data> token = new token_data();
                    token->token_name = "value_long";
                    token->declaration = make_sequence_chain_ref(token_data::token_chain::process_hex_num());
                    get_token("value_long")->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                } else if (config_name == "octal_number_enable") {
                    art::shared_ptr<token_data> token = new token_data();
                    token->token_name = "value_long";
                    token->declaration = make_sequence_chain_ref(token_data::token_chain::process_octal_num());
                    get_token("value_long")->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                } else if (config_name == "binary_number_enable") {
                    art::shared_ptr<token_data> token = new token_data();
                    token->token_name = "value_long";
                    token->declaration = make_sequence_chain_ref(token_data::token_chain::process_binary_num());
                    get_token("value_long")->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                } else
                    throw art::InvalidSyntaxException("Invalid value_processing intrinsics: " + config_name);
            }

            text_language_handler::text_language_handler(std::string_view language_declaration) {
                {
                    art::shared_ptr<token_data> token = new token_data();
                    token->token_name = "symbol";
                    token->declaration = make_sequence_chain_ref(token_data::token_chain::inline_symbol());
                    get_token("symbol")->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                }
                bool name_declaration_part = true;
                bool token_declaration_part = false;
                bool compound_token_part = false;
                std::string token_name;
                std::string token_declaration;
                for (const auto& token : language_declaration) {
                    if (token_declaration_part) {
                        if (token == '\n') {
                            if (compound_token_part)
                                declare_compound_token("ctoken_" + token_name, token_declaration);
                            else
                                declare_token(token_name, token_declaration);
                            token_name = "";
                            token_declaration = "";
                            token_declaration_part = false;
                            compound_token_part = false;
                            name_declaration_part = true;
                            continue;
                        }
                        token_declaration += token;
                        continue;
                    } else if (name_declaration_part) {
                        if (in_header_part && !compound_token_part && token == '#') {
                            if (token_name == "compound_token") {
                                compound_token_part = true;
                                token_name = "";
                                continue;
                            }
                        }
                        if (allowed_declaration_symbols.contains(token)) {
                            if (!in_header_part) {
                                token_name += token;
                                continue;
                            } else if (!disabled_declaration_symbols_in_header.contains(token)) {
                                token_name += token;
                                continue;
                            }
                        } else if (allowed_space_symbols.contains(token))
                            continue;
                        else if (token == '\n') {
                            intrinsics_from_token(token_name);
                            token_name = "";
                            token_declaration_part = false;
                            compound_token_part = false;
                            name_declaration_part = true;
                            continue;
                        } else if (token == '=') {
                            token_declaration_part = true;
                            name_declaration_part = false;
                            continue;
                        } else
                            name_declaration_part = false;
                    } else
                        throw art::InvalidSyntaxException("Invalid symbol in declaration");
                }
                declaration_complete();
            }

            text_language_handler::ast_data::proc_rul::proc_rul(const std::vector<ast_data*>& __vars) {
                v = var::var_vars;
                arr_siz = __vars.size();
                vars = new ast_data*[arr_siz];
                for (size_t i = 0; i < arr_siz; i++)
                    vars[i] = __vars[i];
            }

            text_language_handler::ast_data::proc_rul::~proc_rul() {
                switch (v) {
                case var::var_c:
                    break;
                case var::var_s:
                    delete s;
                    break;
                case var::var_ps:
                case var::var_pl:
                case var::var_pc:
                case var::var_ph:
                case var::var_po:
                case var::var_pb:
                    break;
                case var::var_vars:
                    delete[] vars;
                }
            }

            void text_language_handler::declaration_complete() {
                {
                    register_processing_handler("symbol", [](component_data& data) {
                        data.value = (art::ustring)data.raw_token;
                    });
                    if (namespace_enabled) {
                        register_processing_handler("namespaced_symbol", [](component_data& data) {
                            bool acquire = true;
                            list_array<ValueItem> nms_s;
                            for (auto& it : data.args) {
                                if (acquire) {
                                    nms_s.emplace_back((art::ustring)it->raw_token);
                                    acquire = false;
                                } else {
                                    acquire = true;
                                }
                            }
                            data.value = std::move(nms_s);
                        });
                    }
                }
                if (tokens.contains("value_string")) {
                    register_processing_handler("value_string", [](component_data& data) {
                        for (auto& it : data.args) {
                            if (it->token_name == "process_string_scope") {
                                data.value = (art::ustring)it->raw_token;
                                break;
                            } else if (it->token_name == "process_string_line") {
                                data.value = (art::ustring)it->raw_token;
                                break;
                            }
                        }
                    });
                }
                if (tokens.contains("value_long")) {
                    register_processing_handler("value_long", [](component_data& data) {
                        data.value = std::stoll((std::string)data.raw_token);
                    });
                }
                std::unordered_map<void*, ast_data*> visited;
                ast.reserve(tokens.size());
                for (auto it : tokens) {
                    for (auto& handle : it.second->get_variants())
                        if (visited.find(&handle) == visited.end()) {
                            auto& comp = *handle->get_component();
                            if (comp.entry_token)
                                if (!comp.symbol.empty()) {
                                    auto it = &ast.emplace_back(ast_data({}, comp.symbol));
                                    entry_ast.push_back(it);
                                    comp.assigned_ast = it;
                                    visited.insert_or_assign(&handle, it);
                                }
                        }
                }
                for (auto it : tokens) {
                    for (auto& handle : it.second->get_variants()) {
                        if (!handle->get_component()->entry_token)
                            recursive_declaration_complete(visited, handle);
                    }
                }
                //end
                tokens.clear();
                entry_ast.shrink_to_fit();
            }

            auto text_language_handler::recursive_declaration_complete(std::unordered_map<void*, ast_data*>& visited, art::shared_ptr<token_data::token_chain>& token) -> ast_data* {
                if (visited.find(&token) == visited.end()) {
                    auto it = visited.insert_or_assign(&token, nullptr);
                    auto res = std::visit(
                        [&](auto& handle) {
                            using T = std::decay_t<decltype(handle)>;
                            if constexpr (std::is_same_v<T, token_data::token_chain::component>) {
                                auto& comp = *handle;
                                if (!comp.symbol.empty()) {
                                    auto& it = ast.emplace_back(ast_data((std::string)comp.token_name, comp.symbol));
                                    comp.assigned_ast = &it;
                                    return &it;
                                } else {
                                    std::vector<ast_data*> v;
                                    v.reserve(comp.declaration->size());
                                    for (auto& decl : *comp.declaration)
                                        v.push_back(recursive_declaration_complete(visited, decl));

                                    auto& it = ast.emplace_back(ast_data((std::string)comp.token_name, v));
                                    comp.assigned_ast = &it;
                                    it.cmd.is_sequence = true;
                                    return &it;
                                }
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::inline_decl>) {
                                return &ast.emplace_back(ast_data({}, handle.symbol));
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::option>) {
                                if (handle.size() == 1) {
                                    auto it = recursive_declaration_complete(visited, handle[0]);
                                    it->cmd.is_optional = true;
                                    return it;
                                }
                                std::vector<ast_data*> v;
                                v.reserve(handle.size());
                                for (auto& decl : handle)
                                    v.push_back(recursive_declaration_complete(visited, decl));

                                auto& it = ast.emplace_back(ast_data({}, v));
                                it.cmd.is_sequence = true;
                                it.cmd.is_optional = true;
                                return &it;
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::inline_ref>) {
                                std::vector<ast_data*> v;
                                v.reserve(handle->size());
                                for (auto& decl : *handle)
                                    v.push_back(recursive_declaration_complete(visited, decl));
                                auto& it = ast.emplace_back(ast_data({}, v));
                                it.cmd.is_sequence = true;
                                return &it;
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::sequence>) {
                                std::vector<ast_data*> v;
                                v.reserve(handle.size());
                                for (auto& decl : handle)
                                    v.push_back(recursive_declaration_complete(visited, decl));
                                auto& it = ast.emplace_back(ast_data({}, v));
                                it.cmd.is_sequence = true;
                                return &it;
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::variants>) {
                                std::vector<ast_data*> v;
                                v.reserve(handle.size());
                                for (auto& decl : handle)
                                    v.push_back(recursive_declaration_complete(visited, decl));
                                auto& it = ast.emplace_back(ast_data({}, v));
                                return &it;
                            } else if constexpr (std::is_same_v<T, token_data::token_chain::process_binary_num>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_binary_num{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::process_char>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_char{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::process_hex_num>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_hex_num{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::process_octal_num>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_octal_num{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::process_string_line>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_string_line{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::process_string_scope>)
                                return &ast.emplace_back(ast_data({}, ast_data::process_string_scope{}));
                            else if constexpr (std::is_same_v<T, token_data::token_chain::inline_symbol>)
                                return &ast.emplace_back(ast_data({}, ast_data::inline_symbol{}));
                        },
                        token->value
                    );
                    it.first->second = res;
                    return res;
                } else
                    return visited.at(&token);
            }

            void text_language_handler::register_processing_handler(std::string_view token_name, std::function<void(component_data&)> handler) {
                art::ustring token_name_u(token_name);
                art::unique_lock unify(rw_mutex);
                if (handlers.find(token_name_u) == handlers.end())
                    handlers[token_name_u] = handler;
                else
                    throw art::AlreadyDefinedException("Token handler already defined for " + token_name);
            }

            void text_language_handler::register_end_handler(std::string_view token_name, std::function<art::patch_list(component_data&, text_language_handler&)> handler) {
                art::ustring token_name_u(token_name);
                art::unique_lock unify(rw_mutex);
                if (handlers_end.find(token_name_u) == handlers_end.end())
                    handlers_end[token_name_u] = handler;
                else
                    throw art::AlreadyDefinedException("Token handler already defined for " + token_name);
            }

            art::patch_list text_language_handler::parse_file(art::files::FileHandle& file) {
                shared_lock guard(rw_mutex);
                auto& declared_functions = this->declared_functions[file.get_path()];
                auto& declared_types = this->declared_types[file.get_path()];

                component_data components;
                auto process_component = [this](this auto& process_component, component_data& component) -> void {
                    for (auto& it : component.args)
                        process_component(*it);
                    handlers.at(component.token_name)(component);
                };
                auto process_component_start = [this, &process_component](component_data& component) {
                    for (auto& it : component.args)
                        process_component(*it);

                    return handlers_end.at(component.token_name)(component, *this);
                };


                list_array<std::pair<std::string_view, ast_data*>> variants;
                uint8_t c;
                std::string closing_token;
                enum class states {
                    global_processing,
                    string_processing,
                } current_state;

                art::patch_list patches;

                auto reset_variants = [&]() {
                    variants.clear();
                    for (auto& it : entry_ast)
                        variants.push_back({(std::string_view)*it->cmd.s, it});
                };

                auto reached_end = [&]() {
                    if (current_state == states::string_processing)
                        throw art::InvalidSyntaxException("Unterminated string");
                    patches.add_patches(process_component_start(components));
                    components = {};
                };
                while (file.read(&c, 1) == 1) {
                }

                return patches;
            }

            art::patch_list text_language_handler::handle_init(art::files::FileHandle& file) {
                return parse_file(file);
            }

            art::patch_list text_language_handler::handle_init_complete() {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_create(art::files::FileHandle& file) {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_renamed(const art::ustring& old, art::files::FileHandle& file) {
                if (enable_dynamic_patching) {
                    {
                        lock_guard guard(rw_mutex);
                        declared_functions.insert_or_assign(file.get_path(), std::move(declared_functions.at(file.get_path())));
                        declared_functions.erase(old);
                        declared_types.insert_or_assign(file.get_path(), std::move(declared_types.at(file.get_path())));
                        declared_types.erase(old);
                    }
                    return parse_file(file);
                } else
                    return art::patch_list();
            }

            art::patch_list text_language_handler::handle_changed(art::files::FileHandle& file) {
                return parse_file(file);
            }

            art::patch_list text_language_handler::handle_removed(const art::ustring& removed) {
                if (enable_dynamic_patching) {
                    art::patch_list res;
                    unique_lock guard(rw_mutex);
                    auto declared_functions_file = std::move(declared_functions.at(removed));
                    auto declared_types_file = std::move(declared_types.at(removed));
                    declared_functions.erase(removed);
                    declared_types.erase(removed);
                    guard.unlock();
                    for (auto& func : declared_functions_file)
                        res.undefine_function(func.first);
                    for (auto& type : declared_types_file)
                        res.undefine_type(type.first);
                    return res;
                } else
                    return art::patch_list();
            }
        }
    }
}