#include <boost/asio/io_context.hpp>
#include <boost/process/async.hpp>
#include <boost/process/child.hpp>
#include <boost/process/extend.hpp>
#include <boost/process/io.hpp>
#include <boost/system/error_code.hpp>

#include <cstdio>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

namespace
{

// * boost::process::extend::handler
//     Define `on_setup`, `on_error`, `on_success`
// * boost::process::extend::require_io_context
//     Provide boost::asio::io_context from `boost::process::extend::get_io_context`
// * boost::process::extend::async_handler
//    Define `on_exit_handler`. inherits boost::process::extend::handler and
//    boost::process::extend::require_io_context
class process_handler : // public boost::process::extend::handler,
                    // public boost::process::extend::require_io_context
                    public boost::process::extend::async_handler
{
public:

    process_handler(std::string name)
            : name_(std::move(name))
    {

    }

    process_handler(const process_handler&) = delete;

    process_handler(process_handler&&) = delete;

    template<typename Executor>
    void on_setup(Executor& exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        [name = this->name_]()
        {
            std::cout << name << " on setup." << std::endl;
        });
    }

    template<typename Executor>
    void on_error(Executor& exec, const std::error_code& ec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        [ec, name = this->name_]()
        {
            std::cout << name << " on start error."
                    << " ec.message: " << ec.message() << std::endl;
        });
    }

    template<typename Executor>
    void on_success(Executor& exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        [name = this->name_]()
        {
            std::cout << name << " on start success." << std::endl;
        });
    }

    template<typename Executor>
    std::function<void(int, const std::error_code&)> on_exit_handler(Executor& exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);

        return [&ios, name = this->name_](int exit_code, const std::error_code& ec)
               {
                    std::cout << name << " exit code: " << exit_code << " ec.message:" << ec.message() << std::endl;
                    boost::asio::post(ios.get_executor(),
                    []()
                    {
                        std::cout << "after on exit." << std::endl;
                    });
               };

    }

    process_handler& operator=(const process_handler&) = delete;

    process_handler& operator=(process_handler&&) = delete;

private:

    std::string name_;

};

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Require arguments executable path" << "\n"
                  << "e.g. spawn_and_terminate echo_and_sleep" << "\n";

        return 0;
    }

    const std::filesystem::path executablePath = std::string(argv[1]);

    boost::asio::io_context ioContext;

    std::cout << "launch " << executablePath.filename() << "\n";

    boost::process::child targetProcess(
            executablePath.string(),
            boost::process::std_in.close(),
            boost::process::std_out > stdout,
            boost::process::std_err > stderr,
            ioContext,
            process_handler(executablePath.string()));

    std::thread ioLoop([&ioContext](){ ioContext.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    std::error_code waitForEc;
    const bool alreadyExit =
            targetProcess.wait_for(std::chrono::milliseconds(500), waitForEc);
    if (waitForEc) {
        std::cout << "wait for process exit failure."
                << "ec.message: " << waitForEc.message() << "\n";
    }

    if (alreadyExit) {
        std::cout << executablePath.filename() << " is already exit." << "\n";
    } else {
        std::error_code terminateEc;
        targetProcess.terminate(terminateEc);

        // Check operator bool() and value() == 0,
        // terminateEc.value() == 0 but operator bool() returns false on Windows.
        if (terminateEc || terminateEc.value() == 0) {
            std::cout << executablePath.filename() << " terminate." << "\n";
        } else {
            std::cout << executablePath.filename() << " terminate failure."
                    << " ec.message: " << terminateEc.message() << "\n";
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    ioContext.stop();

    ioLoop.join();

    return 0;
}
