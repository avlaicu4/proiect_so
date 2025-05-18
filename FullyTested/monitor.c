#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_CHAR 120

typedef struct GPS_coord {
    float lat;
    float lon;
} GPS_coord;

typedef struct {
    int id;
    char user_name[MAX_CHAR];
    GPS_coord l;
    char clue[MAX_CHAR];
    int value;
} Treasure;

void list_hunts() {
    DIR *d = opendir(".");
    if (!d) return;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
            char file_path[256];
            sprintf(file_path, "%s/treasure.bin", dir->d_name);

            struct stat st;
            if (stat(file_path, &st) == 0) {
                int total = st.st_size / sizeof(Treasure);
                printf("hunt: %s - treasures: %d, size: %lld bytes\n", dir->d_name, total, (long long)st.st_size);
            }
        }
    }

    closedir(d);
}

void handle_sigusr2(int sig) {
    printf("\n[monitor] Received SIGUSR2\n");

    FILE *f = fopen("command.txt", "r");
    if (!f) {
        perror("Could not open command.txt");
        return;
    }

    char first[16];
    fscanf(f, "%s", first);

    if (strcmp(first, "view") == 0) {
        char hunt[100];
        int id;
        fscanf(f, "%s %d", hunt, &id);
        fclose(f);

        char cmd[256];
        sprintf(cmd, "./treasure_manager --view %s %d", hunt, id);
        system(cmd);
    } else if (strcmp(first, "score") == 0) {
        // caz nou pentru score_calculator
        char hunt[100];
        fscanf(f, "%s", hunt);
        fclose(f);

        // deschidem treasure.bin
        char path[128];
        sprintf(path, "%s/treasure.bin", hunt);
        int fd = open(path, O_RDONLY);
        if (fd == -1) {
            perror("Could not open treasure.bin");
            return;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            close(fd);
            return;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            close(fd);
            return;
        }

        if (pid == 0) {
            // copil: redirectăm pipe-ul spre stdin și pornim score_calculator
            close(pipefd[1]); // închide capătul de scriere
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execl("./score_calculator", "./score_calculator", NULL);
            perror("exec failed");
            exit(1);
        } else {
            // părinte: scrie comorile în pipe
            close(pipefd[0]); // închide capătul de citire

            Treasure t;
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                write(pipefd[1], &t, sizeof(Treasure));
            }

            close(fd);
            close(pipefd[1]); // semnalăm EOF copilului
        }

    } else {
        // fallback: list treasures
        char hunt_name[100];
        strcpy(hunt_name, first);
        fclose(f);

        char file_path[256];
        sprintf(file_path, "%s/treasure.bin", hunt_name);
        int fd = open(file_path, O_RDONLY);
        if (fd == -1) {
            perror("open");
            return;
        }

        Treasure t;
        while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
            printf("ID: %d | User: %s | Lat: %.2f | Lon: %.2f | Clue: %s | Value: %d\n",
                   t.id, t.user_name, t.l.lat, t.l.lon, t.clue, t.value);
        }

        close(fd);
    }
}


void handle_sigusr1(int sig) {
    printf("\n[monitor] Received SIGUSR1: list_hunts\n");
    list_hunts();
}

void handle_sigterm(int sig) {
    printf("\n[monitor] Received SIGTERM: exiting after delay...\n");
    usleep(3000000); // 3 sec delay
    exit(0);
}

int main() {
    struct sigaction sa;

    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigusr2;
    sigaction(SIGUSR2, &sa, NULL);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    printf("[monitor] Ready and waiting for signals...\n");

    while (1) {
        pause(); // wait for signals
    }

    return 0;
}
