#include "commands.h"

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &cd,
  &help,
  &custom_exit
};

int custom_shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int cd(char **args)
{
  char* arg;
  if (args[1] == NULL) {
    struct passwd *pw = getpwuid(getuid());
    arg = pw->pw_dir;
  
  }else if(strcmp(args[1], "~") == 0){
    struct passwd *pw = getpwuid(getuid());
    arg = pw->pw_dir;
  } else {
    arg = args[1];
  }
  if (chdir(arg) != 0) {
    perror("Custom SHELL");
  }
  return 1;
}

int help(char **args)
{
  int i;
  printf("MultiTask shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 1; i < custom_shell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int custom_exit(char **args)
{
  return 0;
}

int execute_line(char** args){
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < custom_shell_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return command_lunch(args);
}
