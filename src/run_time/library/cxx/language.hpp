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
            virtual std::string_view get_language_extension() const = 0; //returns default file extension
            virtual art::patch_list handle_init(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_init_complete() = 0;
            virtual art::patch_list handle_create(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_renamed(const art::ustring& old, art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_changed(art::files::FileHandle& file) = 0;
            virtual art::patch_list handle_removed(const art::ustring& removed) = 0;
        };

        class language_provider {
            struct handle__ {
                std::unordered_map<art::ustring, art::shared_ptr<language_handler>, art::hash<art::ustring>> languages;
                art::ValueItem folder_monitor;
                bool init_mode = true;
                art::TaskRWMutex rw_mutex;
                art::patch_list patches;
            };

            art::shared_ptr<handle__> h = new handle__();

        public:
            language_provider(std::string_view path, bool include_sub_directories);
            ~language_provider();
            void register_language(art::shared_ptr<language_handler> decoder);
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
                std::string_view token_name;
                art::ValueItem value; // processing result
                component args;
                size_t line, column;
                size_t line_end, column_end;
            };

            //symbol is instritic created after header_end
            //value_double, value_long, value_string, value_char is instritic from parser and created only after configuring by `value_processing#***the name***`
            //  list of supported value_processing configs
            //      *string_escape = **token**"
            //          also could be used as intrinsics as alternative to "#string_escape = @[\]"
            //          by default set to '\'
            //          processes value as string, but uses only first symbol
            //          one token to process escape sequence like \" \\ e.t.c...
            //      *decimal_dot = **token**"
            //          also could be used as intrinsics as alternative to "#decimal_dot = @[.]"
            //          one token to process escape sequence like \" \\ e.t.c...
            //          also enables value_double instritic
            //      *string_scope = **token** $[..] **token**
            //          also could be used as intrinsics as alternative to #string_scope = @["""] $[..] @["""]
            //      *string_line = **token** $[..] **token**
            //          also could be used as intrinsics as alternative to #string_line = @["] $[..] @["]
            //      *char = **token** $[..] **token**
            //          also could be used as intrinsics as alternative to #char = @['] $[..] @[']
            //      *hex_number_enable      //flag to add hex style integer handler
            //      *octal_number_enable    //flag to add octal style integer handler
            //      *binary_number_enable   //flag to add binary style integer handler
            //      *disable_long           //flag to remove all integer handlers (does not disables value_double if decimal_dot it already used)
            //
            //compound_token#* declares ctoken_* it used to create tokens that processed by handlers
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
            //use @[...] if needed to use keywords that could be processed ony in this token, works only in handler part
            //   it may be used to create keywords that in other cases could be decoded to symbol like @[file]
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
            //              difference between 'compound_token' and 'token' is that 'compound_token' requires registering handlers and 'token' handled automatically
            //      tokens used to specify tokens it accepts the array of strings and declares components as tokens with token_ prefix
            //              tokens also supports inlined token declaration using # 'tokens#class#struct#enum#union' which is same as 'tokens = [[class][struct][enum][union]]'
            //      language_name used to specify language name
            //      language_full_name used to specify language full name
            //      language_version used to specify language version
            //      dynamic_patching used to enable dynamic patching to patch functions and types when sources changed, moved, or removed
            //      after header_end starts language declaration and processed as usual
            //          when header_end used will be declared tokens: symbol and 'ctoken_.....' ones(declared by compound_token#*)
            //              there also enabled tags feature
            class text_language_handler : public language_handler {

                struct ast_data {
                    struct process_string_scope {};

                    struct process_string_line {};

                    struct process_char {};

                    struct process_hex_num {};

                    struct process_octal_num {};

                    struct process_binary_num {};

                    struct inline_symbol {};

                    class opt_name {
                        std::string* name = nullptr;

                    public:
                        opt_name() {}

                        opt_name(std::string name)
                            : name(new std::string(name)) {}

                        ~opt_name() {
                            if (name)
                                delete name;
                        }

                        std::string_view get() const {
                            if (name)
                                return *name;
                            else
                                return "";
                        }
                    } name;

                    struct proc_rul {
                        union {
                            char c;
                            std::string* s;
                            ast_data** vars; //array(variants)
                        };
                        enum class var : uint64_t {
                            var_c,
                            var_s,
                            var_ps,
                            var_pl,
                            var_pc,
                            var_ph,
                            var_po,
                            var_pb,
                            var_vars,
                            var_symbol,
                        } v : 4;
                        uint64_t is_optional : 1 = false;
                        uint64_t is_sequence : 1 = false;
                        uint64_t arr_siz : 58;

                        proc_rul() {
                            v = var::var_c;
                            c = '\0';
                        }

                        proc_rul(char c)
                            : v(var::var_c), c(c) {}

                        proc_rul(std::string s)
                            : v(var::var_s), s(new std::string(std::move(s))) {}

                        proc_rul(process_string_scope)
                            : v(var::var_ps) {}

                        proc_rul(process_string_line)
                            : v(var::var_pl) {}

                        proc_rul(process_char)
                            : v(var::var_pc) {}

                        proc_rul(process_hex_num)
                            : v(var::var_ph) {}

                        proc_rul(process_octal_num)
                            : v(var::var_po) {}

                        proc_rul(process_binary_num)
                            : v(var::var_pb) {}

                        proc_rul(inline_symbol)
                            : v(var::var_symbol) {}

                        proc_rul(const std::vector<ast_data*>& __vars);

                        ~proc_rul();
                    } cmd;
                };

                struct token_data {
                    art::ustring token_name;
                    std::string symbol;           //if this field specified, declaration ignored
                    list_array<std::string> tags; //erased after initialization

                    struct token_chain {
                        struct component : public art::shared_ptr<token_data> {};

                        struct option : public std::vector<art::shared_ptr<token_chain>> {};

                        struct variants : public std::vector<art::shared_ptr<token_chain>> {};

                        struct sequence : public std::vector<art::shared_ptr<token_chain>> {};

                        struct inline_ref : public art::shared_ptr<sequence> {}; //used to process modified chain in the childs

                        struct inline_decl {
                            std::string symbol;
                        };

                        struct process_string_scope {};

                        struct process_string_line {};

                        struct process_char {};

                        struct process_hex_num {};

                        struct process_octal_num {};

                        struct process_binary_num {};

                        struct inline_symbol {};

                        std::variant<
                            component,
                            option,
                            variants,
                            sequence,
                            inline_ref,
                            inline_decl,
                            process_string_scope,
                            process_string_line,
                            process_char,
                            process_hex_num,
                            process_octal_num,
                            process_binary_num,
                            inline_symbol>
                            value;

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
                    ast_data* assigned_ast = nullptr;
                    bool entry_token = false;
                };

                template <class... Args>
                static token_data::token_chain::sequence make_sequence_chain(Args&&... tokens) {
                    token_data::token_chain::sequence seq;
                    (seq.push_back(new token_data::token_chain{std::forward<Args>(tokens)}), ...);
                    return seq;
                }

                template <class... Args>
                static token_data::token_chain::inline_ref make_sequence_chain_ref(Args&&... tokens) {
                    return token_data::token_chain::inline_ref(new token_data::token_chain::sequence(make_sequence_chain(std::forward<Args>(tokens)...)));
                }


                std::unordered_map<art::ustring, art::shared_ptr<token_data::token_chain>, art::hash<art::ustring>> tokens;
                std::unordered_map<art::ustring, std::function<void(component_data&)>, art::hash<art::ustring>> handlers;
                std::unordered_map<art::ustring, std::function<art::patch_list(component_data&)>, art::hash<art::ustring>> handlers_end;


                list_array<ast_data> ast;
                std::vector<ast_data*> entry_ast;
                art::ustring language_name;
                art::ustring language_full_name;
                art::ustring language_version;
                std::unordered_set<char> allowed_symbol_chars;
                std::unordered_set<char> ignored_chars;
                std::unordered_set<char> delimiting_chars;
                std::unordered_map<char, list_array<std::string>> keep_delimiting_chars;
                std::string namespace_symbol_sequence; // like c++ '::' or c# ':' or java '.'
                art::TaskRWMutex rw_mutex;
                char string_escape = '\\';
                bool in_header_part = true;
                bool enable_dynamic_patching = true; //by default set true
                bool namespace_enabled = false;


                void address_token(std::string_view raw_name_with_addressing, std::function<void(std::tuple<std::vector<art::shared_ptr<token_data::token_chain>>, art::shared_ptr<token_data::token_chain>, const list_array<std::string>&, const std::string&>&)>&& callback);
                art::shared_ptr<token_data::token_chain>& get_token(std::string_view name);

                art::shared_ptr<token_data::token_chain> process_part_component(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration);
                art::shared_ptr<token_data::token_chain> process_part_item(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view& token_declaration);
                token_data::token_chain::inline_ref process_part(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, std::string_view token_declaration);

                void process_token_declaration(std::vector<art::shared_ptr<token_data::token_chain>>& selected_tokens, art::shared_ptr<token_data::token_chain>& token, const std::string& token_name, const std::string& token_declaration, const list_array<std::string>& tags = {});
                void intrinsics_from_token(std::string token_name);
                void declare_token(const std::string& token_name, const std::string& token_declaration);
                void declare_compound_token(const std::string& token_name, const std::string& token_declaration);
                void configure_value_processing(const std::string& config_name, const std::string& token_declaration);

                void declaration_complete();
                ast_data* recursive_declaration_complete(std::unordered_map<void*, ast_data*>& visited, art::shared_ptr<token_data::token_chain>& token);

                std::unordered_map<
                    art::ustring,
                    std::unordered_map<
                        art::ustring,
                        uint64_t,
                        art::hash<art::ustring>>,
                    art::hash<art::ustring>>
                    declared_functions;
                std::unordered_map<
                    art::ustring,
                    std::unordered_map<
                        list_array<art::ustring>,
                        uint64_t,
                        art::hash<list_array<art::ustring>>>,
                    art::hash<art::ustring>>
                    declared_types;

                art::patch_list parse_file(art::files::FileHandle& file);

            protected:
                void register_processing_handler(std::string_view token_name, std::function<void(component_data&)> handler);
                void register_end_handler(std::string_view token_name, std::function<art::patch_list(component_data&)> handler);

            public:
                text_language_handler(std::string_view language_declaration);

                std::string_view get_language_extension() const override {
                    return (std::string_view)language_name;
                }

                std::string_view get_language_name() const {
                    return (std::string_view)language_name;
                }

                std::string_view get_language_full_name() const {
                    return (std::string_view)language_full_name;
                }

                std::string_view get_language_version() const {
                    return (std::string_view)language_version;
                }

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
