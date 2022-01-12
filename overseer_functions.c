/* This source file defines all of the functions used in overseer.c */

/* Include Directives */

#include <arpa/inet.h>          // Definitions for internet operations.
#include <errno.h>              // Defines macros for values that are used for error reporting.
#include <fcntl.h>              // POSIX functions for creating, opening, rewriting, and manipulating files.
#include <linux/limits.h>       // Implementation-defined constants.
#include <pthread.h>            // Function declarations and mappings for threading interfaces and defines a number of constants used by those functions.
#include <stdio.h>              // Functions that deal with standard input and output.
#include <stdlib.h>             // Standard library definitions.
#include <string.h>             // String manipulation functions.
#include <sys/sysinfo.h>        // Defines functions for retrieving system information.
#include <sys/wait.h>           // Declares functions for holding processes.
#include <time.h>               // Declares time and date functions.
#include <unistd.h>             // Declares a number of implementation-specific functions.
#include "overseer_functions.h" // Defines all of the macros and declares all of the functions used in controller.c.

/* Function Definitions */

int accept_conn(int sock_fd, struct sockaddr_in *controller_addr)
{
    int new_fd;                                         // Connection file descriptor.
    socklen_t addr_len = sizeof(struct sockaddr_in);    // Length of an internet address.

    if ((new_fd = accept(sock_fd, (struct sockaddr *)controller_addr, &addr_len)) == ERROR)
    {
        if (errno == EAGAIN) // If no incoming connections.
        {
            return new_fd;
        }
        else
        {
            exit(EXIT_FAILURE);
        }  
    }

    return new_fd;
}

int split_args(char *buf, char *out_file, char *log_file, int *SIGTERM_timeout, int *show_mem_info, int *proc_id, int *kill_mem_percent, double *mem_percent, char **args)
{
    char *token; // Token returned.

    token = strtok(buf, " ");

    if (!strcmp(token, "-o"))
    {
        token = strtok(NULL, " ");
        strcpy(out_file, token);

        token = strtok(NULL, " ");
            
        if (!strcmp(token, "-log"))
        {
            token = strtok(NULL, " ");
            strcpy(log_file, token);

            token = strtok(NULL, " ");

            if (!strcmp(token, "-t"))
            {
                token = strtok(NULL, " ");
                *SIGTERM_timeout = atoi(token);

                token = strtok(NULL, " ");
            }
        }
    }
    else if (!strcmp(token, "-log"))
    {
        token = strtok(NULL, " ");
        strcpy(log_file, token); 

        token = strtok(NULL, " ");

        if (!strcmp(token, "-t"))
        {
            token = strtok(NULL, " ");
            *SIGTERM_timeout = atoi(token);

            token = strtok(NULL, " ");
        }
    }
    else if (!strcmp(token, "-t"))
    {
        token = strtok(NULL, " ");
        *SIGTERM_timeout = atoi(token);

        token = strtok(NULL, " ");
    }
    else if (!strcmp(token, "mem"))
    {
        *show_mem_info = TRUE;

        token = strtok(NULL, " ");

        if (token != NULL)
        {
            *proc_id = atoi(token);
        }

        token = strtok(NULL, " ");
    }
    else if (!strcmp(token, "memkill"))
    {
        *kill_mem_percent = TRUE;

        token = strtok(NULL, " ");

        if (token != NULL)
        {
            *mem_percent = atof(token);
        }

        token = strtok(NULL, " ");
    }

    int num_args = 0; // Number of arguments.
    while (token != NULL)
    {
        args[num_args] = token;
        token = strtok(NULL, " ");
        num_args++;
    }

    return num_args;
}

long int get_mem_used(pid_t c_pid)
{
    char *token;            // Token returned.
    FILE *maps_fp;          // Maps file stream.
    int inode;              // Inode value.
    long int start_address; // Memory start address.
    long int end_address;   // Memory end address.
    long int mem_used = 0;  // Memory usage (bytes).

    char *current_line = malloc(sizeof(char) * PATH_MAX);   // Current line being read in maps file.
    char *maps_file = malloc(sizeof(char) * FILENAME_MAX);  // Maps file path.

    sprintf(maps_file, "/proc/%i/maps", c_pid);
    maps_fp = fopen(maps_file, "r");

    while (fgets(current_line, PATH_MAX, maps_fp) != NULL)
    {
        token = strtok(current_line, "-"); 
        start_address = strtol(token, NULL, 16);

        token = strtok(NULL, " ");         
        end_address = strtol(token, NULL, 16);

        for (int i = 0; i < 4; i++) 
        { 
            token = strtok(NULL, " "); 
        }

        inode = atoi(token);

        if (!inode)
        {
            mem_used += end_address - start_address;
        }
    }

    fclose(maps_fp);

    free(current_line);
    free(maps_file);

    return mem_used;
}

