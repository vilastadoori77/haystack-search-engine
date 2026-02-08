#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

// Global cleanup listener to kill any remaining child processes
class RuntimeTestCleanupListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunEnded(Catch::TestRunStats const&) override {
        // Kill any remaining searchd processes when all tests complete
        // Use system() to run pkill in a way that doesn't wait for orphaned processes
        std::system("pkill -9 searchd 2>/dev/null");
        std::system("pkill -9 -f 'sh -c.*searchd' 2>/dev/null");
        
        // Give processes time to die
        usleep(100000); // 100ms
        
        // Reap any remaining zombie child processes
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {
            // Keep reaping until no more zombies
        }
    }
};

CATCH_REGISTER_LISTENER(RuntimeTestCleanupListener)
