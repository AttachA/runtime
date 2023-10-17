#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/localization.hpp>

#if PLATFORM_WINDOWS
    #include <Windows.h>
#elif PLATFORM_LINUX
    #include <locale.h>
#endif

namespace art {
    namespace localization {
        AttachAFun(get_language, 0, {
            return _internal_::get_language();
        });

        AttachAFun(set_language, 1, {
            return _internal_::set_language((art::ustring)args[0]);
        });

        AttachAFun(force_set_language, 1, {
            _internal_::force_set_language((art::ustring)args[0]);
        });

        AttachAFun(get_default_language, 0, {
            return _internal_::get_default_language();
        });

        AttachAFun(set_default_language, 1, {
            return _internal_::set_default_language((art::ustring)args[0]);
        });

        AttachAFun(use_local_language, 0, {
            return _internal_::use_local_language();
        });

        AttachAFun(get_languages_list, 0, {
            return _internal_::get_languages_list().convert<ValueItem>([](const art::ustring& language) { return language; });
        });

        AttachAFun(get_localized_string, 1, {
            if (len == 1)
                return _internal_::get_localized_string((art::ustring)args[0]);
            else
                return _internal_::get_localized_string((art::ustring)args[0], (art::ustring)args[1]);
        });

        AttachAFun(set_localized_string, 3, {
            _internal_::set_localized_string((art::ustring)args[0], (art::ustring)args[1], (art::ustring)args[2]);
        });

        AttachAFun(update_localization_strings, 2, {
            CXX::excepted_array(args[1]);
            CXX::array_size_min(args[1].size(), 1);
            CXX::excepted_array(args[1][0]);
            CXX::array_size_min(args[1][0].size(), 2);

            list_array<_internal_::localization_pair> localization_strings;
            for (auto& localization_string : args[1])
                localization_strings.push_back(_internal_::localization_pair((art::ustring)localization_string[0], (art::ustring)localization_string[1]));


            _internal_::update_localization_strings((art::ustring)args[0], localization_strings);
        });

        AttachAFun(remove_localized_string, 2, {
            _internal_::remove_localized_string((art::ustring)args[0], (art::ustring)args[1]);
        });

        AttachAFun(remove_localization_strings, 2, {
            CXX::excepted_array(args[1]);
            CXX::array_size_min(args[1].size(), 1);

            list_array<art::ustring> localization_keys;
            for (auto& localization_key : args[1])
                localization_keys.push_back((art::ustring)localization_key);

            _internal_::remove_localization_strings((art::ustring)args[0], localization_keys);
        });

        AttachAFun(get_current_locale_changed, 0, {
            return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _internal_::current_locale_changed), no_copy);
        });

        AttachAFun(get_current_locale_updated, 0, {
            return ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>((AttachAVirtualTable*)CXX::Interface::typeVTable<typed_lgr<EventSystem>>(), _internal_::current_locale_updated), no_copy);
        });

        namespace _internal_ {
            art::ustring default_language = "en";
            art::ustring current_language = "en";
            std::unordered_map<art::ustring, std::unordered_map<art::ustring, art::ustring, art::hash<art::ustring>>, art::hash<art::ustring>> localization_strings;

            art::ustring get_language() {
                return current_language;
            }

            bool set_language(const art::ustring& language) {
                if (localization_strings.find(language) == localization_strings.end())
                    return false;

                art::ustring old = current_language;
                current_language = language;
                ValueItem args = language;
                if (!current_locale_changed->await_notify(args)) {
                    current_language = old;
                    args = old;
                    current_locale_changed->async_notify(args);
                    return false;
                }
                return true;
            }

            void force_set_language(const art::ustring& language) {
                current_language = language;
                ValueItem args = language;
                current_locale_changed->async_notify(args);
            }

            art::ustring get_default_language() {
                return default_language;
            }

            bool set_default_language(const art::ustring& language) {
                if (localization_strings.find(language) == localization_strings.end())
                    return false;

                default_language = language;
                return true;
            }

            bool use_local_language() {
#if PLATFORM_WINDOWS
                wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
                if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH) == 0)
                    return false;
#elif PLATFORM_LINUX
                char* locale_name = setlocale(LC_ALL, "");
                if (locale_name == nullptr)
                    return false;
#endif
                return set_language(locale_name);
            }

            list_array<art::ustring> get_languages_list() {
                list_array<art::ustring> languages;
                for (auto& language : localization_strings)
                    languages.push_back(language.first);
                return languages;
            }

            art::ustring get_localized_string(const art::ustring& localization_language, const art::ustring& localization_key) {
                if (localization_strings.find(localization_language) == localization_strings.end())
                    return localization_key;
                auto& current_localization_strings = localization_strings[localization_language];
                if (current_localization_strings.find(localization_key) == current_localization_strings.end())
                    return localization_key;
                return current_localization_strings[localization_key];
            }

            art::ustring get_localized_string(const art::ustring& localization_key) {
                auto& current_localization_strings = localization_strings[current_language];
                if (current_localization_strings.find(localization_key) == current_localization_strings.end())
                    return localization_key;
                return current_localization_strings[localization_key];
            }

            void set_localized_string(const art::ustring& localization_language, const art::ustring& localization_key, const art::ustring& localization_value) {
                localization_strings[localization_language][localization_key] = localization_value;
                if (localization_language == current_language) {
                    ValueItem args = localization_key;
                    current_locale_updated->async_notify(args);
                }
            }

            void update_localization_strings(const art::ustring& localization_language, const list_array<localization_pair>& localization_strings) {
                for (auto& localization_string : localization_strings)
                    set_localized_string(localization_language, localization_string.key, localization_string.value);
            }

            void remove_localized_string(const art::ustring& localization_language, const art::ustring& localization_key) {
                localization_strings[localization_language].erase(localization_key);
                if (localization_language == current_language) {
                    ValueItem args = localization_key;
                    current_locale_updated->async_notify(args);
                }
            }

            void remove_localization_strings(const art::ustring& localization_language, const list_array<art::ustring>& localization_keys) {
                for (auto& localization_key : localization_keys)
                    remove_localized_string(localization_language, localization_key);
            }

            typed_lgr<EventSystem> current_locale_changed = new EventSystem();
            typed_lgr<EventSystem> current_locale_updated = new EventSystem();
        }
    }
}
