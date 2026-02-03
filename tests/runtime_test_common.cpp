#include "runtime_test_common.hpp"
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace runtime_test {

namespace {

constexpr int READ_END = 0;
constexpr int WRITE_END = 1;

} // namespace

std::pair<int, std::string> run_command_capture_output(const std::string& command,
                                                       int timeout_seconds)
{
    std::string output;
    int exit_code = -1;

    int pipe_fds[2];
    if (pipe(pipe_fds) != 0)
        return {-1, "pipe() failed: " + std::string(strerror(errno))};

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipe_fds[READ_END]);
        close(pipe_fds[WRITE_END]);
        return {-1, "fork() failed: " + std::string(strerror(errno))};
    }

    if (pid == 0)
    {
        // Child: new process group so we can kill the whole tree on timeout
        (void)setpgid(0, 0);
        close(pipe_fds[READ_END]);
        if (dup2(pipe_fds[WRITE_END], STDOUT_FILENO) < 0)
            _exit(127);
        if (dup2(pipe_fds[WRITE_END], STDERR_FILENO) < 0)
            _exit(127);
        close(pipe_fds[WRITE_END]);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    // Parent
    close(pipe_fds[WRITE_END]);
    int rfd = pipe_fds[READ_END];

    // Non-blocking so we can poll with timeout
    int flags = fcntl(rfd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(rfd, F_SETFL, flags | O_NONBLOCK);

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::seconds(static_cast<long>(timeout_seconds));
    bool timed_out = false;

    while (true)
    {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(rfd, &read_set);
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
        if (remaining <= 0)
        {
            timed_out = true;
            break;
        }
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int r = select(rfd + 1, &read_set, nullptr, nullptr, &tv);
        if (r > 0)
        {
            std::array<char, 4096> buf;
            ssize_t n = read(rfd, buf.data(), buf.size());
            if (n > 0)
                output.append(buf.data(), static_cast<size_t>(n));
            else if (n == 0)
            {
                // EOF: child closed write end; reap to get exit code
                int status = 0;
                if (waitpid(pid, &status, 0) == pid)
                {
                    if (WIFEXITED(status))
                        exit_code = WEXITSTATUS(status);
                    else if (WIFSIGNALED(status))
                        exit_code = 128 + WTERMSIG(status);
                }
                break;
            }
        }
        int status = 0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid)
        {
            if (WIFEXITED(status))
                exit_code = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                exit_code = 128 + WTERMSIG(status);
            break;
        }
        if (w < 0 && errno != EINTR)
            break;
    }

    if (timed_out)
    {
        // Kill process group first so any child processes (e.g. sleep) die too
        kill(-pid, SIGKILL);
        // Also ensure direct child is killed (in case setpgid failed)
        kill(pid, SIGKILL);
        // Wait with timeout so we don't hang if something goes wrong
        for (int i = 0; i < 50; ++i)
        {
            pid_t w = waitpid(pid, nullptr, WNOHANG);
            if (w == pid)
                break;
            if (w < 0 && errno != EINTR)
                break;
            usleep(100000); // 100ms
        }
        exit_code = -1;
        output += "\n[timed out; process group killed]\n";
    }
    else
    {
        // Drain pipe (child has exited; read until EOF)
        std::array<char, 4096> buf;
        ssize_t n;
        while ((n = read(rfd, buf.data(), buf.size())) > 0)
            output.append(buf.data(), static_cast<size_t>(n));
    }
    close(rfd);
    return {exit_code, output};
}

