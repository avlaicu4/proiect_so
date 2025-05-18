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




int main()
{
    Treasure t;
    int total = 0;

    while (read(STDIN_FILENO, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        total += t.value;
    }

    printf("[score_calculator] Total score: %d\n", total);
    return 0;
}