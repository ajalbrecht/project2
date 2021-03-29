//
// AJ Albrecht - Project 2 - Unix Shell
// sample outputs are at the end of the code
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
// added include statments
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <fcntl.h>
 
// parse for individual parameters
char** parse(char* s) {
  static char* words[500];
  memset(words, 0, sizeof(words));
  
  char break_chars[] = " \t";
 
  int i = 0;
  char* p = strtok(s, break_chars);
  words[i++] = p;
  while (p != NULL) {
    p = strtok(NULL, break_chars);
    words[i++] = p;
  }
  return words;
}
 
int main(int argc, const char * argv[]) {  
  char input[BUFSIZ];
  char last_command[BUFSIZ];
  
  memset(input, 0, BUFSIZ * sizeof(char));
  memset(input, 0, BUFSIZ * sizeof(char));
  
  int finished = 0;

  while (finished == 0) {
    
    printf("osh> "); 
    fflush(stdout);
 
    if ((fgets(input, BUFSIZ, stdin)) == NULL) {   // or gets(input, BUFSIZ);
      fprintf(stderr, "no command entered\n"); 
      exit(1);
    }
    input[strlen(input) - 1] = '\0';          // wipe out newline at end of string
    //printf("input was: \n'%s'\n", input);
 
    // check for history (!!) command
    if (strncmp(input, "!!", 1) == 0) {
      if (strlen(last_command) == 0) {
        fprintf(stderr, "no last command to execute\n");
      }
      printf("last command was: %s\n", last_command);
    }
 
     // if exit was typed, end the program
    if (strncmp(input, "exit", 4) == 0) {   // only compare first 4 letters
      finished = 1;
    }
   
    // if nothing was typed, let the user know to try again
    else if (strlen(input) == 0) {
      printf("No command was enterd\n");
    }
 
    // the user enterd a comand, pass it to the child
    else {
      
      printf("You entered: %s\n", input);
      
      char** words; // parsed words
      int symbol = 0; // determins if extra steps need to be taken beyond just running the command
      int file_int; // stores index of first file for redirection
      int second_file_int; // stores index of second file for redirection

      // determin if the user wants to run the previous command
      if (strncmp(input, "!!", 1) == 0) { // code runs multple times in here for some reason?
        strcpy(input, last_command);
        words = parse(input);
      } else {
        strcpy(last_command, input);
        words = parse(input);
      }
      
      // any other codes special charcters in the code? ----------------------
      for (int i = 0; i < sizeof(input) && words[i] != NULL; ++i) {
        if (strncmp(words[i], ">", 1) == 0) { // output redirection
          printf("redirecting the output\n");
          words[i] = NULL;
          if (symbol == 0) { // redirection with only an output
            symbol = 1;
            file_int = i + 1;
          } else { // redirection with both an input and an output file
            symbol = 5;
            second_file_int = i + 1;
          } 
        } else if (strncmp(words[i], "<", 1) == 0) { // input redirection
          printf("redirecting the input\n");
          words[i] = NULL;
          if (symbol == 0) {
            symbol = 2;
            file_int = i + 1;
          } else {
            symbol = 6;
            second_file_int = i + 1;
          }
        } else if (strncmp(words[i], "|", 1) == 0) { // pipeing is needed
          printf("using a pipe\n");
          symbol = 3;
          words[i] = NULL;
          file_int = i + 1;
        } else if (strncmp(words[i], "&", 1) == 0) { // the parent should not wait
          printf("running concurently\n");
          symbol = 4;
          words[i] = NULL;
        }
      }
      printf("\n");
      // -----------------------------------------------------------------
  
      // create a child proces in preporatoin of a command to be enterd =========
      pid_t pid;
      pid = fork();
  
      if (pid < 0) { // 1st child error 
        printf("Fork Failed/n"); 
        return 1;
      } if (pid == 0) { // 1srt child process
        // output result of command to a file
        if (symbol == 1 || symbol == 5 || symbol == 6) {
          int input;
          if (symbol == 1 || symbol == 6) {
            input = creat(words[file_int] , 0644);
          } else {
            input = creat(words[second_file_int] , 0644);
          }          
          dup2(input, STDOUT_FILENO);
          close(input);
        } 
        // input the command from a file
        if (symbol == 2 || symbol == 5 || symbol == 6) {
          int output;
          if (symbol == 2 || symbol == 5) {
            output = open(words[file_int], O_RDONLY);
          } else {
            output = open(words[second_file_int], O_RDONLY);
          }   
          dup2(output, STDIN_FILENO);
          close(output);
        }
        
        // if needed, use a pipe, if not, just excute without one
        if (symbol == 3) { 
          pid_t pid2;
          int fd[2];

          // second child created within first
          pipe(fd);
          pid2 = fork();

          if (pid2 < 0) { // error with second child
            printf("second fork failed/n");
            return 1;
          } else if (pid2 == 0) {  // second child code
            // input the first part of the command
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            execvp(words[0], words);
            return 0;
          } else { 
            // third child created within second within first
            pid_t pid3;
            pid3 = fork();

            if (pid3 < 0) { // error with third child
              printf("third fork failed/n");
              return 1;
            } else if (pid3 ==0 ) { //third child code
              close(fd[1]);
              dup2(fd[0], STDIN_FILENO);
              close(fd[0]);

              // for some reason, I had allot of trobule creating a second parsed array from an the middle.
              // this implementation should except up to 5 parameters after the |
              // example.  ls -l | A B C D E
              execlp(words[file_int], words[file_int], words[file_int + 1], words[file_int + 2], words[file_int + 3], words[file_int + 4]);
              return 0; // close third child
            }
            return 0; // close second chld
          }
          return 0; // close first child if pipeing was needed
        } else { // else if no | is neded
          execvp(words[0], words);
          return 0; // close first child if pipeing was not needed
        }
        // ================================================================
      }
      if (symbol != 4) { // wait for the child process if & is present
        printf("wating for child or children to complete\n");
        wait(NULL);      
      }
      printf("\n");   
    }
  }
  printf("osh exited\n");
  printf("program finished\n");
  
  return 0; // close out of the main parent and end the program
}