struct request *get_request()
{
    struct request *req; // Pointer to current request.

    if (num_requests > 0)
    {
        req = requests;
        requests = req->next;

        if (requests == NULL)
        {
            last_request = NULL;
        }

        num_requests--;
    }
    else
    {
        req = NULL;
    }

    return req;
}

void add_mem_entry(pid_t proc_id, char* timestamp, char **args, int mem_used)
{
    char *concat_args = malloc(sizeof(char) * PATH_MAX);                            // Concatenated string of file path and its arguments.
    struct mem_entry *entry = (struct mem_entry *)malloc(sizeof(struct mem_entry)); // Pointer to a new memory report entry.

    if (!entry) 
    {
        exit(EXIT_FAILURE);
    }

    entry->proc_id = proc_id;
    entry->timestamp = strdup(timestamp);

    strcpy(concat_args, args[0]);

    int i = 1;
    while (args[i] != NULL)
    {
        strcat(concat_args, " ");
        strcat(concat_args, args[i]);
        i++;
    }

    entry->args = strdup(concat_args);

    entry->mem_used = mem_used;
    entry->next = NULL;

    if (pthread_mutex_lock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    if (mem_report == NULL)
    { 
        mem_report = entry;
        last_entry = entry;
    }
    else
    {
        last_entry->next = entry;
        last_entry = entry;
    }

    if (pthread_mutex_unlock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    free(concat_args);
}

void add_request(struct sockaddr_in controller_addr, int new_fd)
{
    struct request *req = (struct request *)malloc(sizeof(struct request));

    if (!req) 
    {
        exit(EXIT_FAILURE);
    }

    req->controller_addr = controller_addr;
    req->new_fd = new_fd;
    req->next = NULL;

    if (pthread_mutex_lock(&request_mutex))
    {
        exit(EXIT_FAILURE);
    }

    if (num_requests == 0)
    { 
        requests = req;
        last_request = req;
    }
    else
    {
        last_request->next = req;
        last_request = req;
    }

    num_requests++;

    if (pthread_mutex_unlock(&request_mutex) || pthread_cond_signal(&got_request))
    {
        exit(EXIT_FAILURE);
    }
}

void clean_up_unhandled_reqs()
{
    struct request* req; // Pointer to current request.

    while (requests != NULL)
    {
       req = requests;
       requests = requests->next;

       close(req->new_fd);
       
       free(req);
    }
}

void delete_mem_entries(pid_t proc_id)
{
    struct mem_entry *entry;    // Pointer to entry in memory report.
    struct mem_entry *prev;     // Pointer to previous entry in memory report.

    if (pthread_mutex_lock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    /* Starting at first entry, until we reach an entry that doesn't match the process ID, delete all entries. */
    entry = mem_report;
    while (entry != NULL && entry->proc_id == proc_id) 
    {
        mem_report = entry->next;

        free(entry->timestamp);
        free(entry->args);
        free(entry);

        entry = mem_report;
    }
 
    /* Until we reach the last entry, delete all entries that match the process ID. */
    while (entry != NULL) 
    {
        while (entry != NULL && entry->proc_id != proc_id) 
        {
            prev = entry;
            entry = entry->next;
        }
 
        if (entry == NULL)
        {
            break;
        }
            
        prev->next = entry->next;

        if (prev->next == NULL)
        {
            last_entry = prev;

            free(entry->timestamp);
            free(entry->args);
            free(entry);
            break;
        }
        else
        {
            free(entry->timestamp);
            free(entry->args);
            free(entry);
 
            entry = prev->next;
        }
    }

    if (pthread_mutex_unlock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }
}

void exec_request(struct sockaddr_in controller_addr, int new_fd)
{   
    double mem_percent;             // Memory percentage threshold.
    int kill_mem_percent = FALSE;   // Indicator of if processes above a certain percentage memory usage should be killed.
    int num_args;                   // Number of arguments.
    int show_mem_info = FALSE;      // Indicator of if memory information is to be sent back to the controller.
    int SIGTERM_timeout = 10;       // Time before SIGTERM is sent to child.
    pid_t proc_id = 0;              // The ID of the process for memory information to be sent back.

    char **args = calloc(PATH_MAX, sizeof(char));                       // Array of strings to hold executable file path and its arguments.
    char *buf_recv = calloc(PATH_MAX, sizeof(char));                    // Buffer of received arguments.
    char *controller_ip = strdup(inet_ntoa(controller_addr.sin_addr));  // Controller's IP address.
    char *current_time = malloc(sizeof(char) * TIME_STR_LEN);           // Current time string.
    char *log_file = calloc(FILENAME_MAX, sizeof(char));                // File path of logging redirection file.
    char *out_file = calloc(FILENAME_MAX, sizeof(char));                // File path of child output redirection file.

    if (!args || !buf_recv || !controller_ip || !current_time || recv(new_fd, buf_recv, PATH_MAX, 0) == ERROR)
    {
        exit(EXIT_FAILURE);
    }

    num_args = split_args(buf_recv, out_file, log_file, &SIGTERM_timeout, &show_mem_info, &proc_id, &kill_mem_percent, &mem_percent, args);

    get_time(current_time);
    fprintf(stdout, "%s - connection received from %s\n", current_time, controller_ip);

    if (show_mem_info)
    {
        char *proc_args[NUM_THREADS];       // File and arguments of currently running processes.
        long int mem_used[NUM_THREADS];     // Memory usage of currently running processes.
        pid_t proc_ids[NUM_THREADS] = {0};  // Process IDs of currently running processes.

        if (proc_id)
        {
            send_mem_info_id(proc_id, new_fd);
        }
        else
        {
            get_mem_info_all(proc_ids, mem_used, proc_args);
            send_mem_info_all(proc_ids, mem_used, proc_args, new_fd); 
        }

        if (close(new_fd))
        {
            exit(EXIT_FAILURE);
        }
    }
    else if (kill_mem_percent)
    {
        char *proc_args[NUM_THREADS];       // File and arguments of currently running processes.
        long int mem_used[NUM_THREADS];     // Memory usage of currently running processes.
        pid_t proc_ids[NUM_THREADS] = {0};  // Process IDs of currently running processes.
        
        if (close(new_fd))
        {
            exit(EXIT_FAILURE);
        }

        get_mem_info_all(proc_ids, mem_used, proc_args);

        kill_all_percent(proc_ids, mem_used, mem_percent);
    }
    else
    {
        FILE *log_fp;                   // Logging redirection file stream.
        int pipe_fd[NUM_ENDS_PIPE];     // Pipe file descriptor.
        int use_log_file = FALSE;       // Indicator of if redirection file should be used.
        pid_t c_pid;                    // Process ID of child.

        char *message = calloc(PATH_MAX, sizeof(char)); // Message to log

        if (!log_file || !message || !out_file)
        {
            exit(EXIT_FAILURE);
        }
        
        if (close(new_fd))
        {
            exit(EXIT_FAILURE);
        }

        if (strcmp(log_file, ""))
        {
            use_log_file = TRUE;

            if ((log_fp = fopen(log_file, "a")) == NULL)
            {
                exit(EXIT_FAILURE);
            }
        }

        get_time(current_time);
        sprintf(message, "%s - attempting to execute", current_time);
        log_message(use_log_file, log_fp, message);
        log_args(num_args, use_log_file, log_fp, args);
        sprintf(message, "\n");
        log_message(use_log_file, log_fp, message);

        if (pipe(pipe_fd) || (c_pid = fork()) == ERROR)
        {
            exit(EXIT_FAILURE);
        }

        /* Child Process */
        if (!c_pid)
        {
            int out_fd;         // Child output redirection file descriptor.
            int stderr_old_fd;  // Copy of stdout.
            int stdout_old_fd;  // Copy of stderr.

            if (close(pipe_fd[PIPE_READ]) || fcntl(pipe_fd[PIPE_WRITE], F_SETFD, FD_CLOEXEC) == ERROR)
            {
                exit(EXIT_FAILURE);
            }

            if (strcmp(out_file, ""))
            {
                redir_stream(&out_fd, out_file, &stdout_old_fd, &stderr_old_fd);

                if (fcntl(out_fd, F_SETFD, FD_CLOEXEC) == ERROR)
                {
                    exit(EXIT_FAILURE);
                }
            }
                
            if(execv(args[FILE_ARG_INDEX], args) == ERROR) 
            {
                if (strcmp(out_file, ""))
                {
                    restore_stream(stdout_old_fd, stderr_old_fd);

                    if (close(out_fd))
                    {
                        exit(EXIT_FAILURE);
                    }
                }

                if (write(pipe_fd[PIPE_WRITE], "Failed", strlen("Failed") + 1) == ERROR)
                {
                    exit(EXIT_FAILURE);
                }
            }
        }
        /* Parent Process */
        else 
        {
            char err_buf[strlen("Error") + 1];  // Buffer of pipe.
            int child_exec_failed;              // Indicator that execution of child failed.

            if (close(pipe_fd[PIPE_WRITE]) || (child_exec_failed = read(pipe_fd[PIPE_READ], err_buf, strlen("Failed") + 1)) == ERROR || 
                close(pipe_fd[PIPE_READ]))
            {
                exit(EXIT_FAILURE);
            }

            if (child_exec_failed)
            {
                get_time(current_time);
                sprintf(message, "%s - could not execute", current_time);
                log_message(use_log_file, log_fp, message);
                log_args(num_args, use_log_file, log_fp, args);
                sprintf(message, "\n");
                log_message(use_log_file, log_fp, message);
            }
            else
            {
                get_time(current_time);
                sprintf(message, "%s -", current_time);
                log_message(use_log_file, log_fp, message);
                log_args(num_args, use_log_file, log_fp, args);
                sprintf(message, " has been executed with pid %i\n", c_pid);
                log_message(use_log_file, log_fp, message);

                manage_child(c_pid, SIGTERM_timeout, current_time, args, message, use_log_file, log_fp);
            }

            if (use_log_file)
            {
                if (fclose(log_fp))
                {
                    exit(EXIT_FAILURE);
                }
            }
        }
        free(message);
    }

    free(args);
    free(buf_recv);
    free(controller_ip);
    free(current_time);
    free(log_file);
    free(out_file);
}

void get_mem_info_all(pid_t *proc_ids, long int *mem_used, char **proc_args)
{
    int in_array;               // Indicator of if process ID is already in array.
    struct mem_entry* entry;    // Pointer to entry in memory report.

    if (pthread_mutex_lock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    entry = mem_report;
    while (entry != NULL) 
    {
        in_array = FALSE;

        /* Check if process ID is in array. */
        for (int i = 0; i < NUM_THREADS; i++)
        {
            if (entry->proc_id == proc_ids[i])
            {
                in_array = TRUE;
                break;
            }
        }

        /* If not, add to array. */
        if (!in_array)
        {
            for (int i = 0; i < NUM_THREADS; i++)
            {
                if (!proc_ids[i])
                {
                    proc_ids[i] = entry->proc_id;
                    break;
                }
            }
        }

        /* Update memory used and add file and arguments if they aren't already present. */
        for (int i = 0; i < NUM_THREADS; i++)
        {
            if (proc_ids[i] == entry->proc_id)
            {
                mem_used[i] = entry->mem_used;

                if (proc_args[i] == NULL)
                {
                    proc_args[i] = entry->args;
                }
            }
        }

        entry = entry->next;
    }

    if (pthread_mutex_unlock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }
}

void get_time(char *current_time_fmt)
{
    struct tm current_time; // Current system time.
    time_t raw_time;        // Raw system time.

    if ((raw_time = time(NULL)) == ERROR)
    {
        exit(EXIT_FAILURE);
    }

    current_time = *localtime(&raw_time);

    sprintf(current_time_fmt, "%d-%02d-%02d %02d:%02d:%02d", current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday, 
            current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
}

void handle_SIGINT()
{
    if (pthread_mutex_lock(&quit_mutex))
    {
        exit(EXIT_FAILURE);
    }

    quit = TRUE;

    if (pthread_mutex_unlock(&quit_mutex))
    {
        exit(EXIT_FAILURE);
    }
}

void init_SIGINT_handling()
{
    struct sigaction SIGINT_action; // Action to take for SIGINT.

    SIGINT_action.sa_handler = handle_SIGINT;

    if (sigemptyset(&SIGINT_action.sa_mask))
    {
        exit(EXIT_FAILURE);
    }

    SIGINT_action.sa_flags = 0;

    if (sigaction(SIGINT, &SIGINT_action, NULL))
    {
        exit(EXIT_FAILURE);
    }
}

void init_threads(pthread_t *p_threads, void *(*handle_requests)(void *))
{
    if (pthread_mutex_init(&request_mutex, NULL) || pthread_mutex_init(&quit_mutex, NULL) || pthread_mutex_init(&mem_mutex, NULL) || 
    pthread_cond_init(&got_request, NULL))
    {
        exit(EXIT_FAILURE);
    }
        
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&p_threads[i], NULL, handle_requests, NULL))
        {
            exit(EXIT_FAILURE);
        }
    }
}

void kill_all_percent(int *proc_ids, long int *mem_used, double mem_percent) 
{
    struct sysinfo info; // System information.

    sysinfo(&info);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (proc_ids[i])
        {
            if (mem_used[i] >= (mem_percent/HUNDRED_PERCENT) * info.totalram)
            {                    
                kill(proc_ids[i], SIGKILL);
            }
        }
    }
}

void listen_to(int *sock_fd, int overseer_port)
{
    int opt_enable = TRUE;                  // Value of SO_REUSEADDR.
    struct sockaddr_in overseer_addr = {};  // Overseer internet address.

    if ((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR || setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable)) ||
         setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt_enable, sizeof(opt_enable)))
    {
        exit(EXIT_FAILURE);
    }

    overseer_addr.sin_family = AF_INET;        
    overseer_addr.sin_port = overseer_port; 
    overseer_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(*sock_fd, (struct sockaddr *)&overseer_addr, sizeof(struct sockaddr)) || listen(*sock_fd, NUM_CONNS))
    {
        exit(EXIT_FAILURE);
    }
}

