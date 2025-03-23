// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/language/c@.hpp>
#include <base/language/precompiled.hpp>
#include <base/run_time.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/attacha_environment.hpp>
#include <run_time/asm/exception.hpp>
#include <run_time/library/cxx/language.hpp>
#include <run_time/library/cxx_binds/console.hpp>
#include <run_time/standard_lib.hpp>

#include <boost/program_options.hpp>
#include <filesystem>
using namespace art;

template <const char* prefix>
ValueItem* logger(ValueItem* args, uint32_t argc) {
    art::ustring output(prefix);
    switch (argc) {
    case 0:
        if (exception::has_exception()) {
            std::unique_ptr<ValueItem> full_desc;
            full_desc.reset(exception::get_current_exception_full_description());
            if (full_desc) {
                output += "caught_exception " + (art::ustring)*full_desc;
            } else {
                std::unique_ptr<ValueItem> name;
                name.reset(exception::get_current_exception_name());
                if (name)
                    output += "caught_exception " + (art::ustring)*full_desc;
            }
        }
        break;
    case 1:
        output += (art::ustring)args[0];
        break;
    default:
        output += ": [";
        for (uint32_t i = 0; i < argc; i++) {
            output += (art::ustring)args[i];
            if (i != argc - 1)
                output += ", ";
        }
        output += "]";
    }
    art_lib::console::printLine(output);
    return nullptr;
}

const char _FATAL[] = "ERROR ";
const char _ERROR[] = "ERROR ";
const char _WARN[] = "WARN ";
const char _INFO[] = "INFO ";
struct runtime_state {
    list_array<ValueItem> args;
    std::string executing_path;
    std::string input_path;
    std::string main_function;
    bool recursive = false;
    bool safelib = false;
    bool disable_cmath = false;
};

namespace po = boost::program_options;

std::vector<po::option> end_of_opts_parser(std::vector<std::string>& args) {
    std::vector<po::option> result;
    auto i = args.begin();
    if (i != args.end() && *i == "--") {
        po::option opt;
        opt.string_key = "args";
        opt.value = std::vector<std::string>(std::move_iterator(i), std::move_iterator(args.end()));
        opt.original_tokens.push_back(*i);
        result.push_back(opt);
        args.clear();
    }
    return result;
}

runtime_state process_options(const char** argv, int argc) {
    po::command_line_parser cl_parser(argc, argv);
    cl_parser.extra_style_parser(end_of_opts_parser);
    // clang-format off
    po::options_description desc("Art options");
    desc.add_options()
        ("help,h", "produce help message")
        ("main-function,F", po::value<std::string>(), "function symbol name to run on start")
        ("input-path,I", po::value<std::string>(), "input path")
        ("executing-path,E", po::value<std::string>(), "executing path")
        ("version,v", "print version")
        ("safelib,s", "disables functions from standard library marked as unsafe")
        ("no-cmath", "disables CMath library")
        ("recursive,r", "process input path recursively")
    ;

    po::options_description hidden;
    hidden.add_options()
        ("args", po::value<std::vector<std::string>>()->multitoken(), "pass args to program")
    ;

    po::options_description all_options;
    all_options.add(desc).add(hidden);

    po::positional_options_description positional;
    positional.add("input-path,I", -1);
    // clang-format on

    boost::program_options::variables_map vm;
    boost::program_options::store(
        cl_parser.options(all_options).positional(positional).run(),
        vm
    );
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::stringstream ss;
        po::options_description help("Art options");
        help.add(desc).add_options()("--", "end of options, pass args to program");
        help.print(ss);
        art_lib::console::printLine(ss.str());
        exit(0);
    }

    if (vm.count("version")) {
        art_lib::console::printLine(ART_VERSION_STR);
        exit(0);
    }

    runtime_state state;
    if (vm.count("recursive"))
        state.recursive = true;
    if (vm.count("safelib"))
        state.safelib = true;
    if (vm.count("no-cmath"))
        state.disable_cmath = true;
    if (vm.contains("args"))
        state.args = vm.at("args").as<std::vector<std::string>>();
    if (vm.contains("executing-path"))
        state.executing_path = vm.at("executing-path").as<std::string>();
    if (vm.contains("input-path"))
        state.input_path = vm.at("input-path").as<std::string>();
    else
        state.input_path = ".";
    if (vm.contains("main-function"))
        state.main_function = vm.at("main-function").as<std::string>();
    return state;
}

