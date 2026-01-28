// #include <iostream>
// #include <client.hpp>
// #include <server.hpp>
#include <run_cgi.hpp>
#include <time.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>


bool start_cgi(Client& clientObj, request& req, rt& route, int epoll_fd, std::map<int, int>& cgi_fds) {
    int pipefd[2]; // for script output
    int postfd[2]; // for post
    if (pipe(pipefd) < 0 || pipe(postfd) < 0)
        return false;

    pid_t pid = fork();
    if(pid < 0)
        return false;
    if (pid == 0)
    {
        // CHILD
        if(req.getMethod() == "POST")
            dup2(postfd[0], STDIN_FILENO);
        // for set ouput of cgi in the pipe
        dup2(pipefd[1], STDOUT_FILENO); // pipefd[1] = write end && pipefd[0] = read end
        close(postfd[0]);
        close(postfd[1]);
        close(pipefd[0]);
        close(pipefd[1]);
        char* argv[3];
        argv[0] = const_cast<char*>(route.cgiInterpreter().c_str()); // e.g., "/usr/bin/python"
        argv[1] = const_cast<char*>(route.cgiScript().c_str()); // e.g., "/path/to/script.py"
        argv[2] = NULL; // Null-terminate the argument list
        execve(argv[0], argv, NULL);  // Use execvp to search PATH for interpreter
        exit(127);
    }

    // PARENT
    close(pipefd[1]); // Close write end in parent
    close(postfd[0]); // close read end in parent
    if(req.getMethod() == "POST")
        write(postfd[1], req.getBody().c_str(), req.getBody().length());
    close(postfd[1]); // Close write end
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK); // Set read end non-blocking 

    clientObj.cgi_running = true;
    clientObj.cgi_pid = pid; // Store CGI process ID
    clientObj.cgi_stdout_fd = pipefd[0]; // Store read end of pipe
    clientObj.cgi_buffer.clear(); // Clear any existing buffer
    clientObj.cgi_start_time = time::clock();
    clientObj.cgi_timeout_ms = route.cgiTimeout() * 1000; // Convert seconds to milliseconds
    cgi_fds[pipefd[0]] = clientObj.fd; // Map pipe read end to client fd
    struct epoll_event cev; // CGI event
    cev.events = EPOLLIN; // Monitor for read events
    cev.data.fd = pipefd[0]; // Use pipe read end as fd
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipefd[0], &cev); // Add to epoll monitoring

    return true;
}