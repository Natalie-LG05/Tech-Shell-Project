// Name(s): Natalie Gates
// Description: TechShell project


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>


struct shell_command {
    char* name;
    char* dest_filepath;
    char* source_filepath;
    struct shell_command* dest_command;
    char* args[];
};


// Functional Prototypes
static void display_prompt(char* cur_dir);
static char* get_input();
static void parse_input(char* input, struct shell_command* out);
static void execute_command(struct shell_command* command);


int main() // MAIN
{
    char* cur_dir = NULL;

    char* input = NULL;

    // repeatedly prompt the user for input
    while (true) {
        cur_dir = getcwd(NULL, 0);
        display_prompt(cur_dir);

        input = get_input();

        struct shell_command command = {};
        parse_input(input, &command);

        execute_command(&command);
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
    if (chars_read > 0 && input[chars_read - 1] == '\n')
        input[chars_read - 1] = '\0';  // remove the trailing newline if there is one
    return input;
}

static void parse_input(char* input, struct shell_command* out) {
    struct shell_command cmd = {};  // initialize the command to default "zero" values

    // copy the string to avoid changing the provided string
    char* input_string = calloc(strlen(input) + 1, sizeof(char));
    strcpy(input_string, input);

    char* input_search_string = input_string;

    char* token = NULL;
    char* last_token = NULL;
    char last_delim = '\0';

    bool in_quotes = false;

    while (true) {
        // skip over leading spaces to get to the start of the token
        if (!in_quotes && token == NULL) input_search_string += strspn(input_search_string, " ");
        // if we reach the end of the string here, that means we had trailing spaces which should be ignored
        if (*input_search_string == '\0') {
            // do not break if token isn't NULL, as we still need to process the last token
            if (token == NULL) break;

            if (in_quotes) {
                // input ends and is missing a closing ", so give an error or something idk yet
            }
        }

        const char* deliminators = in_quotes ? "\\\"" : " <>|\"\\";
        char *delim_loc = strpbrk(input_search_string, deliminators);  // find the next delim char

        char delim = '\0';
        if (delim_loc != NULL) delim = *delim_loc;  // save what character we stopped at

        if (delim == '\\') {
            // outside quotes, if delim is a \, then remove the \, ignore the next char, and keep going to get the rest of the token
            // inside quotes, if delim is a \, then only escape the following char if it is a \ or ", otherwise treat the \ as a literal '\'
            char next_char = *(delim_loc + 1);
            if (!in_quotes || (next_char == '"' || next_char == '\\')) {
                // remove the \, ignore the next char (the escaped char), and keep going to get the rest of the token
                // shift over the rest of the string, removing the '\'
                memmove(delim_loc, delim_loc + 1, strlen(delim_loc));

                // store the start of this token in token (unless the start of a token is already being stored there)
                if (token == NULL) token = input_search_string;
            }

            last_delim = '\\';
            input_search_string = delim_loc + 1;  // continue reading the token
            continue;
        }

        if (delim == '"') {
            if (!in_quotes) {
                // if we find a " in the first token or that isn't the start of a token, treat it as a literal "
                // if the char be4 the " is \0, then this " must be the start of a token
                if (last_token == NULL || *(delim_loc - 1) != '\0') {
                    // store the start of this token in token
                    if (token == NULL) token = input_search_string;
                    last_delim = '"';
                    input_search_string = delim_loc + 1;  // continue reading the token
                } else {
                    // if delim is a " and it is not in the middle of a token (i.e. it is the start of a token),
                    // go until finding a closing " to get the token
                    in_quotes = true;  // set the parsing loop to run in quotes mode
                    input_search_string = delim_loc + 1;  // move to after the opening "
                    token = input_search_string;  // store where this token starts
                }
                continue;  // continue reading the token
            }

            // we have found the closing " without a \ before it, so everything between the "s is the token
            in_quotes = false;  // set the parsing loop to run in normal mode again
        } else {
            // if the delim is a >, set the next token as the dest_filepath
            // if the delim is a <, set the next token as the source_filepath
            // if the delim is a |, recursively call parse_input to set everything after the | as the dest_command

            // if last_token is NULL, the token is the command name so set name
            // otherwise the token will be an arg, filename, chained command, or redirect operator
                // if the token is a >, ignore it and set the next token as the dest_filepath
                // if the token is a <, ignore it and set the next token as the source_filepath
                // if the token is a |, ignore it and recursively call parse_input to set everything after the | as the dest_command
                // otherwise the token is an arg, filename, or chained command depending on the previous token and delim
        }

        // if a token isn't already set, then everything from where we started up to the delim is the token
        if (token == NULL) token = input_search_string;
        if (delim_loc != NULL) *delim_loc = '\0';  // change the delim to a null terminator to make accessing the token simple

        printf("Token: '%s' (Delim: '%c')\n", token, delim);  // process the token

        // update last_token and last_delim
        last_token = token;
        token = NULL;  // indicate that we are no longer processing a token
        last_delim = delim;

        if (delim_loc == NULL) break;  // no more delim chars, so the remainder of the string was the final token so we are done
        input_search_string = delim_loc + 1;  // move to after the char we stopped at
    }

    free(input_string);  // free allocated memory to avoid memory leaks

    *out = cmd;
}

// test if `exit` command works
static void execute_command(struct shell_command* command) {
    // fork() for a child to run execvp() in (unless the command is 'cd')

    // open necessary source and destination file streams
    // if there is a source command create a pipe to redirect the initial commands stdout into

    // create the args array holding the name of the command, the args to the command,
    // file data that was redirected into the command, and NULL to terminate the array

    // if the command is 'cd', manually perform it instead of using execvp()
    // otherwise, in the child process:-
        // if applicable, redirect stdout into the pipe or the destination file
        // call on execvp, passing in the args array

    // if there was a dest_command, the pipe will have the output of the initial command, so update the dest_command's
    // args with that output then recursively call on execute_command with the new command
}