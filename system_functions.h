#ifndef SMASH_SYSTEM_FUNCS_H
#define SMASH_SYSTEM_FUNCS_H

#include <unistd.h>
#include <cstdio>
#include <exception>
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <istream>
#include <stdexcept>

class Quit : public std::exception{};
class Continue : public std::exception{};

void do_perror(const char* syscall);
int  do_open(const char* file, int flags);
int  do_read(int fd, char* buffer, int BUFSIZE);
int  do_write(int df, char* buffer, int n);
void do_close(int fd);
int  do_fork();
int  do_stoi(std::string num);
void do_execvp(const char* cmd);
int  do_kill(pid_t pid, int signal);
void do_chdir(const char* path);
void do_pipe(int* fds);
void do_dup2(int fd, int fd_replace);
pid_t do_waitpid(pid_t pid, int *stat_loc, int options);


#endif //SMASH_SYSTEM_FUNCS_H
