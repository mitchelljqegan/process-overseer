/* Include Directives */

#include <linux/limits.h>           // Implementation-defined constants.
#include <stdlib.h>                 // Standard library definitions.
#include <unistd.h>                 // Declares a number of implementation-specific functions.
#include "controller_functions.h"   // Defines all of the macros and declares all of the functions used in controller.c.

/*
 * Function main(): Main function reponsible for calling individual functions.
 * 
 * Algorithm: Call functions to validate arguments, connect to the overseer, concatenate the arguments, send the arguments and if applicable, receive 
 * memory information.
 * 
 * Input: Number of command line arguments (argc) and command line arguments (argv).
 * 
 * Output: Exit code.
 */
int main(int argc, char *argv[])
{
    char *overseer_ip;  // Overseer IP address. 
    int overseer_port;  // Overseer port number.
    int show_mem_info;  // Indicates whether memory information was requested from the overseer.
    int sock_fd;        // Socket file descriptor.

    show_mem_info = validate_args(argc, argv);

    overseer_ip = argv[IP_ARG_INDEX];
    overseer_port = atoi(argv[PORT_ARG_INDEX]);

    sock_fd = connect_to(overseer_ip, overseer_port);

    char *args = malloc(sizeof(char) * PATH_MAX); // Arguments to be sent to overseer. 

    if (!args)
    {
        exit(EXIT_FAILURE);
    }

    concat_args(argc, args, argv);
    send_args(sock_fd, args);

    if (show_mem_info)
    {
        get_print_mem_info(sock_fd);
    }

    free(args);
    close(sock_fd);

    return EXIT_SUCCESS;
}