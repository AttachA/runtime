// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/cxx_library/compiler.hpp>
#include <run_time/library/bytes.hpp>

namespace art {
    namespace compiler {
        void Patch::apply() {
            FuncEnvironment::fastHotPatch(function_symbol, patch_handle);
        }

        void Patch::apply_to(const art::shared_ptr<FuncEnvironment>& func_env) {
            func_env->patch(patch_handle);
        }

        void PatchList::add_patches(patch_list&& patches_list) {
            for (const auto& patch : patches_list)
                patches.push_back(new Patch{patch.second, patch.first});
        }

        void PatchList::add_patches(PatchList&& patches_list) {
            patches.insert(patches.end(), patches_list.patches.begin(), patches_list.patches.end());
        }

        void PatchList::add_patch(Patch&& patch) {
            patches.emplace_back(new Patch(std::move(patch)));
        }

        void PatchList::remove_patch(const art::ustring& name) {
            auto it = std::find_if(patches.begin(), patches.end(), [&](const art::shared_ptr<Patch>& patch) {
                return patch->function_symbol == name;
            });
            if (it == patches.end())
                return;
            patches.erase(it);
        }

        art::shared_ptr<Patch> PatchList::take_patch(const art::ustring& name) {
            auto it = std::find_if(patches.begin(), patches.end(), [&](const art::shared_ptr<Patch>& patch) {
                return patch->function_symbol == name;
            });
            if (it == patches.end())
                return nullptr;
            auto result = *it;
            patches.erase(it);
            return result;
        }

        void PatchList::apply() {
            for (const auto& patch : patches)
                patch->apply();
        }

        PatchList::iterator PatchList::iterate(const art::shared_ptr<PatchList>& patches_list) {
            return iterator(patches_list->patches);
        }

        PatchList Compiler::extract_patches() {
            PatchList result;
            auto _opcodes = opcodes();
            for (const auto& opcode : _opcodes) {
                auto _patch = new FuncHandle::inner_handle(opcode, false, nullptr);
                result.add_patch({_patch, art::ustring()});
            }
            return result;
        }

        void Compiler::apply_patches() {
            auto _opcodes = opcodes();
            patch_list _patch_list;
            for (const auto& opcode : _opcodes) {
                auto _patch = new FuncHandle::inner_handle(opcode, false, nullptr);
                _patch_list.push_back({art::ustring(), _patch});
            }

            PatchList patches;
            patches.add_patches(std::move(_patch_list));
            patches.apply();
        }

        void Compiler::save_patches(const art::ustring& path) {
            art::files::_sync_flags flags;
            flags.value = 0;

            files::BlockingFileHandle file(path.c_str(), path.size(), art::files::open_mode::write, art::files::on_open_action::always_new, flags);
            save_patches(file);
        }

        void Compiler::save_patches(files::FileHandle& file) {
            auto _opcodes = opcodes();
            uint64_t _opcodes_size = bytes::convert_endian<bytes::Endian::little>(_opcodes.size());
            file.write((uint8_t*)&_opcodes_size, sizeof(uint64_t)).getAsync();
            for (const auto& opcode : _opcodes) {
                uint64_t _opcode_size = bytes::convert_endian<bytes::Endian::little>(opcode.size());
                file.write((uint8_t*)&_opcode_size, sizeof(uint64_t)).getAsync();
                file.write(opcode.data(), opcode.size()).getAsync();
            }
        }

        void Compiler::save_patches(files::BlockingFileHandle& file) {
            auto _opcodes = opcodes();
            uint64_t _opcodes_size = bytes::convert_endian<bytes::Endian::little>(_opcodes.size());
            file.write((uint8_t*)&_opcodes_size, sizeof(uint64_t));
            for (const auto& opcode : _opcodes) {
                uint64_t _opcode_size = bytes::convert_endian<bytes::Endian::little>(opcode.size());
                file.write((uint8_t*)&_opcode_size, sizeof(uint64_t));
                file.write(opcode.data(), opcode.size());
            }
        }

        void PrecompiledCompiler::decode(const art::ustring& path) {
            files::BlockingFileHandle f(path.c_str(), path.size(), art::files::open_mode::read, art::files::on_open_action::always_new, art::files::_sync_flags());
            decode(f);
        }

        void PrecompiledCompiler::decode(files::FileHandle& file) {
            uint64_t _opcodes_size;
            file.read_fixed((uint8_t*)&_opcodes_size, (uint32_t)sizeof(uint64_t));
            _opcodes_size = bytes::convert_endian<bytes::Endian::little>(_opcodes_size);
            for (uint64_t i = 0; i < _opcodes_size; i++) {
                uint64_t _opcode_size;
                file.read_fixed((uint8_t*)&_opcode_size, sizeof(uint64_t));
                _opcode_size = bytes::convert_endian<bytes::Endian::little>(_opcode_size);
                std::vector<uint8_t> _opcode(_opcode_size);
                file.read_fixed(_opcode.data(), _opcode_size);
                decode_bin(_opcode);
            }
        }

