#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>


#define BUFFER_SIZE 1024


void execute_command(char **args, int background, int input_fd, int output_fd);
void set_env_var(char *name, char *value);
void unset_env_var(char *name);
char *replace_vars(char *line);



int main() {
    char cwd[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    char *args[BUFFER_SIZE];

    printf("xsh# ");

    while (fgets(input, BUFFER_SIZE, stdin)) {
        // Strip newline character
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Exit condition
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        // Variable replacement
        char *processed_input = replace_vars(input);
        strcpy(input, processed_input);
        free(processed_input);

        // Tokenize input
        int argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (argc == 0) {
            printf("xsh# ");
            continue;
        }

        // Handle built-in commands
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: Missing argument\n");
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
        } else if (strcmp(args[0], "pwd") == 0) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }
        } else if (strcmp(args[0], "set") == 0) {
            if (argc < 3) {
                fprintf(stderr, "set: Missing arguments\n");
            } else {
                set_env_var(args[1], args[2]);
            }
        } else if (strcmp(args[0], "unset") == 0) {
            if (argc < 2) {
                fprintf(stderr, "unset: Missing argument\n");
            } else {
                unset_env_var(args[1]);
            }
        } else {
            int background = 0;
            int input_fd = STDIN_FILENO;
            int output_fd = STDOUT_FILENO;
            
            // Check for background execution
            if (argc > 1 && strcmp(args[argc - 1], "&") == 0) {
                background = 1;
                args[--argc] = NULL;
            }

            // Check for input redirection
            for (int i = 0; i < argc; i++) {
                if (strcmp(args[i], "<") == 0) {
                    if (i + 1 < argc) {
                        input_fd = open(args[i + 1], O_RDONLY);
                        if (input_fd < 0) {
                            perror("open");
                            input_fd = STDIN_FILENO;
                        }
                        args[i] = NULL;
                        argc = i;
                    } else {
                        fprintf(stderr, "<: Missing file name\n");
                    }
                }
            }

            // Check for output redirection
            for (int i = 0; i < argc; i++) {
                if (strcmp(args[i], ">") == 0) {
                    if (i + 1 < argc) {
                        output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (output_fd < 0) {
                            perror("open");
                            output_fd = STDOUT_FILENO;
                        }
                        args[i] = NULL;
                        argc = i;
                    } else {
                        fprintf(stderr, ">: Missing file name\n");
                    }
                }
            }

            execute_command(args, background, input_fd, output_fd);
        }

        printf("xsh# ");
    }

    return 0;
}



void execute_command(char **args, int background, int input_fd, int output_fd) {
    pid_t pid = fork();

    if (pid == 0) {  // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {  // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork");
    }
}



void set_env_var(char *name, char *value) {
    if (setenv(name, value, 1) != 0) {
        perror("setenv");
    }
}



void unset_env_var(char *name) {
    if (unsetenv(name) != 0) {
        perror("unsetenv");
    }
}



char *replace_vars(char *line) {
    char *result = malloc(BUFFER_SIZE);
    memset(result, 0, BUFFER_SIZE);

    for (size_t i = 0; i < strlen(line); i++) {
        if (line[i] == '$') {
            char var_name[BUFFER_SIZE] = {0};
            size_t j = 0;
            i++;
            while (isalnum(line[i]) || line[i] == '_') {
                var_name[j++] = line[i++];
            }
            i--;

            char *var_value = getenv(var_name);
            if (var_value) {
                strcat(result, var_value);
            }
        } else {
            strncat(result, &line[i], 1);
        }
    }

    return result;
}
