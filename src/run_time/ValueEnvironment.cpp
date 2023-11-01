#include <run_time/ValueEnvironment.hpp>

namespace art {
    ValueEnvironment::ValueEnvironment(ValueEnvironment* parent)
        : parent(parent) {}

    ValueEnvironment::~ValueEnvironment() {
        clear();
    }

    typed_lgr<ValueEnvironment> ValueEnvironment::joinEnvironment(const art::ustring& str) {
        auto it = environments.find(str);
        if (it == environments.end())
            return environments[str] = new ValueEnvironment(this);
        return it->second;
    }

    typed_lgr<ValueEnvironment> ValueEnvironment::joinEnvironment(const std::initializer_list<art::ustring>& strs) {
        typed_lgr<ValueEnvironment> current_environment(this, true);
        for (auto& str : strs) {
            auto it = current_environment->environments.find(str);
            if (it == current_environment->environments.end()) {
                current_environment = environments[str] = new ValueEnvironment(this);
            } else
                current_environment = it->second;
        }
        return current_environment;
    }

    bool ValueEnvironment::hasEnvironment(const art::ustring& str) {
        return environments.contains(str);
    }

    bool ValueEnvironment::hasEnvironment(const std::initializer_list<art::ustring>& strs) {
        typed_lgr<ValueEnvironment> current_environment(this, true);
        for (auto& str : strs) {
            auto it = current_environment->environments.find(str);
            if (it == current_environment->environments.end())
                return false;
            current_environment = it->second;
        }
        return true;
    }

    void ValueEnvironment::removeEnvironment(const art::ustring& str) {
        auto it = environments.find(str);
        if (it != environments.end())
            environments.erase(it);
    }

    void ValueEnvironment::removeEnvironment(const std::initializer_list<art::ustring>& strs) {
        typed_lgr<ValueEnvironment> prev_environment(this, true);
        typed_lgr<ValueEnvironment> current_environment(this, true);
        decltype(environments)::iterator it = current_environment->environments.end();
        for (auto& str : strs) {
            auto it = current_environment->environments.find(str);
            if (it == current_environment->environments.end())
                return;
            prev_environment = current_environment;
            current_environment = it->second;
        }
        prev_environment->environments.erase(it);
    }

    void ValueEnvironment::clear() {
        environments.clear();
        value = nullptr;
    }

    ValueItem* ValueEnvironment::findValue(const art::ustring& str) {
        typed_lgr<ValueEnvironment> current_environment(this, true);
        while (current_environment) {
            auto it = current_environment->environments.find(str);
            if (it == current_environment->environments.end())
                current_environment = current_environment->parent;
            else
                return &it->second->value;
        }
        return nullptr;
    }

    bool ValueEnvironment::depth_safety() {
        for (auto& [name, env] : environments)
            if (!env.depth_safety())
                return false;
        return true;
    }
}