int run_command_split_output(const std::string& command,
                             std::string& stdout_output,
                             std::string& stderr_output,
                             int timeout_seconds)
{
    stdout_output.clear();
    stderr_output.clear();
    int exit_code = -1;

    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
    {
        stderr_output = "pipe() failed: " + std::string(strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(stdout_pipe[READ_END]);
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[READ_END]);
        close(stderr_pipe[WRITE_END]);
        stderr_output = "fork() failed: " + std::string(strerror(errno));
        return -1;
    }

    if (pid == 0)
    {
        // Child: new process group so we can kill the whole tree on timeout
        (void)setpgid(0, 0);
        
        close(stdout_pipe[READ_END]);
        close(stderr_pipe[READ_END]);
        
        if (dup2(stdout_pipe[WRITE_END], STDOUT_FILENO) < 0)
            _exit(127);
        if (dup2(stderr_pipe[WRITE_END], STDERR_FILENO) < 0)
            _exit(127);
        
        close(stdout_pipe[WRITE_END]);
        close(stderr_pipe[WRITE_END]);
        
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    // Parent
    close(stdout_pipe[WRITE_END]);
    close(stderr_pipe[WRITE_END]);
    int stdout_fd = stdout_pipe[READ_END];
    int stderr_fd = stderr_pipe[READ_END];

    // Make both pipes non-blocking
    int flags;
    if ((flags = fcntl(stdout_fd, F_GETFL, 0)) >= 0)
        fcntl(stdout_fd, F_SETFL, flags | O_NONBLOCK);
    if ((flags = fcntl(stderr_fd, F_GETFL, 0)) >= 0)
        fcntl(stderr_fd, F_SETFL, flags | O_NONBLOCK);

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::seconds(static_cast<long>(timeout_seconds));
    bool timed_out = false;
    bool stdout_closed = false, stderr_closed = false;

    while (true)
    {
        fd_set read_set;
        FD_ZERO(&read_set);
        if (!stdout_closed) FD_SET(stdout_fd, &read_set);
        if (!stderr_closed) FD_SET(stderr_fd, &read_set);
        int maxfd = (stdout_fd > stderr_fd ? stdout_fd : stderr_fd) + 1;

        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
        if (remaining <= 0)
        {
            timed_out = true;
            break;
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int r = select(maxfd, &read_set, nullptr, nullptr, &tv);
        
        if (r > 0)
        {
            std::array<char, 4096> buf;
            
            if (!stdout_closed && FD_ISSET(stdout_fd, &read_set))
            {
                ssize_t n = read(stdout_fd, buf.data(), buf.size());
                if (n > 0)
                    stdout_output.append(buf.data(), static_cast<size_t>(n));
                else if (n == 0)
                    stdout_closed = true;
            }
            
            if (!stderr_closed && FD_ISSET(stderr_fd, &read_set))
            {
                ssize_t n = read(stderr_fd, buf.data(), buf.size());
                if (n > 0)
                    stderr_output.append(buf.data(), static_cast<size_t>(n));
                else if (n == 0)
                    stderr_closed = true;
            }

            if (stdout_closed && stderr_closed)
            {
                // Both pipes closed, child must have exited
                int status = 0;
                if (waitpid(pid, &status, 0) == pid)
                {
                    if (WIFEXITED(status))
                        exit_code = WEXITSTATUS(status);
                    else if (WIFSIGNALED(status))
                        exit_code = 128 + WTERMSIG(status);
                }
                break;
            }
        }

        // Check if child exited
        int status = 0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid)
        {
            if (WIFEXITED(status))
                exit_code = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                exit_code = 128 + WTERMSIG(status);
            break;
        }
        if (w < 0 && errno != EINTR)
            break;
    }

    if (timed_out)
    {
        kill(-pid, SIGKILL);
        kill(pid, SIGKILL);
        for (int i = 0; i < 50; ++i)
        {
            pid_t w = waitpid(pid, nullptr, WNOHANG);
            if (w == pid || (w < 0 && errno != EINTR))
                break;
            usleep(100000);
        }
        exit_code = -1;
        stderr_output += "\n[timed out; process group killed]\n";
    }
    else
    {
        // Drain both pipes
        std::array<char, 4096> buf;
        ssize_t n;
        while ((n = read(stdout_fd, buf.data(), buf.size())) > 0)
            stdout_output.append(buf.data(), static_cast<size_t>(n));
        while ((n = read(stderr_fd, buf.data(), buf.size())) > 0)
            stderr_output.append(buf.data(), static_cast<size_t>(n));
    }

    close(stdout_fd);
    close(stderr_fd);
    return exit_code;
}

} // namespace runtime_test
