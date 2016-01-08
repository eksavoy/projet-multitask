#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

#include <sys/types.h>
#include <pwd.h>

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"


void shell_loop();
char* read_line();
char** split_line();
int command_lunch(char** args);
int execute_line(char **args);