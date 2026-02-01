#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <map>
#include <wait.h>

int main(int argc, char *argv[]) {

    if (argc == 1) {
        std::cerr << "No output requested, have a nice day!" << std::endl;
        return 1;
    }

    // Map with routing table
    // functionality, stream to send
    std::map<std::string, int> routing;

    // Parse command line arguments into map
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        auto pos = arg.find(':');
        if (pos == std::string::npos) return EXIT_FAILURE;
        

        std::string func = arg.substr(0, pos);
        std::string stream = arg.substr(pos + 1);

        int fd = (stream == "stdout") ? STDOUT_FILENO :
                (stream == "stderr") ? STDERR_FILENO : -1;

        if (fd == -1) {
            std::cerr << "Error: Unknown stream '" << stream << "'!" << std::endl;
            return EXIT_FAILURE;
        }

        if (routing.count(func)) {
            std::cerr << "Error: " << func << " is directed to more than one output stream!" << std::endl;
            return EXIT_FAILURE;
        }

        // make sure we are specifying valid functionality
        if (func != "agent_rating" &&
            func != "agent_performance" &&
            func != "state_rating" &&
            func != "state_performance") {
            std::cerr << "Error: invalid functionality '" << func << "'\n";
            return 1;
        }

        routing[func] = fd;
    }

    // create pipes with syscalls
    int pipe_I_II[2];  
    int pipe_I_III[2];

    pipe(pipe_I_II);
    pipe(pipe_I_III);

    std::cerr << "Pipes created" << std::endl;

    pid_t pid1 = fork();
    // std::cerr << "PID of transformer I: " << pid1 << std::endl;
    if (pid1 < 0) {
        std::cerr << "Fork failed for transformer I" << std::endl;
        return EXIT_FAILURE;
    }
    if (pid1 == 0) {
        std::cerr << "Forked transformer I\n";
        // stdout -> pipe to Transformer II
        dup2(pipe_I_II[1], STDOUT_FILENO);
        // stderr -> pipe to Transformer III
        dup2(pipe_I_III[1], STDERR_FILENO);

        close(pipe_I_II[0]);
        close(pipe_I_II[1]);
        close(pipe_I_III[0]);
        close(pipe_I_III[1]);

        if (!routing.count("state_rating") && !routing.count("agent_rating")) {
            close(STDERR_FILENO);
        }
        if (!routing.count("agent_performance") && !routing.count("state_performance")) {
            close(STDOUT_FILENO);
        }

        
        execl("./transformer1", "transformer1", (char*)nullptr);
        //perror("exec transformer1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // stdin <- pipe from Transformer I
        dup2(pipe_I_II[0], STDIN_FILENO);

        // Handle agent_performance (stdout)
        if (!routing.count("agent_performance")) {
            close(STDOUT_FILENO);
        }

        // Handle state_performance (stderr)
        if (!routing.count("state_performance")) {
            close(STDERR_FILENO);
        }

        close(pipe_I_II[0]);
        close(pipe_I_II[1]);
        close(pipe_I_III[0]);
        close(pipe_I_III[1]);

        std::cerr << "Forked transformer II\n";
        execl("./transformer2", "transformer2", (char*)nullptr);
        //perror("exec transformer2");
        exit(1);
    }

    pid_t pid3 = fork();
    if (pid3 == 0) {
        // stdin <- pipe from Transformer I
        dup2(pipe_I_III[0], STDIN_FILENO);

        // Handle agent_rating (stdout)
        if (!routing.count("agent_rating")) {
            close(STDOUT_FILENO);
        }

        // Handle state_rating (stderr)
        if (!routing.count("state_rating")) {
            close(STDERR_FILENO);
        }

        close(pipe_I_II[0]);
        close(pipe_I_II[1]);
        close(pipe_I_III[0]);
        close(pipe_I_III[1]);

        std::cerr << "Forked transformer III\n";
        execl("./transformer3", "transformer3", (char*)nullptr);
        //perror("exec transformer3");
        exit(1);
    }

    close(pipe_I_II[0]);
    close(pipe_I_II[1]);
    close(pipe_I_III[0]);
    close(pipe_I_III[1]);

    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);


    return EXIT_SUCCESS;
}