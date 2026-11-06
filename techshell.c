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
    while (true) {
        // skip over leading spaces to get to the start of the token
        if (token == NULL) input_search_string += strspn(input_search_string, " ");
        // if we reach the end of the string here, that means we had trailing spaces which should be ignored
        if (*input_search_string == '\0') {
            // do not break if token isn't NULL, as we still need to process the last token
            if (token == NULL) break;
        }

        char *delim_loc = strpbrk(input_search_string, " <>|\"\\");  // find the next deliminator

        char delim = '\0';
        if (delim_loc != NULL) delim = *delim_loc;  // save what character we stopped at

        // if delim is a \, then remove the \, ignore the next char, and keep going to get the rest of the token
        if (delim == '\\') {
            // shift over the rest of the string, removing the '\'
            memmove(delim_loc, delim_loc + 1, strlen(delim_loc));
            // store the start of this token in token (unless the start of a token is already being stored there)
            if (token == NULL) token = input_search_string;
            last_delim = '\\';

            input_search_string = delim_loc + 1;  // continue reading the token
        }
        else if (delim == '"') {
            // if we find a " in the first token or without a literal space before it, treat it as a literal "
            if (last_token == NULL) {
                // store the start of this token in token
                token = input_search_string;
                last_delim = '"';
                input_search_string = delim_loc + 1;  // continue reading the token
            }
            else if (*(delim_loc - 1) != '\0') {  // if the char be4 the " is \0, then this " must be the start of a token
                // store the start of this token in token (unless the start of a token is already being stored there)
                if (token == NULL) token = input_search_string;
                last_delim = '"';
                input_search_string = delim_loc + 1;  // continue reading the token
            }
            else {
                // if delim is a " and it is not in the middle of a token (i.e. it is the start of a token),
                // go until finding a closing " to get the token
                input_search_string = delim_loc + 1;  // move to after the char we stopped at

                token = input_search_string;  // store where this token starts
                while (true) {  // loop until finding the closing " in order to get this token
                    delim_loc = strpbrk(input_search_string, "\\\"");
                    if (delim_loc != NULL) {
                        delim = *delim_loc;
                        if (delim == '\\') {  // a / inside quotes is used to escape a \ or ", otherwise it is a literal '\'
                            switch (*delim_loc + 1) {
                                case '"':
                                case '\\':
                                    // remove the \, ignore the next char, and keep going to get the rest of the token
                                    memmove(delim_loc, delim_loc + 1, strlen(delim_loc));
                                    input_search_string = delim_loc + 1;
                                    break;
                                default:
                                    // if it isn't escaping a " or \, then just treat it as a literal \ and continue
                                    input_search_string = delim_loc + 1;
                            }
                        } else {
                            // we have found the closing " without a \ before it so everything between the "s is the token
                            *delim_loc = '\0';  // change the closing " to a null terminator to make accessing the token simple

                            printf("Token: '%s' (Delim: '%c')\n", token, delim);

                            // update last_token and last_delim
                            last_token = token;
                            token = NULL;
                            last_delim = delim;

                            input_search_string = delim_loc + 1;
                            break;
                        }
                    } else {
                        // input is missing a closing ", so give an error or something idk yet
                    }
                }
            }
        }
        else {
            if (token == NULL) token = input_search_string;

            // for testing, simply assume everything from where we are up until the delim is the token (may not be the case based on certain chars)
            if (delim_loc != NULL) *delim_loc = '\0';  // change the delim to a null terminator to make accessing the token simple
            printf("Token: '%s' (Delim: '%c')\n", token, delim);

            // update last_token and last_delim
            last_token = token;
            token = NULL;
            last_delim = delim;

            if (delim_loc == NULL) break;  // no more delim chars, so the remainder of the string was the final token so we are done
            input_search_string = delim_loc + 1;  // move to after the char we stopped at
        }

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