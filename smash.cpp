#include <iostream>
#include <csignal>
#include "Commands.h"
#include "signals.h"
#include "system_functions.h"


int main(int argc, char *argv[]) {
    setbuf(stdout, nullptr); //TODO remove
    struct sigaction alarm = {{alarmHandler}};
    alarm.sa_flags = SA_RESTART;
    alarm.sa_handler = alarmHandler;
    if (sigaction(SIGALRM, &alarm,nullptr) == -1) {
        perror("smash error: failed to set alarm handler");
    }
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        //std::cout<<"\x1B[1;36m"<<smash.get_prompt()<< "> " << "\x1B[0m";
        std::cout << smash.get_prompt() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash.executeCommand(cmd_line.c_str());
        }
        catch (Continue &) {
            if(smash.is_smash())
                continue;
            break;
        }
        catch (Quit &) {
            break;
        }
        catch(std::bad_alloc& e) {
            if (smash.is_smash())
                continue;
            break;
        }
    }
    return 0;
}