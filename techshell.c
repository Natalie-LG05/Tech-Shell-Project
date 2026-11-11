// Name(s): Natalie Gates
// Description: TechShell project


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>


struct shell_command {
    char* name;
    char* dest_filepath;
    char* source_filepath;
    char** args;  // first arg is the command name, and last arg must be a null terminator for execvp
    size_t num_args;  // the number of args (including the null terminator)
    struct shell_command* dest_command;
};


// Functional Prototypes
static void display_prompt(char* cur_dir);
static char* get_input();
static void parse_input(char* input, struct shell_command* out);
static void execute_command(struct shell_command command);


int main() // MAIN
{
    char* cur_dir = NULL;

    char* input = NULL;

    // repeatedly prompt the user for input
    while (true) {
        cur_dir = getcwd(NULL, 0);
        display_prompt(cur_dir);

        input = get_input();

        struct shell_command command;
        parse_input(input, &command);

        // only attempt to execute if a valid command was able to be constructed
        // parse_input will set command.name to NULL if there was an error (or if input was just a blank line)
        if (command.name != NULL) execute_command(command);

        free(command.args);
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
                // input ends and is missing a closing ", so just do nothing and it will fall through to
                // processing everything after the first " as one token
                // (effectively automatically adding a closing " to the end of the line)
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
                // if the char be4 the " is a \0 or a space, then this " must be the start of a token
                if (last_token == NULL || !(*(delim_loc - 1) == '\0' || *(delim_loc - 1) == ' ')) {
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
            // redirects will always end up as delims, never standalone tokens
            // if the delim is a >, set the next token as the dest_filepath
            // if the delim is a <, set the next token as the source_filepath
            // if the delim is a |, recursively call parse_input to set everything after the | as the dest_command
        }

        // if a token isn't already set, then everything from where we started up to the delim is the token
        if (token == NULL) token = input_search_string;
        if (delim_loc != NULL) *delim_loc = '\0';  // change the delim to a null terminator to make accessing the token simple

        // process the token
        printf("Token: '%s' (Delim: '%c')\n", token, delim);  // print for testing

        if (last_token == NULL) {
            // if last_token is NULL, the token is the command name so set name
            // also initialize args with the name as the first value and make sure it null terminates
            cmd.name = strdup(token);
            cmd.args = calloc(2, sizeof(char*));
            cmd.args[0] = strdup(token);
            cmd.args[1] = NULL;
            cmd.num_args += 2;
        } else {  // handle if the token is a filename or destination command (based on previous token and delim)
            // the token must be an argument to the command, so reallocate space for it
            char** temp_args = realloc(cmd.args, sizeof(char*) * (cmd.num_args + 1));
            if (temp_args == NULL) {
                perror("There was an error reallocating memory for args.");
                cmd.name = NULL;  // set name to null so that the caller knows an error occurred
                *out = cmd;
                return;
            }

            // then, if the allocation was successful, add the argument
            cmd.args = temp_args;
            cmd.num_args++;
            cmd.args[cmd.num_args - 2] = strdup(token);
            cmd.args[cmd.num_args - 1] = NULL;
        }

        // update last_token and last_delim
        last_token = token;
        last_delim = delim;
        token = NULL;  // indicate that we are no longer processing a token

        if (delim_loc == NULL) break;  // no more delim chars, so the remainder of the string was the final token so we are done
        input_search_string = delim_loc + 1;  // move to after the char we stopped at
    }

    free(input_string);  // free allocated memory to avoid memory leaks

    *out = cmd;
}

static void execute_command(struct shell_command command) {
    // for testing command struct
    printf("Executing the following command:\n");
    printf("\tName: %s\n", command.name);
    printf("\tDestination Filepath: %s\n", command.dest_filepath);
    printf("\tSource Filepath: %s\n", command.source_filepath);
    printf("\tArguments: ");
    int i = 0;
    while (command.args[i] != NULL) {
        printf("%s, ", command.args[i]);
        i++;
    }
    printf("\n");
    if (command.dest_command == NULL) printf("\tDestination Command: None\n");
    else {
        printf("\tDestination Command:\n");
        execute_command(*command.dest_command);
    }

    // the below comments are wrong, fix them up then begin implementation
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

    // test if `exit` command works
}