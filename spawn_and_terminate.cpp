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

    template<typename Executor>
    void on_setup(Executor& exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        []()
        {
            std::cout << "on setup." << std::endl;
        });
    }

    template<typename Executor>
    void on_error(Executor &exec, const std::error_code& ec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        [ec]()
        {
            std::cout << "on start error."
                    << " ec.message: " << ec.message() << std::endl;
        });
    }

    template<typename Executor>
    void on_success(Executor & exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        boost::asio::post(ios.get_executor(),
        []()
        {
            std::cout << "on start success." << std::endl;
        });
    }


    template<typename Executor>
    std::function<void(int, const std::error_code&)> on_exit_handler(Executor & exec)
    {
        auto &ios = boost::process::extend::get_io_context(exec.seq);
        return [&ios](int exit_code, const std::error_code & ec)
               {
                    std::cout << "exit code: " << exit_code << " ec.message:" << ec.message() << std::endl;
                    boost::asio::post(ios.get_executor(),
                    []()
                    {
                        std::cout << "after on exit." << std::endl;
                    });
               };

    }
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
            process_handler());

    std::thread ioLoop([&ioContext](){ ioContext.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    std::error_code ec;
    if (targetProcess.wait_for(std::chrono::milliseconds(500), ec)) {
        std::cout << executablePath.filename() << " is already exit." << "\n";
    } else {
        if (ec) {
            std::cout << "wait for process exit failure."
                    << "ec.message: " << ec.message() << "\n";
        }

        std::error_code terminateEc;
        targetProcess.terminate(terminateEc);

        if (terminateEc) {
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
