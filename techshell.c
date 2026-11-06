// Name(s): Natalie Gates
// Description: TechShell project


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>


struct command {
    char* name;
    char* destFilepath;
    char* sourceFilepath;
    struct command* destCommand;
    char* args[];
};


// Functional Prototypes
static void display_prompt(char* cur_dir);
static char* get_input();
static void parse_input(char* input, struct command* out);
// static bool execute_command(struct ShellCommand command);


int main() // MAIN
{
    char* cur_dir = NULL;
    bool is_running = true;

    char* input = NULL;

    // repeatedly prompt the user for input
    while (is_running) {
        cur_dir = getcwd(NULL, 0);
        display_prompt(cur_dir);

        input = get_input();
        printf("Input: %s", input);

        struct command command = {};
        parse_input(input, &command);

        // is_running = execute_command(command);
    }

    free(cur_dir);
    free(input);

    exit(0);
}


static void display_prompt(char* cur_dir) {
    printf("%s$ ", cur_dir);
}

static char* get_input() {
    char* input = NULL;
    size_t size = 0;
    ssize_t chars_read = getline(&input, &size, stdin);  // use this if I end up needing error handling
    return input;
}

/*
    A function that parses through the user input.
    Consider having this function return a struct that stores vital
    information about the parsed instruction such as:
    - The command itself
    - The arguments that come after the command
        Hint: When formatting your data,
        look into execvp and how it takes in args.
    - Information about if a redirect was detected such as >, <, or |
    - Information about whether or not a new file
        needs to be created and what that filename may be.


    Some helpful functions when doing this come from string.h and stdlib.h, such as
    strtok, strcmp, strcpy, calloc, malloc, realloc, free, and more

    Be sure to consider/test for situations when a backslash is used to escape the space char
    and when quotes are used to group together various tokens.
*/
static void parse_input(char* input, struct command* out) {
    struct command cmd = {};  // initialize the command to default "zero" values

    // copy the string to avoid changing the provided string
    char* input_search_string = calloc(strlen(input) + 1, sizeof(char));
    strcpy(input_search_string, input);

    printf("Searching input string: %s\n", input);

    const char* deliminators = " <>|\"\\";  // stop when reaching any of these characters
    while (1) {  // loop until reaching the end of the original input
        char *delim_loc = strpbrk(input_search_string, deliminators);  // find the next delim char

        if (delim_loc != NULL) {
            char delim = *delim_loc;  // save what character we stopped at

            *delim_loc = '\0';  // change the delim to a null terminator to make accessing the preceding token simple
            printf("Working");
            printf("Token: %s\tDelim: %c\n", input_search_string, delim);

            printf("Still working!");
            input_search_string = delim_loc + 1;  // move to after the char we stopped at
            printf("Is it still working?");
        } else {
            // no more delim chars, so the remainder of the string is the final token
            printf("Token: %s\tDelim: None\n", input_search_string);
            break;
        }
    }

    free(input_search_string);  // free allocated memory to avoid memory leaks

    *out = cmd;
}

/*
    A function that executes the command.
    This function might take in a struct that represents the shell command.

    Be sure to consider each of the following:
    1. The execvp() function.
        This can execute commands that already exist, that is,
        you don't need to recreate the functionality of
        the commands on your computer, just the shell.
        Keep in mind that execvp takes over the current process.
    2. The fork() function.
        This can create a process for execvp to take over.
    3. cd is not a command like ls and mkdir.
        cd is a tool provided by the shell,
        so you WILL need to recreate the functionality of cd.
    4. Be sure to handle standard output redirect and standard input redirects here
        That is, there symbols: > and <.
        Pipe isn't required but could be a nice addition.
*/
// static bool execute_command(struct ShellCommand command) {
//
// }