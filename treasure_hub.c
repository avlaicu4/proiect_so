#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
  treasure_hub:
  - Este programul principal care oferă interfața de comandă utilizatorului
  - Trimite semnale către procesul monitor (SIGUSR1, SIGUSR2, SIGTERM)
  - Scrie fișiere cu detalii despre comandă (ex: command.txt pentru list_treasures)
*/

pid_t monitor_pid = -1;
int waiting_for_monitor_exit = 0;

void handle_sigchld(int sig)
{
    int status;
    waitpid(monitor_pid, &status, 0);
    printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    monitor_pid = -1;
    waiting_for_monitor_exit = 0;
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGCHLD, &sa, NULL);

    char command[100];

    while(1)
    {
        printf("hub> ");
        fflush(stdout);
        if(fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }
        command[strcspn(command,"\n")] = 0; // elimina linia noua

        if(strcmp(command,"start_monitor") == 0)
        {
            if(monitor_pid != -1)
            {
                printf("monitor already running!\n");
                continue;
            }

            monitor_pid = fork();

            if(monitor_pid == 0)
            {
                execl("./monitor", "./monitor", NULL);
                perror("execl faild");
                exit(1);
            }
            else
            {
                printf("Monitor started with PID %d\n", monitor_pid);
            }
        }
        else if(strcmp(command,"list_hunts") == 0)
        {
            if(monitor_pid == -1)
            {
                printf("monitor not running!\n");
                continue;
            }

            kill(monitor_pid,SIGUSR1);
        }
        else if (strcmp(command, "view_treasure") == 0) {
            if (monitor_pid == -1) {
                printf("Monitor not running!\n");
                continue;
            }
        
            char hunt[100];
            int id;
            printf("Enter hunt name: ");
            scanf("%s", hunt);
            printf("Enter treasure ID: ");
            scanf("%d", &id);
        
            FILE *f = fopen("command.txt", "w");
            if (!f) {
                perror("Could not open command.txt");
                continue;
            }
        
            fprintf(f, "view %s %d", hunt, id);
            fclose(f);
        
            kill(monitor_pid, SIGUSR2);
            getchar(); // curăță newline
        }
        else if(strcmp(command,"stop_monitor") == 0)
        {
            if(monitor_pid == -1)
            {
                printf("monitor not running!\n");
                continue;
            }

            kill(monitor_pid,SIGTERM);
            waiting_for_monitor_exit = 1;
        }
        else if(strcmp(command,"exit") == 0)
        {
            if(monitor_pid != -1)
            {
                printf("error");
            }
            else
            {
                break;
            }
        }
        else
        {
            if(waiting_for_monitor_exit)
            {
                printf("waiting for monitor to exit...\n");
            }
            else
            {
                printf("unknown commlistand: %s \n",command);
            }
        }
    }

    return 0;
}