void log_args(int num_args, int use_log_file, FILE *log_fp, char **args)
{
    for (int i = 0; i < num_args; i++) 
    {
        if (use_log_file)
        {
            fprintf(log_fp, " %s", args[i]);
        }
        else
        {
            fprintf(stdout, " %s", args[i]);
        }
    }
}

void log_message(int use_log_file, FILE *log_fp, char *message)
{
    if (use_log_file)
    {
        fprintf(log_fp, "%s", message);
    }
    else
    {
        fprintf(stdout, "%s", message); 
    }
}

void manage_child(pid_t c_pid, int SIGTERM_timeout, char *current_time, char **args, char* message, int use_log_file, FILE *log_fp) 
{
    int exec_time = 0;          // Time (in terms of quarter-seconds) that the prcoess has been running for.
    long int mem_used;          // Current memory usage of the process.
    int SIGTERM_sent = FALSE;   // Indicator that SIGTERM has been sent.
    int SIGKILL_sent = FALSE;   // Indicator that SIGKILL has been sent.
    int status;                 // Status of the process.
    pid_t state_changed;        // Indicator that state of process has changed.

    while(TRUE) 
    {
        if ((state_changed = waitpid(c_pid, &status, WNOHANG)) == ERROR)
        {
            exit(EXIT_FAILURE);
        }

        /* If the child is running and SIGTERM has not yet been sent. */
        if (!state_changed && !SIGTERM_sent)
        {
            /* If the overseer has been instructed to terminate and SIGKILL has not yet been sent. */
            if (quit && !SIGKILL_sent)
            {
                if (kill(c_pid, SIGKILL))
                {
                    exit(EXIT_FAILURE);
                }

                SIGKILL_sent = TRUE;
            }
            /* If it isn't time yet to send SIGTERM. */
            else if (exec_time < SIGTERM_timeout * TIME_FACTOR) 
            {
                exec_time++;

                /* If a second has passed. */
                if (!(exec_time % TIME_FACTOR))
                {
                    mem_used = get_mem_used(c_pid);

                    get_time(current_time);

                    add_mem_entry(c_pid, current_time, args, mem_used);
                }

                usleep(QUARTER_SECOND_US);
            }
            else 
            {
                if (kill(c_pid, SIGTERM))
                {
                    exit(EXIT_FAILURE);
                }

                SIGTERM_sent = TRUE;

                get_time(current_time);
                sprintf(message, "%s - sent SIGTERM to %i\n", current_time, c_pid);
                log_message(use_log_file, log_fp, message);
            }
        }
        else
        {
            /* If process terminated normally. */
            if (WIFEXITED(status))
            {
                delete_mem_entries(c_pid);

                status = WEXITSTATUS(status);

                get_time(current_time);
                sprintf(message, "%s - %i has terminated with status code %i\n", current_time, c_pid, status);
                log_message(use_log_file, log_fp, message);

                break;
            }
            /* If process received a signal. */
            else if (WIFSIGNALED(status))
            {
                /* If the process was terminated by SIGTERM, SIGKILL or SIGINT. */
                if (WTERMSIG(status) == SIGTERM || WTERMSIG(status) == SIGKILL || WTERMSIG(status) == SIGINT)
                {
                    delete_mem_entries(c_pid);

                    status = WEXITSTATUS(status);

                    get_time(current_time);
                    sprintf(message, "%s - %i has terminated with status code %i\n", current_time, c_pid, status);
                    log_message(use_log_file, log_fp, message);

                    break;
                }
                /* If the overseer has been instructed to terminate and SIGKILL has not yet been sent. */
                else if (quit && !SIGKILL_sent)
                {
                    if (kill(c_pid, SIGKILL))
                    {
                        exit(EXIT_FAILURE);
                    }

                    SIGKILL_sent = TRUE;
                }
                /* If it isn't time yet to send SIGKILL. */
                else if (exec_time < (SIGTERM_timeout + SIGKILL_TIMEOUT) * TIME_FACTOR) 
                {
                    exec_time++;

                    usleep(QUARTER_SECOND_US);
                }
                /* If SIGKILL hasn't been sent yet. */
                else if (!SIGKILL_sent) 
                {
                    if (kill(c_pid, SIGKILL))
                    {
                        exit(EXIT_FAILURE);
                    }

                    SIGKILL_sent = TRUE;

                    get_time(current_time);
                    sprintf(message, "%s - sent SIGKILL to %i\n", current_time, c_pid);
                    log_message(use_log_file, log_fp, message);
                }
            }
        }
    }
}

