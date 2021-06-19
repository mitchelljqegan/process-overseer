/* This header file defines all of the macros and declares all of the functions used in overseer.c. */

#ifndef __OVERSEER_FUNCTIONS_H__
#define __OVERSEER_FUNCTIONS_H__

/* Macro Definitions */

#define _GNU_SOURCE                 // ISO C89, ISO C99, POSIX.1, POSIX.2, BSD, SVID, X/Open, LFS, and GNU extensions.
#define BUSY_WAIT_SLEEP 100000      // Time in microseconds to sleep, in order to avoid busy-waiting.
#define ERROR -1                    // Typical value returned by various functions to indicate error. 
#define FALSE 0                     // Integer representation of truth-value false.
#define FILE_ARG_INDEX 0            // Index of file path to be executed within array of data received from controller.
#define HUNDRED_PERCENT 100         // One hundred percent.
#define IP_STR_LEN 15               // The string length of an IPV4 address.
#define NUM_ARGS 2                  // Expected number of command line arguments.
#define NUM_CONNS 10                // Number of pending connections the queue will hold.
#define NUM_ENDS_PIPE 2             // The number of ends in a pipe (read & write).
#define NUM_THREADS 5               // The number of request-handling threads to be created.
#define PIPE_READ 0                 // Index of read end of pipe within pipe array.
#define PIPE_WRITE 1                // Index of write end of pipe within pipe array.
#define QUARTER_SECOND_US 250000    // Quarter second in microseconds.
#define PORT_ARG_INDEX 1            // Index of overseer port within command line arguments.
#define SIGKILL_TIMEOUT 5           // The amount of time before SIGKILL is sent to a running child process which has already received SIGTERM.
#define STDOUT_STDERR 2             // Option for redir_stream() to indicate both stdout and stderr should be redirected to the provided file.
#define TIME_FACTOR 4               // Factor to represent time in half-seconds.
#define TIME_STR_LEN 28             // The string length of a timestamp.
#define TRUE 1                      // Integer representation of truth-value true.

/* Structure Definitions */

struct request // Structure describing a single controller request.
{
    struct sockaddr_in controller_addr; // Internet socket address of current request's controller.
    int new_fd;                         // File descriptor for the socket of current request.  
    struct request *next;               // Pointer to next request.
};

struct mem_entry // Structure describing a single memory report entry.
{
    pid_t proc_id;          // Process ID of entry.
    char *timestamp;        // Timestamp of entry.
    char *args;             // Process' file and arguments.
    long int mem_used;      // Process' current memory usage (bytes).                 
    struct mem_entry *next; // Pointer to next entry.          
};

/* Global Variables */

extern int num_requests;                // Number of currently pending requests.
extern int quit;                        // Indicates whether the program is to continue executing or not. 
extern pthread_cond_t got_request;      // Program condition variable.
extern pthread_mutex_t mem_mutex;       // Mutex for memory variables.
extern pthread_mutex_t quit_mutex;      // Mutex for quit variable.   
extern pthread_mutex_t request_mutex;   // Mutex for request variables.  
struct mem_entry *last_entry;           // Pointer to last report entry of linked list.
struct mem_entry *mem_report;           // Pointer to first report entry of linked list.
struct request *last_request;           // Pointer to last request of linked list. 
struct request *requests;               // Pointer to first request of linked list.

/* Function Declarations */

/*
 * Function accept_conn(): Accept incoming connections. 
 * 
 * Algorithm: As above.
 * 
 * Input: Socket file descriptor (sock_fd), controller internet address (controller_addr).
 * 
 * Output: Connection file descriptor.
 */
int accept_conn(int sock_fd, struct sockaddr_in *controller_addr);

/*
 * Function split_args(): Split up the received string of arguments.
 * 
 * Algorithm: Split the string of received arguments using the space delimiter, check the for flags and commands, and assign to the appropriate variables. 
 * 
 * Input: Buffer of received arguments (buf), file path of child output redirection file (out_file), file path of logging redirection file 
 * (log_file), time before SIGTERM is sent to child (SIGTERM_timeout), indicator of if memory information is to be sent back to the controller 
 * (show_mem_info), the ID of the process for memory information to be sent back (proc_id), indicator of if processes above a certain percentage 
 * memory usage should be killed (kill_mem_percent), percentage of memory usage used to kill processes (mem_percent), array of strings to hold 
 * executable file and its arguments (args).
 * 
 * Output: The number of arguments (not including the file) in args.
 */
