//Simple UNIX Shell
//Implements &, >> << < > and |
//Single Pipe.
//Very complex combinations may not work.
//Dylan J. Hoban

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#define MAX_LINE 80 //maximum lenghth for command.
int status; //give result from the execvp call.
int pipeindx; //location of the pipe.
int pipe_id[2]; //pipes
char *args[MAX_LINE / 2 + 1]; //cmd line args, nopipe.



//functions
int parse(char Buffer[], int length, int*);
void redirect(char *args[], int length);
void nonpipe(int, int*);
void containspipe(int);
void intro();
int nopipe = 1; //no pipe by default.

int main(void)
{
  intro();
  int bg; //tell whether or not to run in background.
  char Buffer[MAX_LINE];
  int numargs = 1; //when to exit shell
  int nocd = 1; //nocd used.
  int i; //counter

  while (1)
  {
    int length;
    do //print out the command line.
    {
      printf("djh>");
      fflush(stdout);
      length = read(STDIN_FILENO, Buffer, MAX_LINE);
    } while (Buffer[0] == '\n'); //ignore newlines from user.

    bg = 0;
    numargs = parse(Buffer, length, &bg); //parse command arguments.
    if (strncmp(Buffer, "exit", 4) == 0)
      return 0; //exit the shell.
    if (strncmp(args[0], "cd", 2) == 0)
    {
      chdir(args[1]);
      nocd = 0; //don't do other stuff.
    }
    if (nocd)
    {
      for (i = 0; i < numargs; ++i)
      {
        if (strcmp(args[i], "|") == 0)
        {
          pipeindx = i;
          nopipe = 0; //false
        }
      }
      if (!nopipe)
        containspipe(numargs);
      if (nopipe)
        nonpipe(numargs, &bg);
    }
    nopipe = 1; //true
    nocd = 1; //true
  }
  return 0;
}

//execute commands that don't contain
//a pipe.
void nonpipe(int numargs, int *bg)
{
  if(*bg == 1)
  {
    args[numargs-1] = NULL;
    numargs -= 1;
  }
  pid_t child; //pid of child process.
  child = fork(); //fork process
  switch (child)
  {
    case -1:
      printf("Cannot fork the process");
      break;
    case 0:
      redirect(args, numargs);
      status = execvp(args[0], args);
      if (status != 0)
      {
        printf("Execvp didn't work, probably bad cmd\n");
        perror("error: ");
        exit(0);
      }
      break;
    default:
      if (*bg == 0) //not running in background
       wait(NULL);
  }

}

//execute commands that do contain a pipe.
void containspipe(int numargs)
{
  pid_t child; //pid of child process.
  pid_t child2;
  int i = 0;
  int j = 0;
  char *argsleft[MAX_LINE / 2 + 1]; //args for left of pipe
  char *argsright[MAX_LINE / 2 + 1]; //args for right of pipe.

  pipe(pipe_id);
  //parse out each side.
  for (i = 0; i < pipeindx; ++i)
    argsleft[i] = args[i];
  j = 0;
  argsleft[pipeindx] = '\0';
  int argsllen = pipeindx;
  for (i = pipeindx + 1; i < numargs; ++i)
  {
    argsright[j] = args[i];
    ++j;
  }
  int argsrlen = numargs-pipeindx-1;
  argsright[argsrlen] = '\0';
  child = fork();
  //setup children and streams.
  if(child == 0)
  {
    close(0);
    dup2(pipe_id[0], STDIN_FILENO);
    close(pipe_id[1]);
    redirect(argsright, argsrlen);
    execvp(argsright[0], argsright);
  } else
  {
    child2 = fork(); //need another child.
    if(child2 == 0)
    {
      close(1);
      dup2(pipe_id[1], STDOUT_FILENO);
      close(pipe_id[0]);
      redirect(argsleft, argsllen);
      execvp(argsleft[0], argsleft);

    }
    else
    {
      close(pipe_id[0]);
      close(pipe_id[1]);
      waitpid(child, NULL, 0); //wait for both children
      waitpid(child2, NULL, 0);
    }

  }

}

//redirects the streams as necessary from the
//user input.
void redirect(char *args[], int length)
{
  int i = 0;
  int in, out;
  for(i = 0; i<length; ++i)
  {
    if(strcmp(args[i], "<") == 0)
    {
      in = open(args[i+1], O_RDONLY);
      close(0);
      dup(in);
      close(in);
      args[i] = '\0';
    }
    else if(strcmp(args[i], ">") == 0) //new/overwrite
    {
      out = open(args[i+1], O_CREAT|O_WRONLY, 0666);
      close(1);
      dup(out);
      close(out);
      args[i] = '\0';
    }
    else if(strcmp(args[i], ">>") == 0) //append
    {
      out = open(args[i+1], O_WRONLY|O_APPEND, 0644);
      close(1);
      dup(out);
      close(out);
      args[i] = '\0';
    }
  }
}


