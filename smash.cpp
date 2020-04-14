#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"
#include <fstream>


int main(int argc, char* argv[]) {
    setbuf(stdout,nullptr); //TODO remove
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler


    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        //smash.jobs_list.printJobsList();
        std::cout << smash.get_prompt() <<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash.executeCommand(cmd_line.c_str());
        }
        catch(QuitCommand::Quit&){
            break;
        }
    }
    return 0;
}