void redir_stream(int *out_fd, char* out_file, int *stdout_old_fd, int *stderr_old_fd)
{
    if ((*out_fd = open(out_file, O_APPEND | O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO)) == ERROR) 
    {
        exit(EXIT_FAILURE);
    }

    *stdout_old_fd = dup(STDOUT_FILENO);
    dup2(*out_fd, STDOUT_FILENO);

    *stderr_old_fd = dup(STDERR_FILENO);
    dup2(*out_fd, STDERR_FILENO);
}

void restore_stream(int stdout_old_fd, int stderr_old_fd)
{
    dup2(stdout_old_fd, STDOUT_FILENO);
    close(stdout_old_fd);

    dup2(stderr_old_fd, STDERR_FILENO);
    close(stderr_old_fd);
}

void send_mem_info_all(pid_t *proc_ids, long int *mem_used, char **proc_args, int new_fd)
{
    char *buf_send = calloc(PATH_MAX, sizeof(char)); // Buffer to send back to controller.

    if (!buf_send)
    {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (proc_ids[i])
        {
            sprintf(buf_send, "%i %li %s\n", proc_ids[i], mem_used[i], proc_args[i]);

            if (send(new_fd, buf_send, PATH_MAX, 0) == ERROR)
            {
                exit(EXIT_FAILURE);
            }
        }
    }

    free(buf_send);
}

