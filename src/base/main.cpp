// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/run_time.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/exception.hpp>
#include <run_time/library/console.hpp>
#include <run_time/library/internal.hpp>
#include <run_time/standard_lib.hpp>
#include <run_time/tasks/util/interrupt.hpp>

#include <run_time/library/cxx_binds/console.hpp>

using namespace art;

#pragma optimize("", off)

ValueItem* busyWorker(ValueItem* args, uint32_t argc) {
    size_t i = 0;
    for (; i < UINT64_MAX - 1; i++)
        ;
    return new ValueItem(i);
}
#pragma optimize("", on)

ValueItem* attacha_main(ValueItem* args, uint32_t argc) {
    ValueItem noting;
    //Task::start(new Task(FuncEnvironment::environment("busy_worker"), noting));

    art_lib::console::setBgColor(123, 21, 2);
    art_lib::console::setTextColor(0, 230, 0);
    art_lib::console::print("test");

    FuncEnviroBuilder build;
    auto fn_console_set_text_color = build.create_constant("console set_text_color");
    auto fn_console_printf = build.create_constant("console printf");
    auto text = build.create_constant("The test text, Current color: r%d,g%d,b%d\n");
    auto num12 = build.create_constant((uint8_t)12);
    auto num128 = build.create_constant((uint8_t)128);
    auto num0 = build.create_constant((uint8_t)0);
    auto num1 = build.create_constant((uint8_t)1);
    auto num2 = build.create_constant((uint8_t)2);
    auto num3 = build.create_constant((uint8_t)3);

    build.set_stack_any_array(0_env, 4);
    build.arg_set(0_env);

    build.static_arr(0_env, VType::saarr).set(num12, num0, false, ArrCheckMode::no_check);
    build.static_arr(0_env, VType::saarr).set(num128, num1, false, ArrCheckMode::no_check);
    build.static_arr(0_env, VType::saarr).set(num12, num2, false, ArrCheckMode::no_check);
    build.call(fn_console_set_text_color);

    build.static_arr(0_env, VType::saarr).set(text, num0, false, ArrCheckMode::no_check);
    build.static_arr(0_env, VType::saarr).set(num12, num1, false, ArrCheckMode::no_check);
    build.static_arr(0_env, VType::saarr).set(num128, num2, false, ArrCheckMode::no_check);
    build.static_arr(0_env, VType::saarr).set(num12, num3, false, ArrCheckMode::no_check);
    build.arg_set(0_env);
    build.call(fn_console_printf);

    build.remove(0_env);
    build.ret();
    build.O_load_func("start");
    CXX::cxxCall("start");

    // for (size_t i = 0; i < 1000000; i++)
    //	CXX::cxxCall("start");

    {
        FuncEnviroBuilder build;
        auto fn_Yay = build.create_constant("Yay");
        build.call_and_ret(fn_Yay);
        build.O_load_func("Yay");
    }
    art::shared_ptr<FuncEnvironment> env = CXX::MakeNative([]() {
        auto started = std::chrono::high_resolution_clock::now();
        Task::sleep(1000);
        uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
        art_lib::console::printLine(time);
    });
    ////Task::start(new Task(FuncEnvironment::environment("4"), nullptr));
    // Task::start(new Task(FuncEnvironment::environment("3"), nullptr));
    // Task::start(new Task(FuncEnvironment::environment("2"), nullptr));
    // Task::start(new Task(FuncEnvironment::environment("1"), nullptr));

    Task::await_end_tasks(true);
    try {
        CXX::cxxCall("start");
    } catch (const std::exception&) {
        art_lib::console::print("Catched!\n");
    } catch (const StackOverflowException&) {
        art_lib::console::print("Catched!\n");
    }

    // cxx::console::print("Hello!\n");
    int e = 0;
    Task::await_end_tasks(true);
    {
        Task::start(new Task(env, noting));
        Task::await_end_tasks(true);
    }

    list_array<art::typed_lgr<Task>> tasks;

    // for (size_t i = 0; i < 10000; i++) {
    //	tasks.push_back(new Task(FuncEnvironment::environment("start"), noting));
    //	tasks.push_back(new Task(FuncEnvironment::environment("1"), noting));
    //	tasks.push_back(new Task(env, noting));
    // }
    //
    // Task::await_multiple(tasks);
    // Task::clean_up();
    // tasks.clear();
    /// for (size_t i = 0; i < 10000; i++)
    ///	tasks.push_back(new Task(env, noting));
    /// Task::await_multiple(tasks);
    /// Task::clean_up();
    /// tasks.clear();
    /// for (size_t i = 0; i < 10000; i++)
    ///	tasks.push_back(new Task(env, noting));
    /// Task::await_multiple(tasks);
    /// tasks.clear();
    for (size_t i = 0; i < 10000; i++)
        tasks.push_back(new Task(env, noting));
    Task::await_multiple(tasks);
    tasks.clear();
    Task::clean_up();

    art_lib::console::resetBgColor();
    Task::sleep(1000);
    return new ValueItem(e);
}

