#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>

#define MAXARGS 10

void errExit(const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    fprintf(stderr, " [ %s ] \n", strerror(errno));
    va_end(argList);

    exit(EXIT_FAILURE);
}

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or >
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int mode;          // the mode to open the file with
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};
int execPipeCmd(struct pipecmd* cmd);
int fork1(void);  // Fork but exits on failure.
bool isBackProc(char* buff);
void addBackProc(pid_t* pidList,pid_t pid);
bool isPidStore(pid_t* pidList, int size, pid_t pid);
void foregroundHandler(int signum);
struct cmd *parsecmd(char*);
pid_t* listPid;
pid_t procForeground;
int sizePidList = 0;
// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(0);

  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(0);
    execvp(ecmd->argv[0],ecmd->argv);
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    int fd = open(rcmd->file, rcmd->mode, S_IRWXU);
    dup2(fd, rcmd->fd);
    runcmd(rcmd->cmd);
    close(fd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    execPipeCmd(pcmd);
    // Your code here ...
    break;
  }
  exit(0);
}

int execPipeCmd(struct pipecmd* cmd)
{
    int filedes[2];

    if (pipe(filedes) == -1)
        errExit("pipe");                     /* Create the pipe */

    switch (fork()) {
        case -1:
            errExit("fork");                 /* Create a child process */

        case 0: /* Child LS exec !*/
            if (close(filedes[0]) == -1)     /* Close unused write end */
                errExit("close");

            dup2(filedes[1], STDOUT_FILENO);
            runcmd(cmd->left);

            if(close(filedes[1]) == -1)
                errExit("close");
            break;

        default: /* Parent */

            if(close(filedes[1]) == -1)
              errExit("close");
            dup2(filedes[0],STDIN_FILENO);
            runcmd(cmd->right);
            break;
    }
    return EXIT_SUCCESS;
}
int getcmd(char *buf, int nbuf)
{
  printf("$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

bool isBackProc(char* buf){
  if (buf[strlen(buf) - 2] == '&'){
    return true;
  }
  return false;
}

void addBackProc(pid_t* pidList,pid_t pid){
  listPid = realloc(pidList, (sizePidList+1)*sizeof(pid_t));
  listPid[sizePidList] = pid;
  sizePidList++;
}

bool isPidStore(pid_t* pidList, int size, pid_t pid){
  if (size == 0)
    return false;
  int i = 0;
  for(i; i < size; ++i)
  {
    if(pidList[i] == pid)
      return true;
  }
  return false;
}

void backgroundHandler(int siganl) {
  pid_t pid;
  if(siganl == SIGHUP) {
      int i = 0;
      for(i; i<sizePidList; i++) {
        kill(listPid[i], SIGHUP);
      }
  } else {
    while ( (pid = waitpid(-1, NULL, WNOHANG)) > 0) {
      if(isPidStore(listPid,sizePidList,pid)) {
        printf("A background process has terminated: %ld\n\n", (long)pid);
      }
      continue;
    }
  }
}
void foregroundHandler(int signum) {
 if(signum == SIGINT)
  kill(procForeground, SIGINT);
}
void killAllHandler(int signum) {
  if(signum == SIGHUP) {
    backgroundHandler(SIGHUP);
    foregroundHandler(SIGINT);
    kill(getpid(), SIGHUP);
  }
}

int main(void)
{
  signal(SIGCHLD, backgroundHandler);
  signal(SIGINT, foregroundHandler);
  signal(SIGHUP, killAllHandler);
  // Read and run input commands.
  char buf[100];
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
    } else if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't') {
      return EXIT_SUCCESS;
    }else if (isBackProc(buf)){
      buf[strlen(buf) - 2] = 0;
      int pid = fork1();
      if(pid == 0) {
        addBackProc(listPid, getpid());
      } else {
        addBackProc(listPid,pid);
      }
      if(pid == 0){
        setpgrp();
        runcmd(parsecmd(buf));
      }
    } else {
        //buf[strlen(buf) - 1] = 0;
        int pid = fork1();
        if(pid == 0){
          runcmd(parsecmd(buf));
        }
        procForeground = pid;
        wait(&pid);
    }
  }

  return 0;
}

int fork1(void)
{
  int pid;
  pid = fork();

  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd* execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd* redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd* pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

// ps : pointer to a string
// es : pointer to the end of the string
// moves *ps to the next character, returns true if this character is in the
// string toks
int peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char *mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd* parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd* parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd* parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd* parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
