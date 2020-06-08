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

#include "MemoryModifier.h"

#include <string.h> 
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  

//  TODO: Maybe the size is not enough
MemoryRegion* getMemoryRegions(Process *p, size_t *size) {
    if (p == NULL) {
        fprintf(stderr, "Process is NULL\n");
        return NULL;
    }

    MemoryRegion* m_regions;

    FILE *f_maps;
    char line[512];
    char n_maps[512];
    char buffer[100];

    sprintf(n_maps, "/proc/%lu/maps", p->p_id);

    if ((f_maps = fopen(n_maps, "r")) == NULL) {
        fprintf(stderr, "Failed to open file %s\n", n_maps);
        return NULL;
    }

    m_regions = NULL;
    (*size) = 0;

    while (fgets(line, 512, f_maps)) {
        if (m_regions == NULL) {
            m_regions = malloc(sizeof(MemoryRegion));
        } else {
            m_regions = realloc(m_regions, sizeof(MemoryRegion) * (*size + 1));
        }

        int32_t s0 = strstr(line, "-") - line;
        int32_t s1 = strstr(line, " ") - line;
        int32_t s2 = (long)strstr(line, "/");

        if (s2 == 0) {
            s2 = (long)strstr(line, "[");

            if (s2 != 0) {
                s2 -= (long)line;
            }
        } else {
            s2 -= (long)line;
        }

        // start
        memset(buffer, 0, 100);
        strncpy(buffer, line, s1);
        m_regions[(*size)].start = strtol(buffer, NULL, 16);

        // end
        memset(buffer, 0, 100);
        strncpy(buffer, line + s0 + 1, s1);
        m_regions[(*size)].end = strtol(buffer, NULL, 16);

        // rwxp
        m_regions[*size].readable = line[s1 + 1] != '-';
        m_regions[*size].writable = line[s1 + 2] != '-';
        m_regions[*size].executable = line[s1 + 3] != '-';
        m_regions[*size].shared = line[s1 + 4] != '-';

        // name/path
        memset(m_regions[*size].name, 0, 100);
        if (s2) {
            strncpy(m_regions[*size].name, line + s2, strlen(line) - s2 - 1);
        }
    
        (*size) ++;
    }

    fclose(f_maps);
    return m_regions;
}

bool getMemoryRegion(Process *p, char *m_name, MemoryRegion *m_region) {
    for (int i = 0; p->m_regions_size; ++ i) {
        if (strstr(p->m_regions[i].name, m_name) != NULL) {
            *m_region = p->m_regions[i];
            return TRUE;
        }
    }

    return FALSE;
}

Process* openProcess(char *p_name) {
    if (strlen(p_name) >= 1024) {
        fprintf(stderr, "Process Name is invalid\n");
        return NULL;
    }

    struct dirent *entry;

    Process *p;

    if ((p = (Process*) malloc(sizeof(Process))) == NULL) {
        fprintf(stderr, "Failed to allocate memory to Process\n");
        return NULL;
    }


    DIR *d_proc;
    int i;

    if ((d_proc = opendir("/proc/")) == NULL) {
        fprintf(stderr, "Failed to open directory /proc/\n");
        return NULL;
    }

    while ((entry = readdir(d_proc)) != NULL) {
        if (atol(entry->d_name) == 0) {
            continue;
        }

        FILE *f_status;
        char n_status[32];
        char b_status[32];

        sprintf(n_status, "/proc/%s/status", entry->d_name);

        if ((f_status = fopen(n_status, "r")) == NULL) {
            fprintf(stderr, "Failed to open file %s\n", n_status);
            return NULL;
        }

        fseek(f_status, 6, SEEK_SET);
        fgets(b_status, 32, f_status);

        fclose(f_status);

        if (strstr(b_status, p_name) != NULL) {
            p->p_id = atol(entry->d_name);
            strncpy(p->p_name, b_status, strlen(b_status) - 1); // copy to struct and remove endl
            p->p_name[strlen(b_status) - 1] = '\0';

            char n_mem[1024];

            sprintf(n_mem, "/proc/%s/mem", entry->d_name);

            if ((p->p_mem = open(n_mem, O_RDWR)) < 0) {
                fprintf(stderr, "Failed to inject into memory, forgot sudo?\n");
                return NULL;
            }

            p->m_regions = getMemoryRegions(p, &p->m_regions_size);
            for (i = 0; i < p->m_regions_size; ++ i) {
                p->m_regions[i].mem = p->p_mem;
            }

            goto end;
        }
    }

    fprintf(stderr, "Failed, process not found\n");
    exit(EXIT_FAILURE);

    end:

    closedir(d_proc);

    return p;
}

void closeProcess(Process *p) {
    if (p->p_mem) {
        close(p->p_mem);
    }
    free(p->m_regions);
    free(p);
}

