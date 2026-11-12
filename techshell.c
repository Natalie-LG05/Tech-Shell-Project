// Name(s): Natalie Gates
// Description: TechShell project


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>


struct shell_command {
    char* name;
    char* dest_filepath;
    char* source_filepath;
    char** args;  // first arg is the command name, and last arg must be a null terminator for execvp
    size_t num_args;  // the number of args (including the null terminator)
    struct shell_command* dest_command;
};


// Functional Prototypes
static void free_shell_command(struct shell_command* cmd);
static void display_prompt(char* cur_dir);
static char* get_input();
static void parse_input(char* input, struct shell_command* out);
static void print_shell_command(struct shell_command command);
static void execute_command(struct shell_command command, int src_cmd_pipe_rd_fd);


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

        bool is_exit = command.name != NULL && strcmp(command.name, "exit") == 0;

        // only attempt to execute if a valid command was able to be constructed
        // parse_input will set command.name to NULL if there was an error (or if input was just a blank line)
        // do not execute anything if the command is exit
        // if (command.name != NULL) print_shell_command(command);  // debugging line
        if (command.name != NULL && !is_exit) execute_command(command, STDIN_FILENO);

        // free any memory that was dynamically allocated (such as by calloc() or strdup()) to prevent memory leaks
        free_shell_command(&command);

        free(cur_dir);
        free(input);

        if (is_exit) break;  // exit from the loop if the command was exit (ignoring any other tokens)
    }

    exit(0);
}

static void free_shell_command(struct shell_command* cmd) {
    // free the strings
    free(cmd->name);
    free(cmd->dest_filepath);
    free(cmd->source_filepath);

    // args is an array of strings, so free each string (which are char arrays), then free the entire array
    if (cmd->args != NULL) {
        for (int i = 0; i < cmd->num_args - 1; i++) {
            free(cmd->args[i]);
        }
        free(cmd->args);
    }

    // recursively call to free dest_command (if there is one)
    if (cmd->dest_command != NULL) free_shell_command(cmd->dest_command);
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
    char* input_string = strdup(input);

    char* input_search_string = input_string;

    char* token = NULL;
    char* last_token = NULL;

    bool in_quotes = false;

    char last_redirect = '\0';
    char current_redirect = '\0';

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
        }
        else if (delim == '>' || delim == '<' || delim == '|') {
            // redirects will always end up as delims, never standalone tokens
            // when a redirect is spotted, set current_redirect to indicate that we must process it
            if (current_redirect == '\0') current_redirect = delim;
            else {
                // for simplicity sake, just give a syntax error when several redirects are put strangely next to each other
                fprintf(stderr, "Error: Invalid redirect symbol syntax\n");
                cmd.name = NULL;  // set name to null so that the caller knows an error occurred
                break;
            }
        }

        // if a token isn't already set, then everything from where we started up to the delim is the token
        if (token == NULL) token = input_search_string;

        // change the delim to a null terminator to make accessing the token simple,
        // unless we are processing a pipe redirect
        if (delim_loc != NULL && !(last_redirect == '|' && current_redirect == '|')) *delim_loc = '\0';

        // process the token
        if (strlen(token) > 0) {  // ignore empty tokens caused by putting spaces around redirects
            // printf("Token: '%s' (Delim: '%c')\n", token, delim);  // print for testing

            if (last_token == NULL) {
                // if last_token is NULL, the token is the command name so set name
                // this means that redirects must come after the command name for simplicity sake
                // also initialize args with the name as the first value and make sure it null terminates
                cmd.name = strdup(token);
                cmd.args = calloc(2, sizeof(char*));
                cmd.args[0] = strdup(token);
                cmd.args[1] = NULL;
                cmd.num_args += 2;
            }
            else if (last_redirect == '>' && current_redirect == '>') {
                // the token comes after a >, so it is a destination file
                cmd.dest_filepath = strdup(token);

                // indicate that we are done processing the last redirect
                last_redirect = '\0';
                current_redirect = '\0';
            }
            else if (last_redirect == '<' && current_redirect == '<') {
                // the token comes after a <, so it is a source file
                cmd.source_filepath = strdup(token);

                // indicate that we are done processing the last redirect
                last_redirect = '\0';
                current_redirect = '\0';
            }
            else if (last_redirect == '|' && current_redirect == '|') {
                // everything left after the | is the destination command, so assign it and break
                // must allocate dest_cmd on the heap so that its lifetime extends beyond that of this parse_input call
                struct shell_command* dest_cmd = calloc(1, sizeof(struct shell_command));
                parse_input(token, dest_cmd);  // parse everything after the | as its own command
                if (dest_cmd -> name != NULL) cmd.dest_command = dest_cmd;
                else cmd.name = NULL;  // set name to null so that the caller knows an error occurred

                break;
            }
            else {
                // the token must be an argument to the command, so reallocate space for it
                char** temp_args = realloc(cmd.args, sizeof(char*) * (cmd.num_args + 1));
                if (temp_args == NULL) {
                    perror("There was an error reallocating memory for args");
                    cmd.name = NULL;  // set name to null so that the caller knows an error occurred
                    break;
                }

                // then, if the allocation was successful, add the argument
                cmd.args = temp_args;
                cmd.num_args++;
                cmd.args[cmd.num_args - 2] = strdup(token);
                cmd.args[cmd.num_args - 1] = NULL;
            }
        }

        // update last_token
        last_token = token;
        token = NULL;  // indicate that we are no longer processing a token

        // if there is an unprocessed redirect, set last_redirect to indicate that the next token should be processed based on the redirect
        if (current_redirect != '\0') last_redirect = current_redirect;

        if (delim_loc == NULL) break;  // no more delim chars, so the remainder of the string was the final token so we are done
        input_search_string = delim_loc + 1;  // move to after the char we stopped at
    }

    free(input_string);  // free allocated memory to avoid memory leaks

    *out = cmd;
}

