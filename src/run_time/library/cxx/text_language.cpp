// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/library/cxx/language.hpp>
#include <sstream>
#include <util/exceptions.hpp>

namespace art {
    namespace language {
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
            std::unordered_set<char> disabled_declaration_symbols_in_header = [] {
                std::unordered_set<char> result;
                result.insert('@');
                result.insert('#'); //processed by parser
                result.insert('^');
                result.insert('$');
                return result;
            }();

            std::unordered_set<char> allowed_space_symbols = {' ', '\t'};

            void text_language_handler::intrinsics_from_token(std::string token_name) {
                if (in_header_part) {
                    if (token_name == "header_end")
                        in_header_part = false;
                    else if (token_name.starts_with("token#")) {
                        art::shared_ptr<token_data> token;
                        token->symbol = token_name.substr(6, token_name.size() - 6);
                        token->token_name = "token_" + token->symbol;
                        get_token((std::string_view)token->token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    } else if (token_name.starts_with("tokens#")) {
                        std::string_view tokens_name = std::string_view(token_name).substr(7, token_name.size() - 7);
                        size_t pos = tokens_name.find('#');
                        do {
                            std::string_view token_name = tokens_name.substr(0, pos);
                            art::shared_ptr<token_data> token;
                            token->symbol = token_name;
                            token->token_name = "token_" + token->symbol;
                            get_token((std::string_view)token->token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                            tokens_name = tokens_name.substr(pos + 1, tokens_name.size() - pos - 1);
                            pos = tokens_name.find('#');
                        } while (pos != std::string::npos);
                    } else
                        throw art::InvalidEncodingException("Invalid intrinsics: " + token_name);
                } else
                    throw art::InvalidEncodingException("Intrinsics is available only in header part, got: " + token_name);
            }

            auto text_language_handler::process_part_item(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration) -> art::shared_ptr<token_data::token_chain> {
                if (auto it = token_declaration.find_first_not_of(' '); it != std::string_view::npos)
                    token_declaration = token_declaration.substr(it, token_declaration.size() - it);

                if (token_declaration.starts_with('[')) {
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                    return process_part_optional(selected_tokens, token_declaration);
                } else if (token_declaration.starts_with('{')) {
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                    return process_part_component_or_variants(selected_tokens, token_declaration);
                } else if (token_declaration.starts_with("$[..]")) {
                    token_declaration = token_declaration.substr(6, token_declaration.size() - 6);
                    return new token_data::token_chain(token_data::token_chain::inline_ref(new token_data::token_chain::sequence(std::move(selected_tokens))));
                } else
                    return nullptr;
            }

            auto text_language_handler::process_part(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view token_declaration) -> token_data::token_chain::inline_ref {
                std::vector<art::shared_ptr<token_data::token_chain>> tokens;
                while (!token_declaration.empty() && token_declaration[0] != '\n') {
                    if (auto res = process_part_item(selected_tokens, token_declaration)) {
                        tokens.push_back(res);
                    } else
                        return {};
                }
                return token_data::token_chain::inline_ref(new token_data::token_chain::sequence(std::move(tokens)));
            }

            auto text_language_handler::process_part_optional(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration) -> art::shared_ptr<token_data::token_chain> {
                std::vector<art::shared_ptr<token_data::token_chain>> tokens;
                while (!token_declaration.empty() && token_declaration[0] != ']' && token_declaration[0] != '\n') {
                    if (auto res = process_part_item(selected_tokens, token_declaration)) {
                        tokens.push_back(res);
                    } else
                        return nullptr;
                }
                return new token_data::token_chain(token_data::token_chain::option(tokens));
            }

            auto text_language_handler::process_part_component_or_variants(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration) -> art::shared_ptr<token_data::token_chain> {
                std::vector<art::shared_ptr<token_data::token_chain>> tokens;
                while (!token_declaration.empty() && token_declaration[0] != '}' && token_declaration[0] != '\n') {
                    if (!allowed_declaration_symbols.contains(token_declaration[0])) {
                        std::vector<art::shared_ptr<token_data::token_chain>> inner_component;
                        while (!token_declaration.empty() && token_declaration[0] != '}' && token_declaration[0] != '\n') {
                            if (auto res = process_part_item(selected_tokens, token_declaration)) {
                                inner_component.push_back(res);
                            } else
                                return nullptr;
                        }
                        tokens.push_back(new token_data::token_chain(token_data::token_chain::sequence(inner_component)));
                    } else {
                        auto close = token_declaration.find('}');
                        auto token_name = token_declaration.substr(0, close);
                        if (close != std::string_view::npos)
                            token_declaration = token_declaration.substr(close + 1, token_declaration.size() - close - 1);
                        else
                            token_declaration = {};
                        tokens.push_back(get_token(token_name));
                        if (!token_declaration.starts_with('|'))
                            break;
                        else {
                            token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                        }
                    }
                }
                if (tokens.size() == 1)
                    return tokens[0];
                return new token_data::token_chain(token_data::token_chain::variants(tokens));
            }

            void text_language_handler::process_token_declaration(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, art::shared_ptr<token_data::token_chain>& token_dec, const std::string& token_name, const std::string& token_declaration, const list_array<std::string>& tags) {
                art::shared_ptr<token_data> token = new token_data();
                token->token_name = token_name;
                token->declaration = process_part(selected_tokens, token_declaration);
                token->tags = tags;
                if (token->declaration->empty())
                    throw art::InvalidEncodingException("Invalid token declaration: " + token_name);
                token_dec->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
            }

            bool get_boolean(std::string_view token_declaration) {
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                if (token_declaration == std::string_view("true"))
                    return true;
                return false;
            }

            std::string get_chars(std::string_view token_declaration) {
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                if (token_declaration.starts_with('[') && token_declaration.ends_with(']'))
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 2);
                bool slash = false;
                std::string res;
                for (auto ch : token_declaration) {
                    if (slash) {
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
                        slash = false;
                    } else if (ch == '\\')
                        slash = true;
                    else
                        res += ch;
                }
                if (slash)
                    res += '\\';
                return res;
            }

            list_array<std::string> get_array_of_chars(std::string_view token_declaration) {
                if (token_declaration.starts_with(' '))
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 1);
                if (token_declaration.starts_with('[') && token_declaration.ends_with(']'))
                    token_declaration = token_declaration.substr(1, token_declaration.size() - 2);
                else
                    throw art::InvalidEncodingException("Invalid array of chars format");

                list_array<std::string> result;
                while (token_declaration.starts_with('[')) {
                    auto end = token_declaration.find_first_of(']');
                    if (end == std::string::npos)
                        throw art::InvalidEncodingException("Invalid array of chars format");
                    result.push_back(get_chars(token_declaration.substr(1, end - 1)));
                    token_declaration = token_declaration.substr(end + 1, token_declaration.size() - end - 1);
                }
                return result;
            }