        void PrecompiledCompiler::decode(files::BlockingFileHandle& file) {
            uint64_t _opcodes_size;
            file.read((uint8_t*)&_opcodes_size, sizeof(uint64_t));
            _opcodes_size = bytes::convert_endian<bytes::Endian::little>(_opcodes_size);
            for (uint64_t i = 0; i < _opcodes_size; i++) {
                uint64_t _opcode_size;
                file.read((uint8_t*)&_opcode_size, sizeof(uint64_t));
                _opcode_size = bytes::convert_endian<bytes::Endian::little>(_opcode_size);
                std::vector<uint8_t> _opcode(_opcode_size);
                file.read(_opcode.data(), _opcode_size);
                decode_bin(_opcode);
            }
        }

        void PrecompiledCompiler::decode_bin(const std::vector<uint8_t>& _opcodes) {
            compiled.push_back(_opcodes);
        }

        std::vector<std::vector<uint8_t>> PrecompiledCompiler::opcodes() {
            return compiled;
        }

        art::shared_ptr<Compiler> PrecompiledCompiler::clone() {
            return new PrecompiledCompiler(*this);
        }

        UniversalCompiler::UniversalCompiler() {
            art::shared_ptr<Compiler> pre_compiled = new PrecompiledCompiler();
            register_compiler("precompiled", pre_compiled);
            register_compiler("art_precompiled", pre_compiled);
            register_compiler("art_pc", pre_compiled);
            register_compiler("art", pre_compiled);
        }

        UniversalCompiler::UniversalCompiler(const UniversalCompiler& copy) {
            for (const auto& compiler : copy.format_compiler)
                format_compiler.emplace(compiler.first, compiler.second->clone());
        }

        UniversalCompiler::UniversalCompiler(UniversalCompiler&& move) {
            format_compiler = std::move(move.format_compiler);
        }

        Compiler* UniversalCompiler::selectCompiler(const art::ustring& path) const {
            files::FolderBrowser browser(path.c_str(), path.size());
            if (browser.is_file()) {
                auto format = (art::ustring)browser.file_extension();
                auto compiler_item = format_compiler.find(format);
                if (compiler_item == format_compiler.end())
                    throw std::runtime_error("Compiler for format " + format + " not registered");

                auto compiler = compiler_item->second;
                compiler->decode(path);
            } else if (browser.is_folder()) {
                throw std::runtime_error("Folder decoding not implemented");
            }
        }

        void UniversalCompiler::decode(const art::ustring& path) {
            art::lock_guard<TaskMutex> lock(mutex);
            selectCompiler(path)->decode(path);
        }

        void UniversalCompiler::decode(files::FileHandle& file) {
            art::lock_guard<TaskMutex> lock(mutex);
            selectCompiler(file.get_path())->decode(file);
        }

        void UniversalCompiler::decode(files::BlockingFileHandle& file) {
            art::lock_guard<TaskMutex> lock(mutex);
            selectCompiler(file.get_path())->decode(file);
        }

        void UniversalCompiler::decode(files::FileHandle& file, const art::ustring& format) {
            art::lock_guard<TaskMutex> lock(mutex);
            auto compiler_item = format_compiler.find(format);
            if (compiler_item == format_compiler.end())
                throw std::runtime_error("Compiler for format " + format + " not registered");
            compiler_item->second->decode(file);
        }

        void UniversalCompiler::decode(files::BlockingFileHandle& file, const art::ustring& format) {
            art::lock_guard<TaskMutex> lock(mutex);
            auto compiler_item = format_compiler.find(format);
            if (compiler_item == format_compiler.end())
                throw std::runtime_error("Compiler for format " + format + " not registered");
            compiler_item->second->decode(file);
        }

        void UniversalCompiler::apply_patches() {
            art::lock_guard<TaskMutex> lock(mutex);
            for (const auto& compiler : format_compiler)
                compiler.second->apply_patches();
        }

        art::shared_ptr<Compiler> UniversalCompiler::clone() {
            return new UniversalCompiler(*this);
        }

        std::vector<std::vector<uint8_t>> UniversalCompiler::opcodes() {
            art::lock_guard<TaskMutex> lock(mutex);
            std::vector<std::vector<uint8_t>> result;
            for (const auto& compiler : format_compiler) {
                auto _opcodes = compiler.second->opcodes();
                result.insert(result.end(), _opcodes.begin(), _opcodes.end());
            }
            return result;
        }

        void UniversalCompiler::register_compiler(const art::ustring& format, const art::shared_ptr<Compiler>& compiler) {
            art::lock_guard<TaskMutex> lock(mutex);
            if (format_compiler.emplace(format, compiler).second == false)
                throw std::runtime_error("Compiler for format " + format + " already registered");
        }

        void UniversalCompiler::unregister_compiler(const art::ustring& format) {
            art::lock_guard<TaskMutex> lock(mutex);
            if (format_compiler.erase(format) == 0)
                throw std::runtime_error("Compiler for format " + format + " not registered");
        }
    }
}