void send_mem_info_id(pid_t proc_id, int new_fd)
{
    struct mem_entry* entry; // Pointer to entry in memory report. 

    char *buf_send = calloc(PATH_MAX, sizeof(char)); // Buffer to send back to controller.

    if (pthread_mutex_lock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    entry = mem_report;
    while (entry != NULL)
    {
        if(entry->proc_id == proc_id)
        {
            sprintf(buf_send, "%s %li\n", entry->timestamp, entry->mem_used);

            if (send(new_fd, buf_send, PATH_MAX, 0) == ERROR)
            {
                exit(EXIT_FAILURE);
            }
        }

        entry = entry->next;
    }

    if (pthread_mutex_unlock(&mem_mutex))
    {
        exit(EXIT_FAILURE);
    }

    free(buf_send);
}

void *handle_requests(void *void_var)
{
    struct request *req; // Current request.

    if (pthread_mutex_lock(&request_mutex) || pthread_mutex_lock(&quit_mutex))
    {
        exit(EXIT_FAILURE);
    }

    while (!quit)
    {
        if (pthread_mutex_unlock(&quit_mutex))
        {
            exit(EXIT_FAILURE);
        }

        if (num_requests > 0)
        {
            req = get_request();

            if (req)
            {
                if (pthread_mutex_unlock(&request_mutex))
                {
                    exit(EXIT_FAILURE);
                }

                exec_request(req->controller_addr, req->new_fd);

                free(req);

                if (pthread_mutex_lock(&request_mutex))
                {
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (pthread_cond_wait(&got_request, &request_mutex) || pthread_mutex_lock(&quit_mutex))
        {
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_mutex_unlock(&quit_mutex) || pthread_mutex_unlock(&request_mutex))
    {
        exit(EXIT_FAILURE);
    }

    return NULL;
}