            auto text_language_handler::get_token(std::string_view name) -> art::shared_ptr<token_data::token_chain>& {
                return tokens[name];
            }

            void text_language_handler::address_token(std::string_view raw_name_with_addressing, std::function<void(std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, list_array<std::string>>&)>&& callback) {
                list_array<std::string> tags;
                art::shared_ptr<token_data::token_chain> set_to;
                list_array<std::pair<art::shared_ptr<token_data::token_chain>, list_array<art::shared_ptr<token_data::token_chain>>>> process_from;
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
                        raw_name_with_addressing = raw_name_with_addressing.substr(pos + 1, raw_name_with_addressing.size() - pos - 1);
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
                                    process_from.push_back({set_to, filter_childs_by_name(set_to, token_name)});
                                } else
                                    for (auto& it : set_to->get_variants())
                                        process_from.push_back({it, filter_childs_by_name(it, token_name)});
                            } else {
                                for (auto& [ignored, inner_arr] : process_from.take()) {
                                    for (auto& it : inner_arr)
                                        process_from.push_back({it, filter_childs_by_name(it, token_name)});
                                }
                            }
                        }
                        break;
                    }
                    case mode_t::tag_filter: {
                        if (!token_name.empty())
                            if (!process_from.empty())
                                for (auto& [ignored, inner_arr] : process_from)
                                    inner_arr = inner_arr.where(filter_by_tag);
                        break;
                    }
                    case mode_t::negate_tag_filter: {
                        if (!token_name.empty())
                            if (!process_from.empty())
                                for (auto& [ignored, inner_arr] : process_from)
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
                        raw_name_with_addressing = raw_name_with_addressing.substr(1, raw_name_with_addressing.size() - 1);
                        break;
                    case '@':
                        current_mode = mode_t::tag_filter;
                        raw_name_with_addressing = raw_name_with_addressing.substr(1, raw_name_with_addressing.size() - 1);
                        break;
                    case '^':
                        if (raw_name_with_addressing.size() > 1)
                            if (raw_name_with_addressing[1] == '@') {
                                current_mode = mode_t::negate_tag_filter;
                                raw_name_with_addressing = raw_name_with_addressing.substr(2, raw_name_with_addressing.size() - 2);
                                break;
                            }
                        throw art::InvalidEncodingException("Invalid token addressing");
                    case '$':
                        current_mode = mode_t::define_tag;
                        raw_name_with_addressing = raw_name_with_addressing.substr(1, raw_name_with_addressing.size() - 1);
                        if (raw_name_with_addressing.find_first_of("@#^") != std::string::npos)
                            throw art::InvalidEncodingException("Tag must declared after addressing");
                        break;
                    };
                }

                if (!process_from.empty()) {
                    for (auto& [check, inner_arr] : process_from) {
                        std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, list_array<std::string>> tuple_res = {
                            inner_arr.take().to_container<std::vector<art::shared_ptr<token_data::token_chain>>>(),
                            check,
                            tags
                        };
                        callback(tuple_res);
                    }
                } else if (set_to) {
                    std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, list_array<std::string>> tuple_res = {
                        {},
                        set_to,
                        tags
                    };
                    callback(tuple_res);
                }
            }

            void text_language_handler::declare_token(art::shared_ptr<token_data::token_chain>& token, const std::string& token_name, const std::string& token_declaration) {
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
                    } else if (token_name == "enable_namespace") {
                        namespace_enabled = get_boolean(token_declaration);
                    } else if (token_name == "language_name") {
                        language_name = token_declaration;
                    } else if (token_name == "language_full_name") {
                        language_full_name = token_declaration;
                    } else if (token_name == "language_version") {
                        language_version = token_declaration;
                    } else if (token_name == "dynamic_patching") {
                        enable_dynamic_patching = get_boolean(token_declaration);
                    } else if (token_name == "token") {
                        art::shared_ptr<token_data> token;
                        std::string set_token_name = get_chars(token_declaration);
                        token->token_name = "token_" + set_token_name;
                        token->symbol = set_token_name;
                        get_token("token_" + set_token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    } else if (token_name == "tokens") {
                        for (auto& token_name : get_array_of_chars(token_declaration)) {
                            art::shared_ptr<token_data> token;
                            token->token_name = "token_" + token_name;
                            token->symbol = token_name;
                            get_token("token_" + token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                        }
                    } else if (token_name.starts_with("token#")) {
                        art::shared_ptr<token_data> token;
                        std::string set_token_name = get_chars(token_declaration);
                        token->token_name = "token_" + token_name.substr(6, token_name.size() - 6);
                        if (token_name.empty())
                            token->token_name = set_token_name;
                        token->symbol = set_token_name;
                        get_token("token_" + set_token_name)->get_variants().emplace_back(new token_data::token_chain(token_data::token_chain::component(token)));
                    }
                } else {
                    if (token_name.find_first_of("@#^$") == std::string::npos) {
                        std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens;
                        process_token_declaration(selected_tokens, token, token_name, token_declaration);
                    } else {
                        address_token(token_name, [&](std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, list_array<std::string>>& result) {
                            auto& [selected_tokens, parent, tags] = result;
                            process_token_declaration(selected_tokens, parent, token_name, token_declaration, tags);
                        });
                    }
                }
            }

            void text_language_handler::declare_conditional_token(art::shared_ptr<token_data::token_chain>& token, const std::string& token_name, const std::string& token_declaration) {
                if (in_header_part) {
                    std::vector<art::shared_ptr<token_data::token_chain>> selected_tokens;
                    process_token_declaration(selected_tokens, token, token_name, token_declaration);
                }
            }

            text_language_handler::text_language_handler(std::string_view language_declaration) {
                bool name_declaration_part = true;
                bool token_declaration_part = false;
                bool conditional_token_part = false;
                std::string token_name;
                std::string token_declaration;
                for (const auto& token : language_declaration) {
                    if (token_declaration_part) {
                        if (token == '\n') {
                            if (conditional_token_part) {
                                token_name = "ctoken_" + token_name;
                                declare_conditional_token(tokens[token_name], token_name, token_declaration);
                            } else
                                declare_token(tokens[token_name], token_name, token_declaration);
                            token_name = "";
                            token_declaration = "";
                            token_declaration_part = false;
                            conditional_token_part = false;
                            name_declaration_part = true;
                            continue;
                        }
                        token_declaration += token;
                        continue;
                    } else if (name_declaration_part) {
                        if (allowed_declaration_symbols.contains(token)) {
                            if (!in_header_part) {
                                token_name += token;
                                continue;
                            } else if (!disabled_declaration_symbols_in_header.contains(token)) {
                                token_name += token;
                                continue;
                            }
                        }
                        if (in_header_part && !conditional_token_part && token == '#') {
                            if (token_name == "decl_cond_token") {
                                conditional_token_part = true;
                                token_name = "";
                                continue;
                            }
                        } else
                            name_declaration_part = false;
                    } else if (allowed_space_symbols.contains(token))
                        continue;
                    else if (token == '=') {
                        token_declaration_part = true;
                        continue;
                    } else if (token == '\n') {
                        intrinsics_from_token(token_name);
                        token_name = "";
                        continue;
                    } else
                        throw art::InvalidEncodingException("Invalid symbol in declaration");
                }
            }

            void text_language_handler::register_processing_handler(std::string_view token_name, std::function<art::shared_ptr<component_data>(list_array<art::shared_ptr<component_data>>&)> handler) {
                art::ustring token_name_u(token_name);
                art::unique_lock unify(rw_mutex);
                if (handlers.find(token_name_u) == handlers.end())
                    handlers[token_name_u] = handler;
                else
                    throw art::AlreadyDefinedException("Token handler already defined for " + token_name);
            }

            void text_language_handler::register_end_handler(std::string_view token_name, std::function<art::patch_list(art::shared_ptr<component_data>&)> handler) {
                art::ustring token_name_u(token_name);
                art::unique_lock unify(rw_mutex);
                if (handlers_end.find(token_name_u) == handlers_end.end())
                    handlers_end[token_name_u] = handler;
                else
                    throw art::AlreadyDefinedException("Token handler already defined for " + token_name);
            }

            art::patch_list text_language_handler::handle_init(art::files::FileHandle& file) {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_init_complete() {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_create(art::files::FileHandle& file) {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_renamed(const art::ustring& old, art::files::FileHandle& file) {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_changed(art::files::FileHandle& file) {
                return art::patch_list();
            }

            art::patch_list text_language_handler::handle_removed(const art::ustring& removed) {
                return art::patch_list();
            }
        }
    }
}