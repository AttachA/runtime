// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/attacha_environment.hpp>
#include <run_time/tasks.hpp>

namespace art {
    attacha_environment attacha_environment::self;

    attacha_environment::function_globals_handle& attacha_environment::get_function_globals() {
        return self.function_globals;
    }

    typed_lgr<values_global> attacha_environment::get_value_globals() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (!self._value_global)
            self._value_global = new values_global();
        return self._value_global;
    }

    typed_lgr<types_global> attacha_environment::get_types_global() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (!self._types_global)
            self._types_global = new types_global();
        return self._types_global;
    }

    attacha_environment::code_gen_handle& attacha_environment::get_code_gen() {
        return self.code_gen;
    }

    ValueItem* attacha_environment::find_global_value(const art::ustring& str) {
        return get_value_globals()->find_value(str);
    }

    ValueItem* attacha_environment::find_global_value_local(const art::ustring& str) {
        return get_value_globals()->find_value_local(str);
    }

    ValueItem* attacha_environment::find_global_value_auto_join(const art::ustring& str, const art::ustring& separator) {
        return values_global::find_auto_join(get_value_globals(), str, separator);
    }

    ValueItem* attacha_environment::find_global_value_local_auto_join(const art::ustring& str, const art::ustring& separator) {
        return values_global::find_value_local_auto_join(get_value_globals(), str, separator);
    }

    typed_lgr<types_global> attacha_environment::get_type(std::initializer_list<art::ustring> str) {
        return types_global::join_namespace(get_types_global(), str);
    }

    ValueItem& attacha_environment::get_value(std::initializer_list<art::ustring> str) {
        return values_global::join_namespace(get_value_globals(), str)->value;
    }

    ValueItem& attacha_environment::get_value_strict(std::initializer_list<art::ustring> str) {
        return values_global::join_namespace_strict(get_value_globals(), str)->value;
    }
} // namespace art