template <class T>
auto tttt(T&& typ) {
    return art::shared_ptr<T>();
}

bool process_state(runtime_state& state) {
    language::language_provider provider(state.input_path, state.recursive);
    provider.register_language<language_parsers::precompiled>();
    provider.register_language<language_parsers::c_async>();
    provider.run_once();


    bool faulty_start = false;
    art::shared_ptr<FuncEnvironment> start_function;
    if (state.main_function.empty()) {
        art::ustring set_name;
        FuncEnvironment::enum_functions([&](const art::ustring& name, const art::shared_ptr<FuncEnvironment>& fn) {
            if (name.starts_with('>')) {
                if (!start_function) {
                    start_function = fn;
                    set_name = name;
                } else if (faulty_start)
                    CXX::cxxCall(logger<_FATAL>, art::ustring("Found multiple default main functions : ") + name);
                else {
                    CXX::cxxCall(logger<_FATAL>, art::ustring("Found multiple default main functions : ") + set_name);
                    CXX::cxxCall(logger<_FATAL>, art::ustring("Found multiple default main functions : ") + name);
                    faulty_start = false;
                }
            }
        });
    } else
        start_function = FuncEnvironment::environment(state.main_function);
    if (!start_function) {
        if (state.main_function.empty())
            CXX::cxxCall(logger<_FATAL>, "failed to find default entry point.");
        else
            CXX::cxxCall(logger<_FATAL>, "no such function: " + state.main_function + ".");
        faulty_start = true;
    }

    if (!faulty_start) {
        Task::start(new Task(start_function, {state.args}));
        Task::become_executor_count_manager(true);
    }
    return faulty_start;
}

int main(int argc, const char** argv) {
    Task::create_executor();
    initRuntime();
    try {
        CXX::Interface::getExtractAsStatic<typed_lgr<EventSystem>>(attacha_environment::get_value({"run_time", "event", "unhandled_exception"}))->join(new FuncEnvironment(logger<_ERROR>, false, false));
        CXX::Interface::getExtractAsStatic<typed_lgr<EventSystem>>(attacha_environment::get_value({"run_time", "event", "error"}))->join(new FuncEnvironment(logger<_ERROR>, false, false));
        CXX::Interface::getExtractAsStatic<typed_lgr<EventSystem>>(attacha_environment::get_value({"run_time", "event", "warning"}))->join(new FuncEnvironment(logger<_WARN>, false, false));
        CXX::Interface::getExtractAsStatic<typed_lgr<EventSystem>>(attacha_environment::get_value({"run_time", "event", "info"}))->join(new FuncEnvironment(logger<_INFO>, false, false));

        auto state = process_options(argv, argc);
        if (state.executing_path.size() != 0)
            std::filesystem::current_path(state.executing_path);
        if (!state.disable_cmath)
            initCMathLib();

        if (state.safelib)
            initStandardLib_safe();
        else
            initStandardLib();
        bool faulty_start = process_state(state);
        deinitRuntime();
        return faulty_start;
    } catch (const std::exception& e) {
        CXX::cxxCall(logger<_FATAL>, e.what());
    } catch (const AttachARuntimeException& e) {
        CXX::cxxCall(logger<_FATAL>, e.full_info());
    } catch (...) {
        CXX::cxxCall(logger<_FATAL>, "unknown exception");
    }
    return -1;
}
