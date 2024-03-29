#include "system_functions.h"


static bool is_smash(){
    return SmallShell::getInstance().is_smash();
}

void do_perror(const char* syscall) {
    std::string msg="smash error: ";
    msg+=syscall;
    msg+=" failed";
    perror(msg.c_str());
}


void do_close(int fd)
{
    if(!fd)
        return;
    if (close(fd) == -1) {
        do_perror("close");
        throw Continue();
    }
}

int do_open(const char *file, unsigned int flags) {
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /*rw-rw-rw-*/
    int fd=open(file, flags, perms);
    if(fd == -1) {
        do_perror("open");
        throw Continue();
    }
    return fd;
}

int do_read(int fd, char *buffer, int BUFSIZE) {
    int n= read(fd, buffer, BUFSIZE);
    if(n == -1){
        do_perror("read");
        throw Continue();
    }
    return n;
}

int do_write(int df, char *buffer, int n) {
    if(write(df, buffer, n) != n) {
        do_perror("write");
        throw Continue();
    }
    return 0;
}


int do_fork() {
    //pid_t smash_pid= SmallShell::getInstance().pid;
    bool im_smash=is_smash();
    int child = fork();
    if(child<0) {
        do_perror("fork");
        throw Continue();
    }
    else if(child==0) {
        if(im_smash)
            setpgrp();
        raise(SIGSTOP); // stop the child
    }
    else {
        waitpid(child, NULL, WUNTRACED); // wait until the child is stopped
        kill(child, SIGCONT); // resume the child
    }
    return child;
}

void do_pipe(int *fds) {
    if(pipe(fds) == -1) {
        do_perror("pipe");
        throw Continue();
    }
}

void do_dup2(int fd, int fd_replace) {
    if(dup2(fd, fd_replace)==-1) {
        do_perror("dup2");
        throw Continue();
    }

}

void do_execvp(const char* cmd) {
    const char* const argv[4] = {"/bin/bash", "-c", cmd, nullptr};
    execvp(argv[0], const_cast<char* const*>(argv));
    do_perror("execvp");
    throw Continue();

}

int do_kill(pid_t pid, int signal) {
    int res=killpg(pid, signal);
    if(res!=0) {
        do_perror("kill");
        throw Continue();
    }
    return res;
}

void do_chdir(const char *path) {
    if(chdir(path) == -1) {
        do_perror("chdir");
        throw Continue();
    }
}

pid_t  do_waitpid(pid_t pid, int options){
    if(!pid)
        return pid;
    return waitpid(pid, nullptr, options);;
}

int do_stoi(std::string num) {
    size_t size;
    int value;
    value = stoi(num,&size);
    if(size != num.size())
        throw std::invalid_argument(num);
    return value;
}