int split_args(char* buf, char* out_file, char *log_file, int *SIGTERM_timeout, int *show_mem_info, int *proc_id, int *kill_mem_percent, double *mem_percent, char **args);

/*
 * Function get_mem_used(): Get the current memory usage of the specified process.
 * 
 * Algorithm: Open maps file, read each line, get memory usage from start and end address, check if inode is 0, sum memory usage of inodes which are 0.
 * 
 * Input: Process ID (c_pid).
 * 
 * Output: Memory used by process.
 */
long int get_mem_used(pid_t c_pid);

/*
 * Function get_request(): Retrieves a request from the queue.
 * 
 * Algorithm: Get the first request from the queue and set the following request to be the new head.
 * 
 * Input: None.
 * 
 * Output: First request in the queue.
 */
struct request *get_request();

/*
 * Function add_mem_entry(): Add new memory report entry for the specified process. 
 * 
 * Algorithm: Concatenate file path and its arguments, add process ID, timestamp, memory usage and the file path and args to the linked list.
 * 
 * Input: Process ID (proc_id), timestamp (timestamp), file path and its arguments (args), memory usage (mem_used).
 * 
 * Output: None.
 */
void add_mem_entry(pid_t proc_id, char* timestamp, char **args, int mem_used);

/*
 * Function add_request(): Add request to the end of the queue.
 * 
 * Algorithm: As above.
 * 
 * Input: Controller internet address (controller_addr) and connection file descriptor (new_fd). 
 * 
 * Output: None.
 */
void add_request(struct sockaddr_in controller_addr, int new_fd);

/*
 * Function clean_up_unhandled_reqs(): Clean up requests that had not yet been handled.
 * 
 * Algorithm: Free all linked list nodes.
 * 
 * Input: None.
 * 
 * Output: None.
 */
void clean_up_unhandled_reqs();

/*
 * Function delete_mem_entries(): Delete all memory report entries for the specified process. 
 * 
 * Algorithm: Traverse through the linked list, check if a node has the specified process ID and then remove if it does.
 * 
 * Input: Process ID (proc_id).
 * 
 * Output: None.
 * 
 */
void delete_mem_entries(pid_t proc_id);

/*
 * Function exec_request(): Execute the first request in the queue. 
 * 
 * Algorithm: Receive arguments from controller, split the string of arguments, if applicable send memory report back to controller, if applicable 
 * kill all process above a certain percentage of memory usage, if applicable execute and oversee the specified file and arguments.
 * 
 * Input: Controller internet address (controller_addr) and connection file descriptor (new_fd).
 * 
 * Output: None.
 */
void exec_request(struct sockaddr_in controller_addr, int new_fd);

/*
 * Function get_mem_info_all(): Get memory information of all running processes.
 * 
 * Algorithm: As above.
 * 
 * Input: IDs of currently running processes (proc_ids), memory usage of currently running processes (mem_used), file and arguments of currently 
 * running processes (proc_args).
 * 
 * Output: None.
 */
void get_mem_info_all(pid_t *proc_ids, long int *mem_used, char **proc_args);

/*
 * Function get_time(): Get the current system time. 
 * 
 * Algorithm: Retrieve the current system time and format it into a string.
 * 
 * Input: Current time string (current_time_fmt).
 * 
 * Output: None.
 */
void get_time(char *current_time_fmt);

/*
 * Function handle_SIGINT(): SIGINT handling function.
 * 
 * Algorithm: Set quit to TRUE.
 * 
 * Input: None.
 * 
 * Output: None.
 * 
 */
void handle_SIGINT();

/*
 * Function init_SIGINT_handling(): Initialise handler for SIGINT.
 * 
 * Algorithm: As above.
 * 
 * Input: None.
 * 
 * Output: None.
 */
void init_SIGINT_handling();

/*
 * Function log_args(): Prints arguments to stdout or specified redirection file.
 * 
 * Algorithm: As above.
 * 
 * Input: Number of arguments (num_args), indicator of if redirection file should be used (use_log_file), redirection file stream (log_fp), 
 * arguments to print (args).
 * 
 * Output: None.
 */
