// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_CXX_LIBRARY_COMPILER
#define SRC_RUN_TIME_CXX_LIBRARY_COMPILER
#include <unordered_map>

#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/attacha_abi_structs.hpp>
#include <run_time/library/cxx/files.hpp>
#include <util/hash.hpp>
#include <util/ustring.hpp>

namespace art {
    namespace compiler {
        struct Patch {
            art::FuncHandle::inner_handle* patch_handle = nullptr;
            art::ustring function_symbol;

            void apply();
            void apply_to(const art::shared_ptr<FuncEnvironment>&);
        };

        class PatchList {
            std::list<art::shared_ptr<Patch>> patches;


        public:
            class iterator {
                std::list<art::shared_ptr<Patch>>& patches;
                std::list<art::shared_ptr<Patch>>::iterator _current;
                std::list<art::shared_ptr<Patch>>::iterator _begin;
                std::list<art::shared_ptr<Patch>>::iterator _end;

                enum class ReachedEnd {
                    none,
                    begin,
                    end
                } reached_end = ReachedEnd::none;

                bool on_init = true;

            public:
                iterator(std::list<art::shared_ptr<Patch>>& patches)
                    : patches(patches) {
                    _current = _begin = patches.begin();
                    _end = patches.end();
                }

                iterator begin() {
                    return iterator(patches);
                }

                iterator end() {
                    iterator result(patches);
                    result._current = result._end;
                    return result;
                }

                bool next() {
                    if (reached_end == ReachedEnd::begin)
                        return false;
                    if (reached_end == ReachedEnd::end) {
                        reached_end = ReachedEnd::none;
                        on_init = true;
                    }
                    if (on_init) {
                        on_init = false;
                        return true;
                    }
                    if (_current == _end) {
                        reached_end = ReachedEnd::begin;
                        return false;
                    }
                    ++_current;
                    return true;
                }

                bool prev() {
                    if (reached_end == ReachedEnd::end)
                        return false;
                    if (reached_end == ReachedEnd::begin) {
                        reached_end = ReachedEnd::none;
                        on_init = true;
                    }
                    if (_current == _begin) {
                        reached_end = ReachedEnd::end;
                        return false;
                    }
                    --_current;
                    return true;
                }

                art::shared_ptr<Patch> get() {
                    if (reached_end == ReachedEnd::none)
                        return *_current;
                    return nullptr;
                }

                void set(const art::shared_ptr<Patch>& patch) {
                    if (reached_end == ReachedEnd::none)
                        *_current = patch;
                }
            };

            void add_patches(patch_list&&);
            void add_patches(PatchList&&);

            void add_patch(Patch&&);

            void remove_patch(const art::ustring& name);
            art::shared_ptr<Patch> take_patch(const art::ustring& name);

            void apply();

            static iterator iterate(const art::shared_ptr<PatchList>&);
        };

        class Compiler {
        public:
            virtual ~Compiler() noexcept(false) = default;
            virtual void decode(const art::ustring&) = 0; //path to file
            virtual void decode(files::FileHandle&) = 0;
            virtual void decode(files::BlockingFileHandle&) = 0;

            virtual std::vector<std::vector<uint8_t>> opcodes() = 0;

            //initializes new compiler with the same state
            virtual art::shared_ptr<Compiler> clone() = 0;

            //implemented, call opcodes() to get the opcodes
            PatchList extract_patches();
            //implemented, call opcodes() to get the opcodes
            void apply_patches();
            void save_patches(const art::ustring& path);
            void save_patches(files::FileHandle&);
            void save_patches(files::BlockingFileHandle&);
        };

        class TextCompiler : public Compiler {
        public:
            virtual void decode_text(const art::ustring&) = 0;
        };

        class BinaryCompiler : public Compiler {
        public:
            virtual void decode_bin(const std::vector<uint8_t>&) = 0;
        };

        class PrecompiledCompiler : public BinaryCompiler {
            std::vector<std::vector<uint8_t>> compiled;

        public:
            void decode(const art::ustring&) override;
            void decode(files::FileHandle&) override;
            void decode(files::BlockingFileHandle&) override;
            void decode_bin(const std::vector<uint8_t>&) override;

            std::vector<std::vector<uint8_t>> opcodes() override;
            art::shared_ptr<Compiler> clone() override;
        };

        class UniversalCompiler : public Compiler {
            TaskMutex mutex;
            std::unordered_map<art::ustring, art::shared_ptr<Compiler>, art::hash<art::ustring>> format_compiler;

            Compiler* selectCompiler(const art::ustring& path) const;

        public:
            UniversalCompiler();
            UniversalCompiler(const UniversalCompiler&);
            UniversalCompiler(UniversalCompiler&&);

            void decode(const art::ustring& path) override;
            void decode(files::FileHandle&) override;
            void decode(files::BlockingFileHandle&) override;
            void decode(files::FileHandle&, const art::ustring& format);
            void decode(files::BlockingFileHandle&, const art::ustring& format);

            std::vector<std::vector<uint8_t>> opcodes() override;

            art::shared_ptr<Compiler> clone() override;

            void apply_patches();

            void register_compiler(const art::ustring& format, const art::shared_ptr<Compiler>& compiler);
            void unregister_compiler(const art::ustring& format);
        };
    }
}


#endif /* SRC_RUN_TIME_CXX_LIBRARY_COMPILER */
