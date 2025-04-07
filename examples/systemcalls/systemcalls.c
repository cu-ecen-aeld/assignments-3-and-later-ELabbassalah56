#include "systemcalls.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * @param const char *command
 * @return true if the path is obsulte else false
 */

bool is_absolute_path(const char *command)
{
    return command[0] == '/';
}
/*
 * @param void no need any inputs
 * @return void
 * this function to handle almost all needed system programming signals
 */
void InitializeSignalHandling()
{
    openlog("syscalls", LOG_PID | LOG_CONS, LOG_USER);
    sigset_t sigSet;
    bool success = true;

    // Initialize the signal set
    success = success && (0 == sigemptyset(&sigSet));

    // Block SIGCHLD
    success = success && (0 == sigaddset(&sigSet, SIGCHLD));

    // Block SIGINT
    success = success && (0 == sigaddset(&sigSet, SIGINT));

    // Block SIGQUIT
    success = success && (0 == sigaddset(&sigSet, SIGQUIT));

    // Apply the signal mask: block SIGCHLD, SIGINT, and SIGQUIT
    success = success && (0 == sigprocmask(SIG_BLOCK, &sigSet, NULL));

    if (!success)
    {
        syslog(LOG_ERR, "InitializeSignalHandling failed, sigprocmask returned error: %s", strerror(errno));
    }
    else
    {
        syslog(LOG_INFO, "Signal handling initialized successfully.");
    }

    // Ignore SIGINT and SIGQUIT signals
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    closelog();
}
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */

bool do_system(const char *cmd)
{
    InitializeSignalHandling();
    openlog("syscalls", LOG_PID | LOG_CONS, LOG_USER);
    bool is_sucess = true;
    int ret = system(cmd);

    if (ret == -1)
    {
        // Case 1: Child process could not be created, or status could not be retrieved
        is_sucess = false;
        syslog(LOG_ERR, "Unable to create child process or retrieve status. errno = %s\n", strerror(errno));
    }
    else if (ret == 127)
    {
        // Case 2: Shell could not be executed in the child process
        is_sucess = false;
        syslog(LOG_ERR, "Shell could not be executed. The command may not be found.\n");
    }
    else
    {
        // Case 3: Shell executed successfully, now interpret the exit status
        int status = WEXITSTATUS(ret);
        if (WIFEXITED(ret))
        {
            syslog(LOG_INFO, "Child process exited normally with status: %d\n", status);
        }
        else if (WIFSIGNALED(ret))
        {
            int signal = WTERMSIG(ret);
            syslog(LOG_INFO, "Child process terminated by signal %d\n", signal);
        }
        else if (WIFSTOPPED(ret))
        {
            int signal = WSTOPSIG(ret);
            syslog(LOG_INFO, "Child process stopped by signal %d\n", signal);
        }
    }
    closelog();
    return is_sucess;
}

/**
 * @param count -The numbers of variables passed to the function. The variables are command to execute.
 *   followed by arguments to pass to the command
 *   Since exec() does not perform path expansion, the command to execute needs
 *   to be an absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in execv()
 * @return true if the command @param ... with arguments @param arguments were executed successfully
 *   using the execv() call, false if an error occurred, either in invocation of the
 *   fork, waitpid, or execv() command, or if a non-zero return value was returned
 *   by the command issued in @param arguments with the specified arguments.
 */

