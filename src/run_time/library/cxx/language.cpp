#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/cxx/files.hpp>
#include <run_time/library/cxx/language.hpp>
#include <run_time/library/file.hpp>

namespace art {
    using namespace CXX;

    namespace language {
        language_provider::language_provider(std::string_view path, bool include_sub_directories) {
            folder_monitor = cxxCall(file::constructor::createProxy_FolderChangesMonitor, path, include_sub_directories);
            Interface::makeCall(
                ClassAccess::pub,
                Interface::makeCall(ClassAccess::pub, folder_monitor, "get_event_file_creation"),
                "join",
                MakeNative([&](const ustring& name) {
                    files::FolderBrowser browser(name.c_str(), name.size());
                    ustring extension = (ustring)browser.file_extension();
                    art::unique_lock unify(rw_mutex);
                    auto it = languages.find(extension);
                    if (it != languages.end()) {
                        files::FileHandle handle(name.c_str(), name.size(), files::open_mode::read, files::on_open_action::open_exists, files::_async_flags{});
                        if (init_mode)
                            patches.add_patches(it->second->handle_init(handle));
                        else {
                            patches.add_patches(it->second->handle_create(handle));
                            patches.apply();
                        }
                    }
                })
            );
            Interface::makeCall(
                ClassAccess::pub,
                Interface::makeCall(ClassAccess::pub, folder_monitor, "get_event_file_name_change"),
                "join",
                MakeNative([&](const ustring& old_name, const ustring& new_name) {
                    files::FolderBrowser browser(new_name.c_str(), new_name.size());
                    ustring extension = (ustring)browser.file_extension();
                    art::unique_lock unify(rw_mutex);
                    auto it = languages.find(extension);
                    if (it != languages.end()) {
                        files::FileHandle handle(new_name.c_str(), new_name.size(), files::open_mode::read, files::on_open_action::open_exists, files::_async_flags{});
                        patches.add_patches(it->second->handle_renamed(old_name, handle));
                        if (!init_mode)
                            patches.apply();
                    }
                })
            );
            Interface::makeCall(
                ClassAccess::pub,
                Interface::makeCall(ClassAccess::pub, folder_monitor, "get_event_file_last_write"),
                "join",
                MakeNative([&](const ustring& name) {
                    files::FolderBrowser browser(name.c_str(), name.size());
                    ustring extension = (ustring)browser.file_extension();
                    art::unique_lock unify(rw_mutex);
                    auto it = languages.find(extension);
                    if (it != languages.end()) {
                        files::FileHandle handle(name.c_str(), name.size(), files::open_mode::read, files::on_open_action::open_exists, files::_async_flags{});
                        patches.add_patches(it->second->handle_changed(handle));
                        if (!init_mode)
                            patches.apply();
                    }
                })
            );
            Interface::makeCall(
                ClassAccess::pub,
                Interface::makeCall(ClassAccess::pub, folder_monitor, "get_event_file_removed"),
                "join",
                MakeNative([&](const ustring& name) {
                    files::FolderBrowser browser(name.c_str(), name.size());
                    ustring extension = (ustring)browser.file_extension();
                    art::unique_lock unify(rw_mutex);
                    auto it = languages.find(extension);
                    if (it != languages.end()) {
                        patches.add_patches(it->second->handle_removed(name));
                        if (!init_mode)
                            patches.apply();
                    }
                })
            );
        }

        void language_provider::register_language(std::string_view name, art::shared_ptr<language_handler> decoder) {
            art::unique_lock unify(rw_mutex);
            languages[ustring(name.data(), name.size())] = decoder;
        }

        void language_provider::unregister_language(std::string_view name) {
            art::unique_lock unify(rw_mutex);
            languages.erase(ustring(name.data(), name.size()));
        }

        void language_provider::run_once() {
            Interface::makeCall(ClassAccess::pub, folder_monitor, "once_scan");
            art::unique_lock unify(rw_mutex);
            if (init_mode) {
                for (auto& [name, decoder] : languages)
                    patches.add_patches(decoder->handle_init_complete());
                patches.apply();
            }
            init_mode = false;
        }

        void language_provider::start() {
            run_once();
            Interface::makeCall(ClassAccess::pub, folder_monitor, "start");
        }

        void language_provider::stop() {
            Interface::makeCall(ClassAccess::pub, folder_monitor, "stop");
        }
    }
}