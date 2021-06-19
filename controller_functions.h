/* This header file defines all of the macros and declares all of the functions used in controller.c */

#ifndef __CONTROLLER_FUNCTIONS_H__
#define __CONTROLLER_FUNCTIONS_H__

/* Macro Definitions */

#define ERROR -1            // Typical value returned by various functions to indicate error. 
#define FALSE 0             // Integer representation of truth-value false.
#define FLAG_1_ARG_INDEX 3  // Index of first flag within command line arguments.
#define FLAG_2_ARG_INDEX 5  // Index of second flag within command line arguments. 
#define IP_ARG_INDEX 1      // Index of overseer IP adress within command line arguments.
#define MIN_ARGS 4          // Absolute minimum number of arguments required for correct usage. 
#define MIN_ARGS_1_FLAG 6   // Minimum number of arguments required for correct usage when using one flag.
#define MIN_ARGS_2_FLAGS 8  // Minimum number of arguments required for correct usage when using two flags.
#define MIN_ARGS_HELP 2     // Minimum number of arguments required to receive usage message.
#define PORT_ARG_INDEX 2    // Index of overseer port within command line arguments.
#define TRUE 1              // Integer representation of truth-value true.

/* Function Declarations */

/*
 * Function connect_to(): Initialises connection to overseer.
 * 
 * Algorithm: Get host information from IP address, open socket file descriptor, open connection over socket. 
 * 
 * Input: Overseer IP address (overseer_ip) and overseer port (overseer_port).
 * 
 * Output: Socket file descriptor.
 */
int connect_to(char *overseer_ip, int overseer_port);

/*
 * Function is_num(): Checks if string is a number.
 * 
 * Algorithm: Iterate through string and check if each character is a digit.
 * 
 * Input: String to check (str).
 * 
 * Output: Indication of whether string is a number or not.
 */
int is_num(char *str);

/*
 * Function validate_args(): Validates the provided command line arguments.
 * 
 * Algorithm: Check if enough arguments have been provided, check if the help flag has been used, check the correct types for each argument and that 
 * they are in the correct order, check if the mem command has been used.
 * 
 * Input: Number of command line arguments (argc) and command line arguments (argv).
 * 
 * Output: Indication of whether memory information was requested from the overseer.
 */
int validate_args(int argc, char *argv[]);

/*
 * Function concat_args(): Concatenates command line arguments.
 * 
 * Algorithm: Copy first argument into args, then append the remaining arguments to args.
 * 
 * Input: Number of command line arguments (argc), char array to hold string of arguments (args) and command line arguments (argv).
 * 
 * Output: None.
 */
void concat_args(int argc, char *args, char **argv);

/*
 * Function get_print_mem_info(): Receives and prints memory information from overseer.
 * 
 * Algorithm: As above.
 * 
 * Input: Socket file descriptor (sock_fd).
 * 
 * Output: None.
 */
void get_print_mem_info(int sock_fd);


/*
 * Function send_args(): Send arguments to socket file descriptor.
 * 
 * Algorithm: As above.
 * 
 * Input: Socket file descriptor (sock_fd) and arguments to send (args).
 * 
 * Output: None.
 */
void send_args(int sock_fd, char *args);

#endif // __CONTROLLER_FUNCTIONS_H__