//parses the cmds in the buffer.
//puts nulls where needed and adds the words
//since C relies on NULL termination for
//reading strings.
int parse(char Buffer[], int length, int *bg)
{
  int i, beginw, //beginw, index to beginning of word.
      parsect; //counts how many cmds have been added.


  parsect = 0; //count no. of commands parsed.
  beginw = -100; //some arbitrary number that won't be an index number.
  if (length == 0)
  {
    exit(0);
  }


  if (length < 0)
  {
    perror("Error reading the cmd!");
    exit(-1);
  }

  for (i = 0; i < length; ++i)
  {
    if(Buffer[i] == ' ' || Buffer[i] == '\t')
    {
      if(beginw !=-100)
      {
        args[parsect] = &Buffer[beginw]; //assign to args.
        ++parsect;
      }
      Buffer[i] = '\0'; //null terminator.
      beginw = -100;
    }
    else if(Buffer[i] == '\n') //should be last input
    {
      if(beginw != -100)
      {
        args[parsect] = &Buffer[beginw];
        ++parsect;
      }
      Buffer[i] = '\0';
      args[parsect] = NULL; //no more args, so put a null
    }
    else
    {
      if(beginw == -100)
        beginw = i; //arg begins here.
      if(Buffer[i] == '&')
      {
        *bg = 1;
        Buffer[i-1] = '\0';
      }
    }

  }
  return parsect;
}


void intro()
{
  //generated with xxd. ie, xxd -i "ascii art txt file".
  unsigned char image[] = {
      0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x84, 0x20, 0x20, 0xe2,
      0x96, 0x84, 0xe2, 0x96, 0x84, 0xe2, 0x96, 0x84, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x80, 0xe2, 0x96, 0x80, 0xe2, 0x96, 0x80, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0xe2, 0x96, 0x92,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x80, 0x20, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x8c, 0x20, 0x20, 0x20, 0xe2, 0x96,
      0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x93,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a,
      0xe2, 0x96, 0x91, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x8c, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x80, 0xe2, 0x96, 0x80, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x0a, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x84, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x8c, 0xe2, 0x96, 0x93, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x84, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x93, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x93, 0xe2,
      0x96, 0x88, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x93, 0x20, 0x20, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0x20, 0x20, 0xe2, 0x96,
      0x91, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0xe2, 0x96,
      0x91, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x93, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x0a, 0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x92, 0xe2, 0x96,
      0x93, 0x20, 0x20, 0xe2, 0x96, 0x92, 0x20, 0x20, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x93, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0x20,
      0x20, 0x20, 0xe2, 0x96, 0x92, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x91,
      0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x0a, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92, 0x20, 0x20,
      0xe2, 0x96, 0x92, 0x20, 0x20, 0xe2, 0x96, 0x92, 0x20, 0xe2, 0x96, 0x91,
      0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x92,
      0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0x20, 0xe2,
      0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x20, 0xe2, 0x96, 0x91, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96,
      0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x91, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x20, 0x20, 0x20, 0xe2,
      0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20,
      0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x0a, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x20, 0x20, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2,
      0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x93, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x88, 0xe2, 0x96, 0x93, 0x20, 0x20, 0x20, 0x20, 0x0a, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0x20,
      0xe2, 0x96, 0x92, 0x20, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2,
      0x96, 0x80, 0x20, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x92, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x93, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0x20, 0x20, 0x20, 0x20, 0x0a,
      0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x84, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x80, 0xe2, 0x96, 0x80, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92, 0xe2, 0x96,
      0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0x20, 0x20, 0xe2, 0x96,
      0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0x20, 0x20,
      0x20, 0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x20, 0x20, 0xe2, 0x96, 0x92,
      0x20, 0x20, 0x20, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92,
      0xe2, 0x96, 0x91, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96,
      0x91, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0x20, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x93, 0xe2, 0x96, 0x88, 0x20, 0x20, 0xe2, 0x96, 0x84, 0x20, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x91, 0x20,
      0x20, 0x20, 0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20, 0x0a, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x91, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0xe2,
      0x96, 0x91, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x93, 0xe2,
      0x96, 0x91, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2,
      0x96, 0x88, 0xe2, 0x96, 0x88, 0xe2, 0x96, 0x92, 0x0a, 0xe2, 0x96, 0x92,
      0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x93, 0xe2, 0x96, 0x92, 0x20, 0xe2,
      0x96, 0x92, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92, 0x20, 0xe2,
      0x96, 0x91, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92,
      0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x91, 0x20, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x93, 0x20, 0x20, 0xe2, 0x96,
      0x91, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92, 0xe2, 0x96, 0x91, 0xe2,
      0x96, 0x93, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x0a, 0xe2, 0x96, 0x91, 0x20,
      0xe2, 0x96, 0x91, 0xe2, 0x96, 0x92, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92, 0x20, 0xe2, 0x96, 0x91, 0xe2,
      0x96, 0x92, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96,
      0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96,
      0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x92, 0x20, 0x20, 0xe2,
      0x96, 0x91, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96,
      0x92, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x0a, 0xe2, 0x96, 0x91, 0x20, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0xe2,
      0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0xe2, 0x96, 0x91, 0x20, 0xe2,
      0x96, 0x91, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20,
      0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x20,
      0x20, 0xe2, 0x96, 0x91, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20, 0x0a,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0x20,
      0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96,
      0x91, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91,
      0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91,
      0x20, 0x20, 0x20, 0x20, 0xe2, 0x96, 0x91, 0x20, 0x20, 0xe2, 0x96, 0x91,
      0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x0a
  };
  printf("%s\n", image);
  printf("Welcome to DJH's Shell\n");
}