static void print_shell_command(struct shell_command command) {
    // for testing command struct
    printf("The provided input was parsed as the following command:\n");
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
        print_shell_command(*command.dest_command);
    }
}

static void execute_command(struct shell_command command, int src_cmd_pipe_rd_fd) {
    if (strcmp(command.name, "cd") == 0) {
        // if the command name is cd, change directories to the filepath given by the first argument (very basic cd command)
        char* dir_path = command.args[1] != NULL ? command.args[1] : "~";  // default to user's home directory if no argument was provided

        // replace any valid `~` with the path to the user's home directory
        char* home_path = getenv("HOME");
        char* new_path = NULL;
        if (strcmp(dir_path, "~") == 0) dir_path = home_path;
        else if (dir_path[0] == '~') {  // if the path starts with a ~:
            // create a new string with the ~ replaced by the path to the home directory and reassign the pointers
            new_path = calloc(strlen(dir_path) + strlen(home_path), sizeof(char));
            strcpy(new_path, home_path);
            strcat(new_path, dir_path + 1);  // start after the first char of dir_path (which is the ~ we are replacing)
            dir_path = new_path;  // reassign dir_path to point to the new string
        }

        if (chdir(dir_path) != 0) {  // NOTE: this will break if the user's HOME path environment variable is not set
            // an error occurred, so print a custom error message based on what the error was (may not cover all possible errors)
            int error = errno;
            switch (error) {
                case ENOENT:
                    fprintf(stderr, "Error 2 (No such file or directory)\n");
                    break;
                case EACCES:
                    fprintf(stderr, "Error 13 (Permission denied)\n");
                    break;
                case ENOTDIR:
                    fprintf(stderr, "Error 20 (Not a directory)\n");
                    break;
                default:
                    perror("chdir() failed");
                    break;
            }
        }
        free(new_path);

        // may need to connect command's stdout to destination command's stdin
        if (src_cmd_pipe_rd_fd != STDIN_FILENO) close(src_cmd_pipe_rd_fd);

        // since this basic cd doesn't output anything, if it is piped into something just call the dest_command
        // passing in a pipe that is never written to
        if (command.dest_command != NULL) {
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("The pipe() call failed");
                return;
            }
            close(pipe_fd[1]);  // we don't need the write end
            execute_command(*command.dest_command, pipe_fd[0]);
        }
        return;
    }

    // otherwise fork() for a child process to run execvp() in
    // if there is a destination command, create the pipe we will use to link the stdout of this
    // command to the stdin of the destination command
    bool has_dest_cmd = command.dest_command != NULL;
    int pipe_fd[2];
    if (has_dest_cmd) {
        if (pipe(pipe_fd) == -1) {
            perror("The pipe() call failed");
            return;
        }
    }

    // create the child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("There was an error executing fork()");
        return;
    }

    if (pid == 0) {
        // child process
        if (src_cmd_pipe_rd_fd != STDIN_FILENO) {
            // the output of a previous command was piped into this command
            // so redirect the data from that pipe into this command's stdin
            dup2(src_cmd_pipe_rd_fd, STDIN_FILENO);
            close(src_cmd_pipe_rd_fd);  // close the read end of the pipe now that we are done with it
        }

        // a < should override a | if both are present for a command
        if (command.source_filepath != NULL) {
            // if there is a source file, open it and connect it to the stdin of this command
            int source_fd = open(command.source_filepath, O_RDONLY);
            if (source_fd == -1) {
                perror("Error opening source file");
                exit(EXIT_FAILURE);
            }

            dup2(source_fd, STDIN_FILENO);  // does not check if dup2 failed even though it should to be safe
            close(source_fd);
        }

        if (has_dest_cmd) {
            // if there is a dest_command, redirect this command's stdout into the pipe
            close(pipe_fd[0]);  // child does not need to read from the pipe
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
        }

        // a > should override a | if both are present for a command
        if (command.dest_filepath != NULL) {
            // if there is a destination file, open it (or create it if necessary), erase it, and connect this command's stdout to it
            int dest_fd = open(command.dest_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (dest_fd == -1) {
                perror("Error opening destination file");
                exit(EXIT_FAILURE);
            }

            dup2(dest_fd, STDOUT_FILENO);
            close(dest_fd);
        }

        execvp(command.name, command.args);
        perror("An error occurred while executing a command");
        exit(EXIT_FAILURE);  // look up if I need to worry about freeing things
    }

    // if there is a destination command, call execute_command on it (giving it the read end of the pipe)
    if (has_dest_cmd) {
        close(pipe_fd[1]);  // parent does not need to write to the pipe
        execute_command(*command.dest_command, pipe_fd[0]);
    }

    // parent process
    wait(NULL);  // wait for the command to finish running
}