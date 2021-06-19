/* Include Directives */

#include <arpa/inet.h>          // Definitions for internet operations.
#include <fcntl.h>              // POSIX functions for creating, opening, rewriting, and manipulating files.
#include <pthread.h>            // Function declarations and mappings for threading interfaces and defines a number of constants used by those functions.
#include <stdio.h>              // Functions that deal with standard input and output.
#include <stdlib.h>             // Standard library definitions.
#include <unistd.h>             // Declares a number of implementation-specific functions.
#include "overseer_functions.h" // Defines all of the macros and declares all of the functions used in controller.c.

/* Global Variables */

int num_requests = 0;                   
int quit = FALSE;                  
pthread_cond_t got_request;   
pthread_mutex_t mem_mutex;      
pthread_mutex_t quit_mutex;             
pthread_mutex_t request_mutex;   
struct mem_entry *last_entry = NULL;   
struct mem_entry *mem_report = NULL;     
struct request *last_request = NULL;    
struct request *requests = NULL;       

/*
 * Function main(): Main function reponsible for calling individual functions.
 * 
 * Algorithm: Call functions to initialise signal handling, initialise threads, listen for connections, accept connections, add requests to the 
 * queue and clean up.
 * 
 * Input: Number of command line arguments (argc) and command line arguments (argv).
 * 
 * Output: Exit code.
 */
int main(int argc, char **argv)
{
    int overseer_port;                  // Overseer port number.
    int new_fd;                         // Connection file descriptor.
    int sock_fd;                        // Socket file descriptor.
    pthread_t p_threads[NUM_THREADS];   // Array of thread identifiers.
    struct sockaddr_in controller_addr; // Internet address of controller.

    if (argc != NUM_ARGS)
    {
        fprintf(stderr, "Usuage: overseer <port>\n");
        exit(EXIT_FAILURE);
    }

    init_SIGINT_handling();
    init_threads(p_threads, handle_requests);

    overseer_port = htons(atoi(argv[PORT_ARG_INDEX])); 
    listen_to(&sock_fd, overseer_port);
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    
    if (pthread_mutex_lock(&quit_mutex))
    {
        exit(EXIT_FAILURE);
    }
    
    while (!quit)
    {
        if (pthread_mutex_unlock(&quit_mutex))
        {
            exit(EXIT_FAILURE);
        }

        if ((new_fd = accept_conn(sock_fd, &controller_addr)) == ERROR)
        {
            usleep(BUSY_WAIT_SLEEP);
            continue;
        }

        add_request(controller_addr, new_fd);

        if (pthread_mutex_lock(&quit_mutex))
        {
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_mutex_unlock(&quit_mutex))
    {
        exit(EXIT_FAILURE);
    }

    if (close(sock_fd))
    {
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_broadcast(&got_request))
    {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_join(p_threads[i], NULL))
        {
            exit(EXIT_FAILURE);
        }
    }

    clean_up_unhandled_reqs();

    return EXIT_SUCCESS;
}