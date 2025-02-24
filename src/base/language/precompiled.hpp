// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_BASE_LANGUAGE_PRECOMPILED
#define SRC_BASE_LANGUAGE_PRECOMPILED
#include <run_time/library/cxx/language.hpp>

namespace language_parsers {

    class precompiled : public art::language::language_handler {
        //{path : { function: hash }}...

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
        art::TaskMutex mutex;

    public:
        std::string_view get_language_extension() const override {
            return "art";
        }

        art::patch_list handle_init(art::files::FileHandle& file) override;
        art::patch_list handle_init_complete() override;
        art::patch_list handle_create(art::files::FileHandle& file) override;
        art::patch_list handle_renamed(const art::ustring& old, art::files::FileHandle& file) override;
        art::patch_list handle_changed(art::files::FileHandle& file) override;
        art::patch_list handle_removed(const art::ustring& removed) override;
    };
}

#endif /* SRC_BASE_LANGUAGE_PRECOMPILED */
