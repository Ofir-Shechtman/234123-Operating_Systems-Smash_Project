#include <iostream>
#include <csignal>
#include "Commands.h"
#include "signals.h"



int main(int argc, char* argv[]) {
    setbuf(stdout,nullptr); //TODO remove
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(signal(SIGALRM ,alarmHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");//TODO:change
    }
    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        //std::cout<<"\x1B[1;36m"<<smash.get_prompt()<< "> " << "\x1B[0m";
        std::cout << smash.get_prompt() <<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash.executeCommand(cmd_line.c_str());
        }
        catch(Continue&){
            continue;
        }
        catch(Quit&){
            break;
        }
    }
    return 0;
}