void log_args(int num_args, int use_log_file, FILE *log_fp, char **args);

/*
 * Function log_message(): Prints logging message to stdout or specified redirection file.
 * 
 * Algorithm: As above.
 * 
 * Input: Indicator of if redirection file should be used (use_log_file), redirection file stream (log_fp), message to log (message).
 * 
 * Output: None.
 */
void log_message(int use_log_file, FILE *log_fp, char *log_message);

/*
 * Function manage_child(): Manage and oversee the specified process.
 * 
 * Algorithm: Check every quarter second if the state of the process has changed, make entries in the memory report each second, send SIGTERM or 
 * SIGKILL if the process exceeds its specified timeout.
 * 
 * Input: Process ID (c_pid), time before SIGTERM is sent to child (SIGTERM_timeout), current time string (current_time), file and arguments (args), 
 * message to log (message), indicator of if redirection file should be used (use_log_file) and redirection file stream (log_fp).
 * 
 * Output: None.
 */
void manage_child(pid_t c_pid, int SIGTERM_timeout, char *current_time, char **args, char* message, int use_log_file, FILE *log_fp);

/*
 * Function init_threads(): Initialise POSIX threads.
 * 
 * Algorithm: Initialise mutexes and the condition variable, and create the threads.
 * 
 * Input: Array of thread IDs (p_threads) and pointer to function to handle controller requests (handle_requests).
 * 
 * Output: None.
 */
void init_threads(pthread_t *p_threads, void *(*handle_requests)(void *));

/*
 * Function kill_all_percent(): Kill all processes using more than the specified percentage of the system memory.
 * 
 * Algorithm: Get the total amount of usable memory using sysinfo, check if any processes are using more than the specified percentage of memory and 
 * kill all that are.
 * 
 * Input: IDs of currently running processes (proc_ids), memory usage of currently running processes (mem_used) and percentage threshold (mem_percent).
 * 
 * Output: None.
 */
void kill_all_percent(int *proc_ids, long int *mem_used, double mem_percent);

/*
 * Function listen_to(): Listen for connections on socket.
 * 
 * Algorithm: Open and bind a socket, and start listening for connections.
 * 
 * Input: Socket file descriptor (sock_fd) and overseer port number (overseer_port).
 * 
 * Output: None.
 */
void listen_to(int *sock_fd, int overseer_port);

/*
 * Function redir_stream(): Redirect stdout and stderr to the specified file.
 * 
 * Algorithm: Open the specified file, create a copy of the stdout and stderr streams and redirect to the file.
 * 
 * Input: Redirection file descriptor (out_fd), redirection file path (out_file), copy of stdout (stdout_old_fd) and copy of stderr (stderr_old_fd).
 * 
 * Output: None.
 */
void redir_stream(int *out_fd, char* out_file, int *stdout_old_fd, int *stderr_old_fd);

/*
 * Function restore_stream(): Restore stdout and stderr after redirection.
 * 
 * Algorithm: As above.
 * 
 * Input: Copy of stdout (stdout_old_fd) and copy of stderr (stderr_old_fd).
 * 
 * Output: None.
 */
void restore_stream(int stdout_old_fd, int stderr_old_fd);

/*
 * Function send_mem_info_all(): Send memory information of all running processes to controller.
 * 
 * Algorithm: As above.
 * 
 * Input: IDs of currently running processes (proc_ids), memory usage of currently running processes (mem_used), file and arguments of currently 
 * running processes (proc_args), connection file descriptor (new_fd).
 * 
 * Output: None.
 */
void send_mem_info_all(pid_t *proc_ids, long int *mem_used, char **proc_args, int new_fd);

/*
 * Function send_mem_info_id(): Send memory report for specified process ID.
 * 
 * Algorithm: As above.
 * 
 * Input: Process ID to send memory report for (proc_id), connection file descriptor (new_fd).
 * 
 * Output: None.
 */
void send_mem_info_id(pid_t proc_id, int new_fd);

/*
 * Function handle_requests(): Retrieves requests from the queue and handles them. 
 * 
 * Algorithm: While the program hasn't been instructed to terminate, get a request from the queue and execute it.
 * 
 * Input: None.
 * 
 * Output: None.
 */
void *handle_requests(void *void_var);

#endif // __OVERSEER_FUNCTIONS_H__