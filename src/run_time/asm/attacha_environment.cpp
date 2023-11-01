#include <run_time/asm/attacha_environment.hpp>

namespace art {
    attacha_environment attacha_environment::self;

    attacha_environment::attacha_environment() {
        value_globals = nullptr;
        function_globals = nullptr;
        code_gen = nullptr;
    }

    attacha_environment::~attacha_environment() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (value_globals)
            delete value_globals;

        if (function_globals)
            remove_function_globals(function_globals);

        if (code_gen)
            delete code_gen;
    }

    attacha_environment::function_globals_handle& attacha_environment::get_function_globals() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (!self.function_globals)
            self.function_globals = create_function_globals();
        return *self.function_globals;
    }

    ValueEnvironment& attacha_environment::get_value_globals() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (!self.value_globals)
            self.value_globals = new ValueEnvironment();
        return *self.value_globals;
    }

    attacha_environment::code_gen_handle& attacha_environment::get_code_gen() {
        art::lock_guard<art::TaskRecursiveMutex> lock(self.mutex);
        if (!self.code_gen)
            self.code_gen = new code_gen_handle();
        return *self.code_gen;
    }
} // namespace art