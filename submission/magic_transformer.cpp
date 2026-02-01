#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <wait.h>
#include <fcntl.h>

// use to collect all info from each process into a buffer
std::string read_all(int fd) {
    std::string buffer;
    char tmp[4096];
    ssize_t n;
    while ((n = read(fd, tmp, sizeof(tmp))) > 0) {
        buffer.append(tmp, n);
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "No output requested, have a nice day!" << std::endl;
        return 1;
    }

    // Routing map
    std::map<std::string, int> routing;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        auto pos = arg.find(':');
        if (pos == std::string::npos) return EXIT_FAILURE;

        std::string func = arg.substr(0, pos);
        std::string stream = arg.substr(pos + 1);

        int fd = (stream == "stdout") ? STDOUT_FILENO :
                 (stream == "stderr") ? STDERR_FILENO : -1;
        if (fd == -1) {
            std::cerr << "Unknown stream: " << stream << std::endl;
            return EXIT_FAILURE;
        }

        if (routing.count(func)) {
            std::cerr << "Error: " << func << " is directed to more than one output stream!" << std::endl;
            return EXIT_FAILURE;
        }

        if (func != "agent_rating" && func != "agent_performance" &&
            func != "state_rating" && func != "state_performance") {
            std::cerr << "Invalid functionality: " << func << std::endl;
            return EXIT_FAILURE;
        }

        routing[func] = fd;
    }

    // // Debug: print routing
    // for (auto &p : routing)
    //     std::cout << p.first << ":" << p.second << std::endl;

    /******************************* PIPES *******************************/
    int pipe1[2]; // transformer1 stdout -> transformer2
    int pipe2[2]; // transformer1 stderr -> transformer3
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /******************************* TRANSFORMER 1 *******************************/
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe1[0]);
        close(pipe2[0]);
        dup2(pipe1[1], STDOUT_FILENO);
        dup2(pipe2[1], STDERR_FILENO);
        close(pipe1[1]);
        close(pipe2[1]);
        execl("./transformer1", "transformer1", nullptr);
        perror("execl transformer1 failed");
        exit(EXIT_FAILURE);
    }
    close(pipe1[1]);
    close(pipe2[1]);

    /******************************* TRANSFORMER 2 *******************************/
    int t2_out[2], t2_err[2];
    if (pipe(t2_out) == -1 || pipe(t2_err) == -1) {
        perror("pipe transformer2");
        return EXIT_FAILURE;
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(t2_out[0]); close(t2_err[0]);
        dup2(pipe1[0], STDIN_FILENO);
        dup2(t2_out[1], STDOUT_FILENO);
        dup2(t2_err[1], STDERR_FILENO);
        close(pipe1[0]); close(t2_out[1]); close(t2_err[1]);
        execl("./transformer2", "transformer2", nullptr);
        perror("execl transformer2 failed");
        exit(EXIT_FAILURE);
    }
    close(pipe1[0]);
    close(t2_out[1]); close(t2_err[1]);

    /******************************* TRANSFORMER 3 *******************************/
    int t3_out[2], t3_err[2];
    if (pipe(t3_out) == -1 || pipe(t3_err) == -1) {
        perror("pipe transformer3");
        return EXIT_FAILURE;
    }

    pid_t pid3 = fork();
    if (pid3 == 0) {
        close(t3_out[0]); close(t3_err[0]);
        dup2(pipe2[0], STDIN_FILENO);
        dup2(t3_out[1], STDOUT_FILENO);
        dup2(t3_err[1], STDERR_FILENO);
        close(pipe2[0]); close(t3_out[1]); close(t3_err[1]);
        execl("./transformer3", "transformer3", nullptr);
        perror("execl transformer3 failed");
        exit(EXIT_FAILURE);
    }
    close(pipe2[0]);
    close(t3_out[1]); close(t3_err[1]);

    /******************************* READ BUFFERS *******************************/
    std::string t2_stdout = read_all(t2_out[0]);
    std::string t2_stderr = read_all(t2_err[0]);
    close(t2_out[0]); close(t2_err[0]);

    std::string t3_stdout = read_all(t3_out[0]);
    std::string t3_stderr = read_all(t3_err[0]);
    close(t3_out[0]); close(t3_err[0]);

    /******************************* ROUTE OUTPUTS *******************************/
    auto write_to_fd = [](int fd, const std::string &data) {
        write(fd, data.data(), data.size());
    };

    // Transformer2 outputs
    if (routing.count("agent_performance"))
        write_to_fd(routing["agent_performance"], t2_stdout);
    if (routing.count("state_performance"))
        write_to_fd(routing["state_performance"], t2_stderr);

    // Transformer3 outputs
    if (routing.count("agent_rating"))
        write_to_fd(routing["agent_rating"], t3_stdout);
    if (routing.count("state_rating"))
        write_to_fd(routing["state_rating"], t3_stderr);

    /******************************* WAIT FOR CHILDREN *******************************/
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
    waitpid(pid3, nullptr, 0);

    return EXIT_SUCCESS;
}
