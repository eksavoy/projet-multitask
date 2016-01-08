#include "main.h"

int main(int argc, char const *argv[])
{
	shell_loop();
	return 0;
}

void shell_loop(){
	char *line;
  	char **args;
  	int status;

  	do{
  		printf(">");
  		line = read_line();
  		args = split_line(line);
  		status = execute_line(args);

  		free(line);
  		free(args);
  	}while(status);
}

char* read_line(){
	char *line = NULL;
	ssize_t bufsize = 0; // have getline allocate a buffer for us
	getline(&line, &bufsize, stdin);
	return line;
}

char** split_line(char* line){
	int bufsize = SHELL_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "Custom shell: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, SHELL_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
		  bufsize += SHELL_TOK_BUFSIZE;
		  tokens = realloc(tokens, bufsize * sizeof(char*));
		  if (!tokens) {
		    fprintf(stderr, "Custom shell: allocation error\n");
		    exit(EXIT_FAILURE);
		  }
		}

		token = strtok(NULL, SHELL_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}


int command_lunch(char** args){
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
		  perror("Custom shell");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("Custom shell");
	} else {
		// Parent process
		do {
		  wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}