bool writeProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size) {
    lseek(reg.mem, address, SEEK_SET);

    if (!write(reg.mem, buffer, size)) {
        return 0;
    }

    lseek(reg.mem, 0, SEEK_SET);

    return 1;
}

bool readProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size) {
    lseek(reg.mem, address, SEEK_SET);

    if (!read(reg.mem, buffer, size)) {
        return 0;
    }

    lseek(reg.mem, 0, SEEK_SET);

    return 1;
}

long searchAddress(MemoryRegion reg, const byte *signature, const uint8_t size) {
    return reg.start + searchOffset(reg, signature, size);
}

long searchOffset(MemoryRegion reg, const byte *signature, const uint8_t size) {
    byte* buffer;
    int i, j;

    if ((buffer = malloc(size)) == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; readProcessMemory(reg, reg.start + i, buffer, size * 2); i += size) {
        for (j = 0; j < size; ++ j) {
            if (memcmp(buffer + j, signature, size) == 0) {
                free(buffer);
                return i + j;
            }
        }
    }

    free(buffer);

    fprintf(stderr, "Nothing found\n");

    return 0;
}

void searchByValue(Process *p, MemoryRegion reg, uint8_t size, DataType type) {
    int i, r;
    char *line;
    size_t len;

    byte *buffer;
    bool *valid;

    byte *number;

    uint64_t range = reg.end - reg.start;

    if (size == 0) {
        fprintf(stderr, "No valid size\n");
        exit(EXIT_FAILURE);
    }

    if ((buffer = malloc(range)) == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    if ((line = malloc(100)) == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    buffer = malloc(range * size);
    valid = malloc(range);

    memset(valid, 1, range);

    printf("Write Q to exit.\n");

    for (r = 1; r > 0; ) {
        printf("? ");
        getline(&line, &len, stdin);

        if (strstr(line, "q") > 0) {
            break;
        } else if (type == INT) {
            number = getBytes(int, atoi(line));
        } else if (type == FLOAT) {
            number = getBytes(float, atof(line));
        }

        readProcessMemory(reg, reg.start, buffer, range * size);

        for (i = 0; i < range; ++ i) {
            if (valid[i] && memcmp(buffer + i * size, number, size) != 0) {
                valid[i] = 0;
            }
        }

        for (i = 0, r = 0; i < range / size; ++ i) {
            if (!valid[i]) {
                continue;
            }

            if (r < 100) {
                p->history[r] = i * size;
                printf("%d: %p\n", r, i * size);
            }

            r ++;
        }

        printf("R: %d\n", r);
    }

    free(line);
    free(buffer);
}

void searchByChange(Process *p, MemoryRegion reg, uint64_t size) {
    uint64_t range = (reg.end - reg.start) / size;

    byte *buffer = malloc(range * size);
    byte *buffer1 = malloc(range * size);
    bool *valid = malloc(range);

    unsigned int lastTime;
    int i, r;
    char key;

    bool n = 0;
    bool cw = 0;

    memset(valid, 1, range);

    if (!readProcessMemory(reg, reg.start, buffer, range * size)) {
        printf("\x1b[31;1mE:\x1b[0m Failed to read memory!\n");
        goto end;
    }

    initscr();
    nodelay(stdscr, TRUE);
    clear();

    lastTime = time(0);

    printw("Enter or wait 5s to continue, q to quit\n");
    printw("Equal: %s\n", n ? "false" : "true");
    printf("\a");

    while (1) {
        do {
            key = getch();
        } while (time(0) < lastTime + 5 && key == -1);

        if (key == 'q') {
            break;
        }

        clear();
        printw("Enter or wait 5s to continue, q to quit\n");
        n = !n;
        printw("Equal: %s\n\n", n ? "false" : "true");

        readProcessMemory(reg, reg.start, cw ? buffer : buffer1, range * size);
        cw = !cw;

        for (i = 0; i < range; ++ i) {
            if (n && valid[i] && memcmp(buffer + i * size, buffer1 + i * size, size) != 0) {
                valid[i] = 0;
            } else if (!n && valid[i] && memcmp(buffer + i * size, buffer1 + i * size, size) == 0) {
                valid[i] = 0;
            }
        }


        for (i = 0, r = 0; i < range; ++ i) {
            if (valid[i]) {
                if (r <= 10) {
                    p->history[r] = i * size;
                    printw("%d: %p\n", r, i * size);
                }
                r ++;
            }
        }

        printw("\nTotal results: %d\n", r);
        lastTime = time(0);
    }

    endwin();

    end:

    free(buffer);
    free(buffer1);
    free(valid);
}


// ?    - unknown value
// x    - equal to pattern
// 0-9  - equal to other value
uint64_t getAddressByPattern(MemoryRegion reg, const uint64_t start, byte *pattern, char *mask, const size_t size) {
    byte *memory = malloc(reg.end - reg.start);
    long base;
    uint64_t i, j;

    if (!readProcessMemory(reg, reg.start, memory, reg.end - reg.start)) {
        printf("\x1b[31;1mE:\x1b[0m Failed to read memory!\n");
        free(memory);
        return 0;
    }

    for (base = reg.start + start, i = 0; base < reg.end; ++ base) {

        if (i == size) {
            free(memory);
            return base - reg.start;
        }
        
        if (mask[i] == '?') {
            i ++;
        } else if (mask[i] == 'x') {
            if (pattern[i] == memory[base - reg.start]) {
                i ++;
            } else {
                base -= i;
                i = 0;
            }
        } else if (mask[i] >= '0' && mask[i] <= '9') {
            for (j = 0; j < size; ++ j) {
                if (mask[j] == mask[i] && memory[base - reg.start] != memory[base - reg.start + j - i]) {
                    base -= i;
                    i = -1;
                    break;
                }
            }
            i ++;
        } else {
            printf("\x1b[31;1mE:\x1b[0m Wrong symbol in mask!\n");
            break;
        }
    }

    free(memory);
    return 0;
}

void showRange(Process *p, MemoryRegion *reg, uint64_t address, int start, int end) {
    int64_t range = end - start;
    int i, j, cw, row, col;

    // 4 bytes per line
    start *= 4;
    end *= 4;

    byte *status = malloc(range);
    byte *buffer = malloc(range);
    byte *buffer1 = malloc(range);
    byte number[4];


    memset(status, 1, range);

    cw = 0;
    if (!readProcessMemory(*reg, reg->start + address + start, cw ? buffer1 : buffer, range)) {
        printf("\x1b[31;1mE:\x1b[0m Failed to read memory!\n");
        return;
    }

    initscr();
    use_default_colors();
    nodelay(stdscr, TRUE);
    clear();

    start_color();
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_RED,   -1);

    while (1) {
        getmaxyx(stdscr, row, col); 

        switch (getch()) {
        case 'e':
            clear();
            mvprintw(0, 0, "Press e again to continue");
            while (getch() != 'e');

            endwin();
            scrollok(stdscr, FALSE);
            nodelay(stdscr, FALSE);

            closeProcess(p);
            p = openProcess("supertux");

            if (!getMemoryRegion(p, reg->name, reg)) {
                fprintf(stderr, "Failed to find mem region\n");
            }

            printf("Address?\n");
            scanf("%x", &address);

            clear();
            initscr();
            use_default_colors();
            nodelay(stdscr, TRUE);
            break;
        case 'g':
            endwin();
            scrollok(stdscr, FALSE);
            nodelay(stdscr, FALSE);

            fflush(stdout);
            printf("\n\n\nPattern: ");
            fflush(stdout);
            for (i = start; i < end; ++ i) {
                if (status[i - start]) {
                    if (buffer[i - start] == 0) {
                        printf("\\0");
                    } else {
                        printf("\\x%02x", buffer[i - start]);
                    }
                } else {
                    printf("?");
                }
            }

            printf("\n\nMask: ");
            for (i = start; i < end; ++ i) {
                if (status[i - start]) {
                    printf("x");
                } else {
                    printf("?");
                }
            }
            printf("\n");
            goto end;
        case 'q':
            goto end;
        }

        cw = !cw;
        readProcessMemory(*reg, reg->start + address + start, cw ? buffer1 : buffer, range);

        for (i = start; i < end; i += 4) {
            readProcessMemory(*reg, reg->start + address + i, number, 4);

            char f[80];
            sprintf(f, "%f", getValue(float, number));
            if (strlen(f) > 30) {
                strcpy(f, "inf");
            }

            mvprintw((i - start) / 4, 0, "%p:%d\t%s:%d\t\t ", address + i, i, f, getValue(int, number));

            for (j = 0; j < 4; ++ j) {
                if (buffer[i - start + j] != buffer1[i - start + j]) {
                    status[i - start + j] = 0;
                }

                attron(COLOR_PAIR(status[i - start + j] ? 1 : 2));

                mvprintw (
                    (i - start) / 4,
                    60 + j * 3,
                    "%02x",
                    number[j]
                );

                attroff(COLOR_PAIR(status[i - start + j] ? 1 : 2));
            }

            for (j = 0; j < 4; ++ j) {
                mvprintw((i - start) / 4, 80 + j * 2, "%c", number[j]);
            }
        }

        mvprintw(row - 3, 0, "Press Q to exit.");
        mvprintw(row - 2, 0, "Press G to generate pattern.");
        mvprintw(row - 1, 0, "Press E to overlap new data.");
        refresh();

        usleep(10 * 1000);
    }

end:
    endwin();
    scrollok(stdscr, FALSE);
    nodelay(stdscr, FALSE);

    free(buffer);
    free(buffer1);
    free(status);
}
