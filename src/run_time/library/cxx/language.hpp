// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_LIBRARY_CXX_LANGUAGE
#define SRC_RUN_TIME_LIBRARY_CXX_LANGUAGE
#include <optional>
#include <run_time/func_enviro_builder.hpp>
#include <run_time/library/cxx/files.hpp>
#include <variant>

namespace art {
    namespace language {
        //language_handler can do anything with file and runtime,
        //but handler intended to compile sources with art::FuncEnviroBuilder and return patch list, to be handled by runtime
        class language_handler {
        public:
            virtual art::patch_list handle_init(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_init_complete() = 0;
            virtual art::patch_list handle_create(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_renamed(const art::ustring& old, art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_changed(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_removed(const art::ustring& removed) = 0;
        };

        class language_provider {
            std::unordered_map<art::ustring, art::shared_ptr<language_handler>, art::hash<art::ustring>> languages;
            art::ValueItem folder_monitor;
            bool init_mode = true;
            art::TaskRWMutex rw_mutex;
            art::patch_list patches;

        public:
            language_provider(std::string_view path, bool include_sub_directories);
            void register_language(std::string_view name, art::shared_ptr<language_handler> decoder);
            void unregister_language(std::string_view name);

            void run_once();

            void start();
            void stop();
        };

        namespace helpers {
            struct component_data;
            using component = std::variant<art::shared_ptr<component_data>, list_array<art::shared_ptr<component_data>>, art::ustring>;

            struct component_data {
                art::ustring token_name;
                art::ValueItem value; // processing result
                list_array<art::shared_ptr<component_data>> inner_components;
            };

            //symbol is instritic created after header_end
            //value_double,value_hex, value_long, value_string, value_char is instritic from parser
            //decl_cond_token#* declares ctoken_* custom tokens, if token not set then used token name
            //   if it requires another tokens then use @{...} to use them in this scope
            //after header_end starts language declaration
            //every declaration is creates and adds variants to component
            //every component must be assigned to handler after initialization of declaration
            //to use component from another component use {component_name} to use it
            //to use multiply components as variants use {component_name1}|{component_name2} without spaces    | in handler seen as one component of variants
            //to add optional component use [...usages...]                                                     | in handler seen as array of components
            //to add custom components to specified component after another use
            //       main_component_name#inner_component = custom_declaration
            //   if there more than one component and you need to add in specified component, use index
            //       main_component_name#inner_component[4] = custom_declaration
            //to address component use #
            //   addressing allowed only to components or to option with one component, otherwise will be ignored but inline_ref threated as same level of array where components checked
            //to add tag to component use $
            //       struct$with_body$followed = ...
            //   later it can be used to addres it with tag filter @
            //       main_component_name#struct@with_body#body = ...
            //       #main_component_name$no_struct = ...
            //   tag also can be added to selected obj
            //       main_component_name#struct@with_body#body@another_tag$an_t2 = ...
            //   there also negate tag filter with ^@
            //       main_component_name#struct^@disabled = ...
            //   also can be used to add variants with decorations to specified components
            //       #struct@with_body = {token_sealed} $[..]
            //to use selected component from inner components use $[1..]
            //       const_type#type$constable = {token_const} $[..]
            //processing handlers accepts list of components to create functions/classes or variables, each component has assigned data (if applicable)
            //end handlers accepts myself and returns completed code
            //
            //
            //there two modes of loading parser, the header and the body, header is processed first and the body processed only after 'header_end'
            //the header processed a little differently than body
            //  on header there special 'intrinsics' like ignored_chars, delimiting_chars, token, tokens, keep_delimiting_chars, allowed_symbol_chars, namespace_symbol_sequence, enable_namespace, language_name, language_full_name, language_version and dynamic_patching
            //      the ignored_chars, delimiting_chars, allowed_symbol_chars, namespace_symbol_sequence, language_name, token, language_full_name and language_version ones accepts string that processed differently
            //          the first space is ignored and can be omitted
            //          also checked if the string begins with the [ and ends with ], then those braces ignored and string processed as normally
            //          if need to add next space use slash with 'n' like '\n', '\r', '\t', '\e', '\p', '\a', '\b', '\f', '\v' and '\\'
            //          there also custom ones '\B', '\L', '\C', '\N' and '\S'
            //              \B adds symbols in range from A to Z
            //              \L adds symbols in range from a to z
            //              \C adds symbols in range from A to Z and a to z
            //              \N adds symbols in range from 0 to 9
            //              \S adds symbols in range from 0 to 9, a to z and A to Z
            //      the enable_namespace and dynamic_patching only accepts any 'true'(case sensitive) and anything else is processed as 'false'
            //      the keep_delimiting_chars processed as array of strings and must use [] as array and strings must be in scopes []
            //      the tokens processed as array of strings and must use [] as array and strings must be in scopes []
            //      ignored_chars used to ignore symbols during parsing sources
            //      delimiting_chars used to specify delimiting symbols during parsing sources
            //      keep_delimiting_chars used to specify delimiting symbols that will be contentted during parsing sources( like if declared +=, then symbols + and = will be contentted to '+=' but if in source = + then they will not be contentted)
            //      allowed_symbol_chars used to specify allowed symbols in {symbol} component during parsing sources
            //      namespace_symbol_sequence used to specify namespace symbols sequence during parsing sources( if set as :: then symbols will be divided by '::')
            //      enable_namespace used to enable namespace support(if disabled namespace_symbol_sequence will be ignored but {symbol} component will be still declared)
            //      token used to specify token it accepts the string and declares component as token with token_ prefix,
            //              token also supports custom naming and variants for tokens using # 'token#class = [Class]'
            //              adding variant for token by again declaring token with # 'token#class = [struct]'
            //              there also inline token declaration support using # 'token#class' which is same as 'token#class = [class]'
            //              difference between 'decl_cond_token' and 'token' is that 'decl_cond_token' requires registering handlers and 'token' handled automatically
            //      tokens used to specify tokens it accepts the array of strings and declares components as tokens with token_ prefix
            //              tokens also supports inlined token declaration using # 'tokens#class#struct#enum#union' which is same as 'tokens = [[class][struct][enum][union]]'
            //      language_name used to specify language name
            //      language_full_name used to specify language full name
            //      language_version used to specify language version
            //      dynamic_patching used to enable dynamic patching to patch functions and types when sources changed, moved, or removed
            //      after header_end starts language declaration and processed as usual
            //          when header_end used will be declared tokens: symbol and 'ctoken_.....' ones(declared by decl_cond_token#*)
            //              there also enabled tags feature
            class text_language_handler : public language_handler {
                struct token_data {
                    art::ustring token_name;
                    std::string symbol;
                    list_array<std::string> tags; //erased after initialization

                    struct token_chain {
                        struct component : public art::shared_ptr<token_data> {};

                        struct option : public std::vector<art::shared_ptr<token_chain>> {};

                        struct variants : public std::vector<art::shared_ptr<token_chain>> {};

                        struct sequence : public std::vector<art::shared_ptr<token_chain>> {};

                        struct inline_ref : public art::shared_ptr<sequence> {}; //used to process modified chain in the childs

                        std::variant<component, option, variants, sequence, inline_ref> value;

                        component& get_component();
                        option& get_option();
                        variants& get_variants();
                        inline_ref& get_inline_ref();
                        const component& get_component() const;
                        const option& get_option() const;
                        const variants& get_variants() const;
                        const inline_ref& get_inline_ref() const;

                        bool is_component() const;
                        bool is_option() const;
                        bool is_variants() const;
                        bool is_inline_ref() const;
                    };

                    token_chain::inline_ref declaration;
                };

                std::unordered_map<art::ustring, art::shared_ptr<token_data::token_chain>, art::hash<art::ustring>> tokens;
                std::unordered_map<art::ustring, std::function<art::shared_ptr<component_data>(list_array<art::shared_ptr<component_data>>&)>, art::hash<art::ustring>> handlers;
                std::unordered_map<art::ustring, std::function<art::patch_list(art::shared_ptr<component_data>&)>, art::hash<art::ustring>> handlers_end;

                art::ustring language_name;
                art::ustring language_full_name;
                art::ustring language_version;
                std::unordered_set<char> allowed_symbol_chars;
                std::unordered_set<char> ignored_chars;
                std::unordered_set<char> delimiting_chars;
                std::unordered_map<char, list_array<std::string>> keep_delimiting_chars;
                std::string namespace_symbol_sequence; // like c++ '::' or c# ':' or java '.'
                art::TaskRWMutex rw_mutex;
                bool in_header_part = true;
                bool enable_dynamic_patching = true; //by default set true
                bool namespace_enabled = false;

                void address_token(std::string_view raw_name_with_addressing, std::function<void(std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, list_array<std::string>>&)>&& callback);
                art::shared_ptr<token_data::token_chain>& get_token(std::string_view name);

                art::shared_ptr<token_data::token_chain> process_part_component_or_variants(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration);
                art::shared_ptr<token_data::token_chain> process_part_optional(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration);
                art::shared_ptr<token_data::token_chain> process_part_item(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration);
                token_data::token_chain::inline_ref process_part(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view token_declaration);

                void process_token_declaration(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, art::shared_ptr<token_data::token_chain>& token, const std::string& token_name, const std::string& token_declaration, const list_array<std::string>& tags = {});
                void intrinsics_from_token(std::string token_name);
                void declare_token(art::shared_ptr<token_data::token_chain>&, const std::string& token_name, const std::string& token_declaration);
                void declare_conditional_token(art::shared_ptr<token_data::token_chain>&, const std::string& token_name, const std::string& token_declaration);


                std::unordered_map<art::ustring, art::patch_list_added_items, art::hash<art::ustring>> added_patches;

            public:
                text_language_handler(std::string_view language_declaration);
                void register_processing_handler(std::string_view token_name, std::function<art::shared_ptr<component_data>(list_array<art::shared_ptr<component_data>>&)> handler);
                void register_end_handler(std::string_view token_name, std::function<art::patch_list(art::shared_ptr<component_data>&)> handler);

                art::patch_list handle_init(art::files::FileHandle& file) override;
                art::patch_list handle_init_complete() override;
                art::patch_list handle_create(art::files::FileHandle& file) override;
                art::patch_list handle_renamed(const art::ustring& old, art::files::FileHandle& file) override;
                art::patch_list handle_changed(art::files::FileHandle& file) override;
                art::patch_list handle_removed(const art::ustring& removed) override;
            };
        }
    }
}

#endif /* SRC_RUN_TIME_LIBRARY_CXX_LANGUAGE */
