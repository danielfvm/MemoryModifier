/*
    This file is part of MemoryModifier.

    MemoryModifier is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    MemoryModifier is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MemoryModifier.  If not, see <https://www.gnu.org/licenses/>.
*/

#define DEF_NAME "MemoryModifier"
#define DEF_VERSION "0.1-a1"

#include "MemoryModifier.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ptrace.h>

typedef struct Command Command;

struct Command {
    char name[40];
    char usage[40];
    char desc[80];
    void (*call)(Command *cmd, char **args, size_t len);
};

#define CMD_LEN 12
static Command commands[CMD_LEN];
static Process* p;
static MemoryRegion reg;

DataType getDataType(char *type) {
    if (strcmp(type, "int") == 0)
        return INT;
    else if (strcmp(type, "float") == 0)
        return FLOAT;
    else if (strcmp(type, "str") == 0)
        return STR;
    return 0;
}

void cmd_help(Command *cmd, char **args, size_t len) {
    int i;

    if (len == 1) {
        for (i = 0; i < CMD_LEN; ++ i) {
            printf("%s\t%s\n", commands[i].name, commands[i].usage);
        }
    }
    
    if (len == 2) {
        for (i = 0; i < CMD_LEN; ++ i) {
            if (strcmp(commands[i].name, args[1]) == 0) {
                printf (
                    "Name: \t%s\nUsage: \t%s\n%s\n", 
                    commands[i].name, 
                    commands[i].usage, 
                    commands[i].desc
                );
                return;
            }
        }
        printf("\x1b[31;1mE:\x1b[0m No information found!\n");
    }
}

void cmd_clear(Command *cmd, char **args, size_t len) {
    printf("\033[2J\033[;H");
}

void cmd_open(Command *cmd, char **args, size_t len) {
    int i;

    p = openProcess(args[1]);

    if (len == 3) {
        getMemoryRegion(p, args[2], &reg);
    } else {
        getMemoryRegion(p, "[heap]", &reg);
    }

    if (p != NULL) {
        printf("\x1b[1mProcess Name:\x1b[0m %s\n", p->p_name);
        printf("\x1b[1mProcess ID:\x1b[0m %d\n", p->p_id);
        printf("\x1b[1mMemory Address:\x1b[0m %p\n", reg.start);
        printf("\x1b[1mMemory Size:\x1b[0m %p\n", reg.end - reg.start);
    }
}

void cmd_close(Command *cmd, char **args, size_t len) {
    printf("Process \x1b[3m%s\x1b[0m closed.\n", p->p_name);

    closeProcess(p);
    p = NULL;
}

void cmd_pause(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }
    ptrace(PTRACE_ATTACH, p->p_id, NULL, NULL);
}

void cmd_resume(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }
    ptrace(PTRACE_DETACH, p->p_id, NULL, NULL);
}

void cmd_kill(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }
    ptrace(PTRACE_SETOPTIONS, p->p_id, 0, PTRACE_O_EXITKILL);
    closeProcess(p);
    p = NULL;
}

void cmd_search(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }

    int size = strcmp(args[2], "bool") ? 1 : strcmp(args[2], "double") ? 8 : 4;
    if (strcmp(args[1], "fix") == 0) {
        searchByValue(p, reg, size, getDataType(args[2]));
    } else if (strcmp(args[1], "change") == 0) {
        searchByChange(p, reg, size);
    } else {
        printf("\x1b[31;1mE:\x1b[0m Wrong argument!\n");
    }
}

void cmd_set(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }

    DataType type = getDataType(args[2]);


    if (type == FLOAT) printf (
        "%s\n", 
        writeProcessMemory(reg, reg.start + strtol(args[1], NULL, 16), getBytes(float, atof(args[3])), 4) ? 
            "Success" : 
            "Failed"
    ); else if (type == INT) printf (
        "%s\n", 
        writeProcessMemory(reg, reg.start + strtol(args[1], NULL, 16), getBytes(int, atof(args[3])), 4) ? 
            "Success" : 
            "Failed"
    ); else if (type == STR) printf (
        "%s\n", 
        writeProcessMemory(reg, reg.start + strtol(args[1], NULL, 16), args[3], strlen(args[3])) ? 
            "Success" : 
            "Failed"
    );
}

void cmd_get(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }

    DataType type = getDataType(args[2]);

    if (type == INT) {
        byte buffer[4];
        readProcessMemory(reg, reg.start + strtol(args[1], NULL, 16), buffer, 4);
        printf("%d\n", getValue(int, buffer));
    } else if (type == FLOAT) {
        byte buffer[4];
        readProcessMemory(reg, reg.start + strtol(args[1], NULL, 16), buffer, 4);
        printf("%d\n", getValue(float, buffer));
    }

}