template <const char* prefix>
ValueItem* logger(ValueItem* args, uint32_t argc) {
    art::ustring output(prefix);
    output += ": [";
    for (uint32_t i = 0; i < argc; i++) {
        output += (art::ustring)args[i];
        if (i != argc - 1)
            output += ", ";
    }
    output += "]";
    art_lib::console::printLine(output);
    return nullptr;
}

const char _FATAL[] = "FATAL";
const char _ERROR[] = "ERROR";
const char _WARN[] = "WARN";
const char _INFO[] = "INFO";
#include <run_time/ValueEnvironment.hpp>

int mmain() {
    unhandled_exception.join(new FuncEnvironment(logger<_FATAL>, false, false));
    errors.join(new FuncEnvironment(logger<_ERROR>, false, false));
    warning.join(new FuncEnvironment(logger<_WARN>, false, false));
    info.join(new FuncEnvironment(logger<_INFO>, false, false));
    auto timer_start = std::chrono::high_resolution_clock::now();
    initStandardLib();
    FuncEnvironment::AddNative(busyWorker, "busy_worker", false);
    enable_thread_naming = true;
    Task::start_interrupt_handler();
    Task::max_running_tasks = 20000;
    Task::max_planned_tasks = 0;
    Task::enable_task_naming = false;

    art::typed_lgr<Task> main_task = new Task(new FuncEnvironment(attacha_main, false, false), {});
    main_task->bind_to_worker_id = Task::create_bind_only_executor(1, true);
    Task::create_executor(2);
    Task::start(main_task);
    ValueItem* res = Task::get_result(main_task);
    if (res != nullptr)
        res->getAsync();
    Task::shutDown();
    Task::clean_up();
    auto timer_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = timer_end - timer_start;
    art_lib::console::printLine("Time: " + std::to_string(elapsed.count()) + "ms");
    if (res != nullptr) {
        try {
            art_lib::console::printLine(*res);
        } catch (...) {
            return -1;
        }
        delete res;
        return 0;
    } else
        return 0;
}

#include "run_time/library/cxx/files.hpp"
#include "run_time/library/cxx/networking.hpp"
static constexpr char file_path[] = "D:\\sample_hello_world_http_response.txt";

void test_slow_server_http(TcpNetworkStream& stream) {
    if (!stream.is_closed()) {
        files::FileHandle file(file_path, sizeof(file_path), files::open_mode::read, files::on_open_action::open, files::_async_flags{.sequential_scan = true});
        auto file_read = file.read((uint32_t)file.size());

        while (stream.data_available())
            stream.read_available_ref();
        stream.write((array_t<char>)file_read);
    }
}

files::FileHandle file(file_path, sizeof(file_path), files::open_mode::read, files::on_open_action::open, files::_async_flags{.sequential_scan = true});

AttachAFunc(class_transfer_test, 1) {
    Structure& struct_ = (Structure&)args[0];
    art_lib::console::printLine(struct_.get_name());
    return nullptr;
}

void test_fast_server_http(TcpNetworkStream& stream) {
    //CXX::cxxCall(class_transfer_test, stream);
    if (!stream.is_closed()) {
        while (stream.data_available())
            stream.read_available_ref();
        stream.write_file(file.internal_get_handle());
    }
}

