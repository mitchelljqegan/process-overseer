/* This source file defines all of the functions used in controller.c */

/* Include Directives */

#include <ctype.h>                  // Defines functions that are used in character classification.
#include <linux/limits.h>           // Implementation-defined constants.
#include <netdb.h>                  // Definitions for network database operations.
#include <stdio.h>                  // Functions that deal with standard input and output.
#include <stdlib.h>                 // Standard library definitions.
#include <string.h>                 // String manipulation functions.
#include <unistd.h>                 // Declares a number of implementation-specific functions.
#include "controller_functions.h"   // Defines all of the macros and declares all of the functions used in controller.c

/* Function Definitions */

int validate_args(int argc, char *argv[]) 
{
    if (argc < MIN_ARGS_HELP) 
    {
        fprintf(stderr, "Usage: controller <address> <port> {[-o out_file] [-log log_file] [-t seconds] <file> [arg...] | mem [pid] | memkill <percent>}\n");
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[IP_ARG_INDEX], "--help")) 
    {
        fprintf(stdout, "Usage: controller <address> <port> {[-o out_file] [-log log_file] [-t seconds] <file> [arg...] | mem [pid] | memkill <percent>}\n");
        exit(EXIT_SUCCESS);
    }

    /* Check the correct types for each argument and that they are in the correct order. */
    if (argc < MIN_ARGS || !is_num(argv[PORT_ARG_INDEX]) || (!strcmp(argv[FLAG_1_ARG_INDEX], "-o") && (argc < MIN_ARGS_1_FLAG ||
        !strcmp(argv[FLAG_2_ARG_INDEX], "-o") || (!strcmp(argv[FLAG_2_ARG_INDEX], "-log") && argc < MIN_ARGS_2_FLAGS))) || 
        (!strcmp(argv[FLAG_1_ARG_INDEX], "-log") && (argc < MIN_ARGS_1_FLAG || !strcmp(argv[FLAG_2_ARG_INDEX], "-o") || 
        !strcmp(argv[FLAG_2_ARG_INDEX], "-log")))) 
    {
        fprintf(stderr, "Usage: controller <address> <port> {[-o out_file] [-log log_file] [-t seconds] <file> [arg...] | mem [pid] | memkill <percent>}\n");
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[FLAG_1_ARG_INDEX], "mem"))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

int is_num(char *str) {
    for (int i = 0; i < strlen(str); i++)
    {
        if (!isdigit(str[i]))
        { 
            return FALSE;
        }
    }

    return TRUE;
}

int connect_to(char *overseer_ip, int overseer_port) 
{
    int sock_fd;                                // Socket file descriptor.
    struct hostent *he;                         // Overseer host data.
    struct sockaddr_in overseer_address = {};   // Overseer internet address.

    if ((he = gethostbyname(overseer_ip)) == NULL || (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
    {
        fprintf(stderr, "Could not connect to overseer at %s %d\n", overseer_ip, overseer_port);
        exit(EXIT_FAILURE);
    }

    overseer_address.sin_family = AF_INET; 
    overseer_address.sin_port = htons(overseer_port);
    overseer_address.sin_addr = *((struct in_addr *)he->h_addr);

    if (connect(sock_fd, (struct sockaddr *)&overseer_address, sizeof(struct sockaddr)) == ERROR)
    {
        fprintf(stderr, "Could not connect to overseer at %s %d\n", overseer_ip, overseer_port);
        exit(EXIT_FAILURE);
    }

    return sock_fd;
}

void concat_args(int argc, char *args, char **argv) 
{
    strcpy(args, argv[MIN_ARGS - 1]);

    if (argc > MIN_ARGS) 
    {
        for (int i = MIN_ARGS; i < argc; i++)
        {
            strcat(args, " ");
            strcat(args, argv[i]);
        }
    }
}

void send_args(int sock_fd, char *args) 
{
    if (send(sock_fd, args, PATH_MAX, 0) == ERROR) 
    {
        exit(EXIT_FAILURE);
    }
}

void get_print_mem_info(int sock_fd)
{
    int num_bytes; // Number of bytes read from sock_fd.

    char *buf = malloc(sizeof(char) * PATH_MAX);

    while((num_bytes = recv(sock_fd, buf, PATH_MAX, 0)) != 0)
    {
        if (num_bytes == ERROR)
        {
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "%s", buf);
    }

    free(buf);
}