bool do_exec(int count, ...)
{
    InitializeSignalHandling();
    openlog("syscalls", LOG_PID | LOG_CONS, LOG_USER);
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int _status;
    for (int i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    if (!is_absolute_path(command[0]))
    {   
        syslog(LOG_ERR, "Command path not obsolute path:  %s", command[0]);
        va_end(args);
        closelog();
        return false;
    }
    pid_t _pid = fork();
    if (0 > _pid)
    {
        syslog(LOG_ERR, "Unable to create child process and fork return -1 with errno : %s", strerror(errno));
        va_end(args);
        closelog();
        return false;
    }
    else if (0 == _pid)
    {
        syslog(LOG_INFO, "Success to create child process and fork return 0 with pid : %d started with group: %d", getpid(), getpgrp());
        _status = execv(command[0], command);
        if (-1 == _status)
        {
            syslog(LOG_ERR, "Unable to execute the command path return -1 from execv with errno %s", strerror(errno));
            va_end(args);
            closelog();
            exit(_status);
        }
        else
        {
            syslog(LOG_INFO, "Success to execute the child process and execv return 0");
        }
    }
    else
    {
        // Parent process: wait for the child to finish
        syslog(LOG_INFO, "Parent process waiting for child with pid %d to finish", _pid);
        if (waitpid(_pid, &_status, 0) == -1)
        {
            syslog(LOG_ERR, "Failed to wait for child process with pid %d: %s", _pid, strerror(errno));
            va_end(args);
            closelog();
            return false;
        }
        else if (WIFEXITED(_status))
        {
            syslog(LOG_INFO, "Child with pid %d exited with status %d", _pid, WEXITSTATUS(_status));
        }
        else if (WIFSIGNALED(_status))
        {
            syslog(LOG_ERR, "Child with pid %d terminated by signal %d", _pid, WTERMSIG(_status));
            va_end(args);
            closelog();
            return false;
        }
        else if (WIFSTOPPED(_status))
        {
            syslog(LOG_ERR, "Child with pid %d stopped by signal %d", _pid, WSTOPSIG(_status));
            va_end(args);
            closelog();
            return false;
        }
        else if (WIFCONTINUED(_status))
        {
            syslog(LOG_INFO, "Child with pid %d resumed", _pid);
        }
    }

    va_end(args);
    closelog();
    return true;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    InitializeSignalHandling();
    openlog("syscalls", LOG_PID | LOG_CONS, LOG_USER);
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int _status;
    for (int i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    if (!is_absolute_path(command[0]))
    {
        syslog(LOG_ERR, "Command path not obsolute path:  %s", command[0]);
        va_end(args);
        closelog();
        return false;
    }
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Unable to Open File Discreptor and open return -1 with errno : %s", strerror(errno));
        va_end(args);
        closelog();
        return false;
    }
    else
    {
        pid_t _pid = fork();
        if (0 > _pid)
        {
            syslog(LOG_ERR, "Unable to create child process and fork return -1 with errno : %s", strerror(errno));
            va_end(args);
            close(fd);
            closelog();
            return false;
        }
        else if (0 == _pid)
        {
            syslog(LOG_INFO, "Success to create child process and fork return 0 with pid : %d started with group: %d", getpid(), getpgrp());

            if (dup2(fd, 1) < 0)
            {
                syslog(LOG_ERR, "Unable to redirec the file descriptor and dup2 return -1 with errno : %s", strerror(errno));
                va_end(args);
                close(fd);
                closelog();
                exit(EXIT_FAILURE);
            }
            else
            {

                _status = execv(command[0], command);
                if (-1 == _status)
                {
                    syslog(LOG_ERR, "Unable to execute the command path return -1 from execv with errno %s", strerror(errno));
                    va_end(args);
                    close(fd);
                    closelog();
                    exit(EXIT_FAILURE);
                }
                else
                {
                    syslog(LOG_INFO, "Success to execute the child process and execv return 0");
                }
            }
        }
        else
        {
            // Parent process: wait for the child to finish
            syslog(LOG_INFO, "Parent process waiting for child with pid %d to finish", _pid);
            if (waitpid(_pid, &_status, 0) == -1)
            {
                syslog(LOG_ERR, "Failed to wait for child process with pid %d: %s", _pid, strerror(errno));
                va_end(args);
                close(fd);
                closelog();
                return false;
            }
            else if (WIFEXITED(_status))
            {
                syslog(LOG_INFO, "Child with pid %d exited with status %d", _pid, WEXITSTATUS(_status));
            }
            else if (WIFSIGNALED(_status))
            {
                syslog(LOG_ERR, "Child with pid %d terminated by signal %d", _pid, WTERMSIG(_status));
                va_end(args);
                close(fd);
                closelog();
                return false;
            }
            else if (WIFSTOPPED(_status))
            {
                syslog(LOG_ERR, "Child with pid %d stopped by signal %d", _pid, WSTOPSIG(_status));
                va_end(args);
                close(fd);
                closelog();
                return false;
            }
            else if (WIFCONTINUED(_status))
            {
                syslog(LOG_INFO, "Child with pid %d resumed", _pid);
            }
        }
    }
    va_end(args);
    closelog();
    close(fd);
    return true;
}
