#ifndef SRC_RUN_TIME_LIBRARY_LOCALIZATION
#define SRC_RUN_TIME_LIBRARY_LOCALIZATION
#include <run_time/tasks.hpp>
#include <util/ustring.hpp>

namespace art {
    namespace localization {
        ValueItem* get_language(ValueItem*, uint32_t);
        ValueItem* set_language(ValueItem*, uint32_t);
        ValueItem* force_set_language(ValueItem*, uint32_t);

        ValueItem* get_default_language(ValueItem*, uint32_t);
        ValueItem* set_default_language(ValueItem*, uint32_t);

        ValueItem* use_local_language(ValueItem*, uint32_t);
        ValueItem* get_languages_list(ValueItem*, uint32_t);


        ValueItem* get_localized_string(ValueItem*, uint32_t);
        ValueItem* set_localized_string(ValueItem*, uint32_t);
        ValueItem* update_localization_strings(ValueItem*, uint32_t);
        ValueItem* remove_localized_string(ValueItem*, uint32_t);
        ValueItem* remove_localization_strings(ValueItem*, uint32_t);

        ValueItem* get_current_locale_changed(ValueItem*, uint32_t);
        ValueItem* get_current_locale_updated(ValueItem*, uint32_t);

        namespace _internal_ {
            struct localization_pair {
                art::ustring key;
                art::ustring value;
            };

            art::ustring get_language();
            bool set_language(const art::ustring& language);
            void force_set_language(const art::ustring& language);
            art::ustring get_default_language();
            bool set_default_language(const art::ustring& language);
            bool use_local_language();
            list_array<art::ustring> get_languages_list();


            art::ustring get_localized_string(const art::ustring& localization_key);
            art::ustring get_localized_string(const art::ustring& localization_language, const art::ustring& localization_key);
            void set_localized_string(const art::ustring& localization_language, const art::ustring& localization_key, const art::ustring& localization_value);
            void update_localization_strings(const art::ustring& localization_language, const list_array<localization_pair>& localization_strings);
            void remove_localized_string(const art::ustring& localization_language, const art::ustring& localization_key);
            void remove_localization_strings(const art::ustring& localization_language, const list_array<art::ustring>& localization_keys);

            extern typed_lgr<EventSystem> current_locale_changed;
            extern typed_lgr<EventSystem> current_locale_updated;
        }
    }
}

#endif /* SRC_RUN_TIME_LIBRARY_LOCALIZATION */