// checklist and sample outputs -----------------------------------------
/*
1) the project runs - yes
2) uses fork, exec, and wait - yes
3) uses dupe2 and pipe - yes
4) creates a child - yes
5) can run concurently - yes
  input: ls &
  
  output:
  osh> ls &
  You entered: ls &
  running concurently

  osh> ?1????              DateServer.java  fig3-31.c  fig3-33.c  fig3-35.c     newproc-posix.c  project2              simple-shell.c
  aj.txt        fig3-30          fig3-32    fig3-34    less          newproc-win32.c  project2.c            unix_pipe.c
  aja.txt       fig3-30.c        fig3-32.c  fig3-34.c  multi-fork    phil2.c          shm-posix-consumer.c  win32-pipe-child.c
  DateClient.java  fig3-31          fig3-33    fig3-35    multi-fork.c  pid.c            shm-posix-producer.c  win32-pipe-parent.c

6) can run in backgorund - yes
  input: ls
  
  output:
  osh> ls
  You entered: ls

  wating for child or children to complete
  ?1????           DateServer.java  fig3-31.c  fig3-33.c  fig3-35.c     newproc-posix.c  project2              simple-shell.c
  aj.txt        fig3-30          fig3-32    fig3-34    less          newproc-win32.c  project2.c            unix_pipe.c
  aja.txt       fig3-30.c        fig3-32.c  fig3-34.c  multi-fork    phil2.c          shm-posix-consumer.c  win32-pipe-child.c
  DateClient.java  fig3-31          fig3-33    fig3-35    multi-fork.c  pid.c            shm-posix-producer.c  win32-pipe-parent.c

7) exits with exit or exit() - yes
  input: exit()
  
  output:
  osh> exit()
  osh exited
  program finished

8) allows multple paramters - yes
  input ls -l

  output:
  osh> ls -l
  You entered: ls -l

  wating for child or children to complete
  total 208
  -rw-r--r-- 1 osc osc   350 Mar 13 22:01 ?1????
  -rw-r--r-- 1 osc osc    33 Mar 28 21:00 aj.txt
  -rw-r--r-- 1 osc osc   371 Mar 28 21:55 aja.txt
  -rw-rw-r-- 1 osc osc   710 Jan  3  2018 DateClient.java
  -rw-rw-r-- 1 osc osc   810 Jun 18  2018 DateServer.java
  -rwxrwxr-x 1 osc osc 10248 Feb 13 00:53 fig3-30
  -rw-rw-r-- 1 osc osc   361 Jun 18  2018 fig3-30.c
  -rwxrwxr-x 1 osc osc  9816 Feb  6 16:55 fig3-31
  -rw-rw-r-- 1 osc osc   121 Jan  3  2018 fig3-31.c
  -rwxrwxr-x 1 osc osc  9880 Feb  6 16:59 fig3-32
  -rw-rw-r-- 1 osc osc   136 Jan  3  2018 fig3-32.c
  -rwxrwxr-x 1 osc osc 10288 Feb  6 13:54 fig3-33
  -rw-rw-r-- 1 osc osc   500 Jun 18  2018 fig3-33.c
  -rwxrwxr-x 1 osc osc 10344 Feb  6 16:59 fig3-34
  -rw-rw-r-- 1 osc osc   680 Jun 18  2018 fig3-34.c
  -rwxrwxr-x 1 osc osc 10416 Feb  6 17:00 fig3-35
  -rw-rw-r-- 1 osc osc   534 Jun 18  2018 fig3-35.c
  -rw-r--r-- 1 osc osc  1661 Mar 20 20:47 less
  -rwxrwxr-x 1 osc osc  8712 Jan 30  2018 multi-fork
  -rw-rw-r-- 1 osc osc   257 Jan 30  2018 multi-fork.c
  -rw-rw-r-- 1 osc osc   780 Jan 28  2018 newproc-posix.c
  -rw-rw-r-- 1 osc osc  1413 Jan  3  2018 newproc-win32.c
  -rw-rw-r-- 1 osc osc  4110 Mar 20 12:12 phil2.c
  -rw-r--r-- 1 osc osc  2976 Jun 18  2018 pid.c
  -rwxrwxr-x 1 osc osc 16912 Mar 28 22:56 project2
  -rw-rw-r-- 1 osc osc  7941 Mar 28 22:56 project2.c
  -rw-rw-r-- 1 osc osc  1115 Jun 18  2018 shm-posix-consumer.c
  -rw-rw-r-- 1 osc osc  1434 Jun 18  2018 shm-posix-producer.c
  -rw-rw-r-- 1 osc osc   707 Jun 18  2018 simple-shell.c
  -rw-rw-r-- 1 osc osc  1219 Jan  3  2018 unix_pipe.c
  -rw-rw-r-- 1 osc osc   755 Jan  3  2018 win32-pipe-child.c
  -rw-rw-r-- 1 osc osc  2236 Jan  3  2018 win32-pipe-parent.c

9) history feature - yes
  input: ls  ->  followed by second input: !!
  
  output:
  osh> !! 
  last command was: ls
  You entered: !!

  wating for child or children to complete
  ?1????           DateServer.java  fig3-31.c  fig3-33.c  fig3-35.c     newproc-posix.c  project2              simple-shell.c
  aj.txt        fig3-30          fig3-32    fig3-34    less          newproc-win32.c  project2.c            unix_pipe.c
  aja.txt       fig3-30.c        fig3-32.c  fig3-34.c  multi-fork    phil2.c          shm-posix-consumer.c  win32-pipe-child.c
  DateClient.java  fig3-31          fig3-33    fig3-35    multi-fork.c  pid.c            shm-posix-producer.c  win32-pipe-parent.c

10) input and output redirectoin - yes
10.1) input only
  input: ls > aj.txt

  output: 
  osh> ls > aj.txt
  You entered: ls > aj.txt
  redirecting the output

  wating for child or children to complete

  output file:
  �1���
  aj.txt
  aja.txt
  DateClient.java
  DateServer.java
  fig3-30
  fig3-30.c
  fig3-31
  fig3-31.c
  fig3-32
  fig3-32.c
  fig3-33
  fig3-33.c
  fig3-34
  fig3-34.c
  fig3-35
  fig3-35.c
  less
  multi-fork
  multi-fork.c
  newproc-posix.c
  newproc-win32.c
  phil2.c
  pid.c
  project2
  project2.c
  shm-posix-consumer.c
  shm-posix-producer.c
  simple-shell.c
  unix_pipe.c
  win32-pipe-child.c
  win32-pipe-parent.c

10.2) input only
 input: sort < aja.txt
 input file:
  aj
  nick
  shiv
  mikias
  janelle
  will
  nurhaliza
  taylor
  rodolfo
  alex

  output:
  osh> sort < aja.txt
  You entered: sort < aja.txt
  redirecting the input

  wating for child or children to complete
  aj
  alex
  janelle
  mikias
  nick
  nurhaliza
  rodolfo
  shiv
  taylor
  will

10.3) input to output
  input: sort < aja.txt > aj.txt
  input file:
  aj
  nick
  shiv
  mikias
  janelle
  will
  nurhaliza
  taylor
  rodolfo
  alex
  
  output file:
  aj
  alex
  janelle
  mikias
  nick
  nurhaliza
  rodolfo
  shiv
  taylor
  will
  
  output of code:
  osh> sort < aja.txt > aj.txt
  You entered: sort < aja.txt > aj.txt
  redirecting the input
  redirecting the output

  wating for child or children to complete

11) use pipes - yes
  input: ls -l | less

  ouptut:
  osh> ls -l | less
  You entered: ls -l | less
  using a pipe

  wating for child or children to complete

  total 212
  -rw-r--r-- 1 osc osc   350 Mar 13 22:01 <E0>1<83><E7><FF>^?
  -rw-r--r-- 1 osc osc    62 Mar 28 23:30 aj.txt
  -rw-r--r-- 1 osc osc   371 Mar 28 23:21 aja.txt
  -rw-rw-r-- 1 osc osc   710 Jan  3  2018 DateClient.java
  -rw-rw-r-- 1 osc osc   810 Jun 18  2018 DateServer.java
  -rwxrwxr-x 1 osc osc 10248 Feb 13 00:53 fig3-30
  -rw-rw-r-- 1 osc osc   361 Jun 18  2018 fig3-30.c
  :

*/