int ymain() {
    Task::create_executor(1);
    init_networking();
    initStandardLib_file();
    unhandled_exception.join(CXX::MakeNative(logger<_FATAL>, false, false));
    errors.join(CXX::MakeNative(logger<_ERROR>, false, false));
    warning.join(CXX::MakeNative(logger<_WARN>, false, false));
    info.join(CXX::MakeNative(logger<_INFO>, false, false));

    TcpNetworkServer server(CXX::MakeNative(test_slow_server_http, false, false), "0.0.0.0:1234", TcpNetworkServer::ManageType::write_delayed, 20);
    TcpNetworkServer server2(CXX::MakeNative(test_fast_server_http, false, false), "0.0.0.0:1235", TcpNetworkServer::ManageType::write_delayed, 20);

    server.start();
    server2.start();
    Task::become_executor_count_manager(true);
    server._await();
    server2._await();
    return 0;
}

int main() {
    art_lib::console::resetTextColor();
    art_lib::console::printLine("Sample execution flow begin:");
    art_lib::console::setTextColor(0, 0, 230);
    art_lib::console::print("Main");
    art_lib::console::resetTextColor();
    art_lib::console::print("   -   ");
    art_lib::console::setTextColor(0, 230, 0);
    art_lib::console::print("Generator");
    art_lib::console::resetTextColor();
    art_lib::console::print("   -   ");
    art_lib::console::setTextColor(230, 0, 0);
    art_lib::console::printLine("Iterator");


    art::shared_ptr gen = new Generator(
        CXX::MakeNative([](void* generator_ref) {
            Generator* ref = (Generator*)generator_ref;

            art_lib::console::setTextColor(0, 230, 0);
            art_lib::console::printLine("                #");
            Generator::result(ref, new ValueItem(1));

            art_lib::console::setTextColor(0, 230, 0);
            art_lib::console::printLine("                #");
            Generator::yield(ref, new ValueItem(2));

            art_lib::console::setTextColor(0, 230, 0);
            art_lib::console::printLine("                #");
            Generator::yield(ref, new ValueItem(3));

            art_lib::console::setTextColor(0, 230, 0);
            art_lib::console::printLine("                #");
            return 123;
        }),
        {}
    );
    art_lib::console::setTextColor(0, 0, 230);
    art_lib::console::printLine("  #");
    for (ValueItem item : Generator::cxx_iterate(gen)) {
        art_lib::console::setTextColor(230, 0, 0);
        art_lib::console::printf("                                # []\n", item);
    }
    art_lib::console::setTextColor(0, 0, 230);
    art_lib::console::printLine("  #");
    Generator::restart_context(gen);
    auto results = Generator::await_results(gen);
    art_lib::console::setTextColor(0, 0, 230);
    art_lib::console::printf("  # []\n", results);
    art_lib::console::resetTextColor();
    art_lib::console::printLine("Sample execution end.");
    //ymain();
    Task::create_executor(4);
    auto itt = 100;
    art_lib::console::printf("Hello from main block! []\n", art::this_thread::get_id());

    art_wait {
        art_lib::console::printf("Main block value: [], Hello from wait block! []\n", itt, art::this_thread::get_id());
        itt = 222;
    };

    auto task = art_async {
        art_lib::console::printf("Main block value: [], Hello from async block! []\n", itt, art::this_thread::get_id());
        Task::sleep(1000);
        itt = 333;
    };
    Task::sleep(500);
    art_lib::console::printf("Main block value: [], Hello from main block! []\n", itt, art::this_thread::get_id());
    art_await task;
    art_lib::console::printf("Main block value: [], Hello from main block! []\n", itt, art::this_thread::get_id());


    auto lambda = [&itt](ValueItem*, uint32_t) -> ValueItem* {
        art_lib::console::printf("Hello from lambda, captured value: []\n", itt);
        return nullptr;
    };
    CXX::cxxCall(CXX::MakeNative(lambda));
    Task::shutDown();
    return 0;
}