void cmd_full(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }

    printf("%p\n", p->b_addr + strtol(args[1], NULL, 16));
}

void cmd_show(Command *cmd, char **args, size_t len) {
    if (p == NULL) {
        printf("\x1b[31;1mE:\x1b[0m No process open!\n");
        return;
    }

    showRange(p, &reg, strtol(args[1], NULL, 16), len > 2 ? atoi(args[2]) : 0, len > 3 ? atoi(args[3]) : 4);
}

void addCommand(char *name, char *usage, char *desc, void (*call)(Command*, char**, size_t)) {
    static size_t i = 0;

    strcpy(commands[i].name, name);
    strcpy(commands[i].usage, usage);
    strcpy(commands[i].desc, desc); 
    commands[i].call = call;

    i ++;
}

int main() {
    char *input, *ptr, **args;
    size_t inputlen, argslen;
    size_t minlen;
    size_t i, j;
    bool found;

    // Info Text
    printf("%s %s\n", DEF_NAME, DEF_VERSION);
    printf("Type help for more informations.\n");
    printf("\n\x1b[1mmm? \x1b[0m");
    fflush(stdout);

    if ((input = malloc(100)) == NULL) {
        return EXIT_FAILURE;
    }

    if ((args = malloc(sizeof(char*) * 40)) == NULL) {
        free(input);
        return EXIT_FAILURE;
    }

    addCommand("help", "help [cmd]", "Shows additional info how to use this tool.", cmd_help);
    addCommand("clear", "clear", "Clears the screen.", cmd_clear);
    addCommand("open", "open <name> [module]", "Opens a process.", cmd_open);
    addCommand("close", "close", "Closes a process.", cmd_close);
    addCommand("search", "search <fix/change> <DataType>", "Finds offsets.", cmd_search);
    addCommand("set", "set <offset> <DataType> <value>", "Changes memory of offset.", cmd_set);
    addCommand("get", "get <offset> <DataType>", "Shows value of offset.", cmd_get);
    addCommand("full", "full <offset>", "Shows address of offset.", cmd_full);
    addCommand("pause", "pause", "Pauses a process.", cmd_pause);
    addCommand("resume", "resume", "Resums a process again.", cmd_resume);
    addCommand("kill", "kill", "Stops current process.", cmd_resume);
    addCommand("show", "show <address> [start] [end]", "Show address from certain area.", cmd_show);

    while (getline(&input, &inputlen, stdin) && strstr(input, "exit\n") == NULL) {

        // Remove endl of input
        input[strlen(input) - 1] = '\0';

        // Split input into args
        ptr = strtok(input, " ");

        for (argslen = 0; ptr != NULL; ++ argslen) {
            strcpy(args[argslen] = malloc(strlen(ptr) + 1), ptr);
            ptr = strtok(NULL, " ");
        }

        // Execute cmd
        for (i = 0, found = 0; i < CMD_LEN; ++ i) {

            if (args[0][i] == ':') {
                long x = atol(input + 1);
                found = 1;
                printf("%p\n", (p != NULL && x >= 0 && x < 100) ? p->history[x] : 0x0);
                break;
            }

            for (j = 0; j < argslen; ++ j) {
                if (args[j][0] == ':') {
                    long x = atol(args[j] + 1);
                    if (p != NULL && x >= 0 && x < 100) {
                        sprintf(args[j], "%p",  p->history[x]);
                    } else {
                        strcpy(args[j], "0x0");
                    }
                }
            }

            if (strstr(args[0], commands[i].name) == NULL) {
                continue;
            }

            found = 1;

            for (j = 0, minlen = 1; commands[i].usage[j] != '\0'; ++ j) {
                if (commands[i].usage[j] == ' ' && commands[i].usage[j + 1] == '<') {
                    minlen ++;
                }
            }

            if (minlen > argslen) {
                printf("\x1b[31;1mE:\x1b[0m Missing arguments!\n");
                printf("\x1b[1mUsage:\x1b[0m %s\n", commands[i].usage);
                break;
            }

            commands[i].call(&commands[i], args, argslen);
            break;
        }

        // Failed to find cmd
        if (!found) {
            printf("\x1b[31;1mE:\x1b[0m Command not found!");
        }

        // Free memory
        for (i = 0; i < argslen; ++ i) {
            free(args[i]);
        }

        // Print console
        printf("\n\x1b[1mmm? \x1b[0m");
        fflush(stdout);
    }

    // Free memory
    free(args);
    free(input);

    if (p) {
        closeProcess(p);
    }

    return EXIT_SUCCESS;
}
