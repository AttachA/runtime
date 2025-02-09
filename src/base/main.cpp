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

const char _FATAL[] = "FATAL";
const char _ERROR[] = "ERROR";
const char _WARN[] = "WARN";
const char _INFO[] = "INFO";

struct runtime_state {
    std::vector<std::string> args;
    std::string executing_path;
    std::string input_path;
    std::string main_function;
    bool recursive = false;
    bool safelib = false;
    bool cmath = false;
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
        ("cmath", "enables CMath library")
        ("recursive,r", "process input path recursively")
    ;

    po::options_description hidden;
    hidden.add_options()
        ("args,", po::value<std::vector<std::string>>()->multitoken(), "pass args to program")
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
        desc.print(ss);
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
    if (vm.count("cmath"))
        state.cmath = true;
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

int main(int argc, const char** argv) {
    atexit([]() {
        Task::shutDown();
        Task::clean_up();
    });

    auto state = process_options(argv, argc);
    if (state.executing_path.size() != 0)
        std::filesystem::current_path(state.executing_path);
    if (state.safelib)
        initStandardLib_safe();
    else
        initStandardLib();

    if (state.cmath)
        initCMathLib();

    language::language_provider provider(state.input_path, state.recursive);
    {
        art::shared_ptr<art::language::language_handler> precompiled(new language_parsers::precompiled());
        provider.register_language("art", precompiled);
        provider.register_language("pcart", precompiled);
        provider.register_language("precart", precompiled);
    }
    {
        art::shared_ptr<art::language::language_handler> c_async(new language_parsers::c_async());
        provider.register_language("c@", c_async);
        provider.register_language("c_async", c_async);
    }
    unhandled_exception.join(new FuncEnvironment(logger<_FATAL>, false, false));
    errors.join(new FuncEnvironment(logger<_ERROR>, false, false));
    warning.join(new FuncEnvironment(logger<_WARN>, false, false));
    info.join(new FuncEnvironment(logger<_INFO>, false, false));
    Task::create_executor();

    provider.start();

    art::shared_ptr<FuncEnvironment> start_function;
    if (state.main_function.empty())
        FuncEnvironment::enum_functions([&](const art::ustring& name, const art::shared_ptr<FuncEnvironment>& fn) {

        });
    else
        start_function = FuncEnvironment::environment(state.main_function);
    if (!start_function) {
        ValueItem text(art::ustring("no such function: ") + state.main_function);
        errors.await_notify(text);
        exit(1);
    }
    Task::start(new Task(start_function, {}));
    Task::become_executor_count_manager(true);
}