#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace
{

std::atomic<bool> handleSignal = false;

volatile std::sig_atomic_t signalStatus;

#ifdef __cplusplus
extern "C"
{
#endif

void signal_handler__(int signal)
{
    static_assert(
            std::atomic<bool>::is_always_lock_free,
            "using std::atomic<bool> is undefined behavior"
            " in signal handler function");

    signalStatus = signal;

    handleSignal.store(true);
}

#ifdef __cplusplus
} // extern "C"
#endif

} // namespace

int main(int /* argc */, char** /* argv */) {
    if (std::signal(SIGTERM, signal_handler__) == SIG_ERR) {
        std::cerr << "register signal_handler failure." << "\n";

        return 1;
    }

    while (!handleSignal.load()) {
        std::cout << "echo: "
                << std::chrono::system_clock::now().time_since_epoch().count()
                << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (handleSignal.load()) {
        std::cout << "handle signal number: " << signalStatus << "\n";
    }

    return 0;
}
