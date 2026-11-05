// Name(s): Natalie Gates
// Description: TechShell project


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>


struct ShellCommand {
    char* name;
    char* args[];
};


// Functional Prototypes
void displayPrompt(char* curDir);
char* getInput();
// struct ShellCommand parseInput(char* input);
// bool executeCommand(struct ShellCommand command);


int main() // MAIN
{
    char* curDir = NULL;
    bool isRunning = true;

    char* input = NULL;

    // repeatedly prompt the user for input
    while (isRunning) {
        curDir = getcwd(NULL, 0);
        displayPrompt(curDir);

        // input = getInput();
        // printf("%s\n", input);

        // struct ShellCommand command = parseInput(input);
        //
        // isRunning = executeCommand(command);

        break;
    }

    free(curDir);
    free(input);

    exit(0);
}


void displayPrompt(char* curDir) {
    printf("%s$ ", curDir);
}

// char* getInput() {
//     char* input = NULL;
//     getline(&input, NULL, stdin);
//     return input;
// }

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
// struct ShellCommand parseInput(char* input) {
//
// }

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
// bool executeCommand(struct ShellCommand command) {
//
// }