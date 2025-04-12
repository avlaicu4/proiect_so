#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include <fcntl.h>
#include<time.h>
#include <errno.h> 

#define MAX_PATH 100

typedef struct GPS_coord
{
    float lat;
    float lon;
}GPS_coord;

typedef struct
{
    int id;
    char user_name[30];
    GPS_coord l;
    char clue[30];
    int value;

}Treasure;

void add(const char *hunt_name,Treasure *t)
{
    //creem caile necesare
    char dir_path[MAX_PATH ], file_path[MAX_PATH], log_path[MAX_PATH],symlink_path[MAX_PATH];
    sprintf(dir_path,"./%s",hunt_name); // hunt ul unde ne aflam
    //construim incet linia de terminal
    sprintf(file_path,"%s/treasure.bin",dir_path);
    sprintf(log_path,"%s/logged_hunt",dir_path);
    sprintf(symlink_path,"logged_hunt -- %s",hunt_name);

    if(mkdir(dir_path,0755) == -1) // folosim 0755 ca sa dam permisiunea ca ceilalti sa poata citi si executa
    {
        if(errno != EEXIST)
        {
            perror("nu exista directorul!");
            exit(-1);
        }
    }

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND,0644); //folosim permisiunea 0644 ca ceilalti sa poata doar citi
    if(fd == -1)
    {
        perror("eroare la deschiderea fisierului!");
        exit(-1);
    }

    if(write(fd,t,sizeof(Treasure)) != sizeof(Treasure))
    {
        perror("eroare la scriere");
        close(fd);
        exit(-1);
    }

    //inchide fisierul binar
    close(fd);

    int fd_log=open(log_path,O_WRONLY | O_CREAT | O_APPEND,0644);
    if(fd_log >= 0)
    {
        dprintf(fd_log,"Addes treasure id %d by %s\n",t->id,t->user_name);
        close(fd_log);
    }

    close(fd_log);
    //verificam daca sydlink ul exista si daca nu il creem
    if(access(symlink_path,F_OK) == -1)
    {
        symlink(log_path,symlink_path);
    }


}

void list(const char *hunt_name)
{
    char file_path[MAX_PATH];
    sprintf(file_path,"./%s/treasure.bin",hunt_name);
    printf("Verific: %s\n", file_path);

    struct stat st;

    if(stat(file_path, &st) == -1)
    {
        perror("stat");
        exit(-2);
    }

    printf("Hunt: %s \n Size: %lld bytes\n Last modified: %s",hunt_name,st.st_size,ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if(fd == -1)
    {
        perror("open");
        exit(-2);
    }

    Treasure t;

    while(read(fd,&t,sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("ID: %d | User: %s | Latitudine: %.2f | Longitudine: %.2f | Clue: %s | Value: %d\n", t.id,t.user_name,t.l.lat,t.l.lon,t.clue,t.value);
    }

    close(fd);
}

void view(const char *hunt_name, int id)
{
    char file_path[MAX_PATH];
    sprintf(file_path,"./%s/treasure.bin", hunt_name);

    int fd=open(file_path,O_RDONLY);
    if(fd == -1)
    {
        perror("eroare la deschidere");
        exit(-3);
    }

    Treasure t;
    while(read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if(t.id == id)
        {
            printf("Found treasure -> id: %d, user: %s, lat: %.2f, lot: %.2f, clue: %s, value: %d\n", t.id, t.user_name, t.l.lat, t.l.lon, t.clue, t.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with id %d not found\n",id);
    close(fd);
}

void remove_treasure(const char *hunt_name, int id)
{
    char file_path[MAX_PATH], aux[MAX_PATH];
    sprintf(file_path,"./%s/treasure.bin",hunt_name);
    sprintf(aux,"./%s/aux.bin",hunt_name);

    int fd_in=open(file_path,O_RDONLY);
    int fd_out=open(aux,O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(fd_in == -1 || fd_out == -1)
    {
        perror(NULL);
        exit(-1);
    }

    Treasure t;

    while(read(fd_in, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if(t.id != id)
        {
            write(fd_out, &t, sizeof(Treasure));
        }
    }

    close(fd_in);
    close(fd_out);

    unlink(file_path);
    rename(aux, file_path);

    char log_path[MAX_PATH];
    sprintf(log_path,"./%s/logged_hunt", hunt_name);
    int fd_log = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if(fd_log >= 0)
    {
        dprintf(fd_log,"Removed tresure ID %d\n", id);
        close(fd_log);
    }
}

void remove_hunt(const char *hunt_name)
{
     //creem caile necesare
     char dir_path[MAX_PATH ], file_path[MAX_PATH], log_path[MAX_PATH],symlink_path[MAX_PATH];
     sprintf(dir_path,"./%s",hunt_name); // hunt ul unde ne aflam
     //construim incet linia de terminal
     sprintf(file_path,"%s/treasure.bin",dir_path);
     sprintf(log_path,"%s/logged_hunt",dir_path);
     sprintf(symlink_path,"logged_hunt -- %s",hunt_name);

     unlink(file_path);
     unlink(log_path);
     unlink(symlink_path);
     rmdir(dir_path);
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: %s -- add| --list| --view| --remove_treasure| --remove <hunt_name> [id]",argv[0]);
    }

    if(strcmp(argv[1],"--add") == 0)
    {
        Treasure t;
        printf("id: ");
        scanf("%d", &t.id);

        printf("user: ");
        scanf("%s", t.user_name);
        
        printf("latitude: ");
        scanf("%f", &t.l.lat);

        printf("longitude: ");
        scanf("%f", &t.l.lon);

        printf("clue: ");
        fgetc(stdin);
        fgets(t.clue, sizeof(t.clue), stdin);
        t.clue[strcspn(t.clue, "\n")] = '\0';


        printf("value: ");
        scanf("%d", &t.value);

        add(argv[2], &t);
    }
    else if(strcmp(argv[1],"--list") == 0)
    {
        list(argv[2]);
    }
    else if(strcmp(argv[1],"--view") == 0 && argc >= 4)
    {
        view(argv[2],atoi(argv[3]));
    }
    else if(strcmp(argv[1],"--remove_treasure") == 0 && argc >= 4)
    {
        remove_treasure(argv[2],atoi(argv[3]));
    }
    else if(strcmp(argv[1],"--remove") == 0)
    {
        remove_hunt(argv[2]);
    }
    else
    {
        printf("comanda neidentificata\n");
    }
    return 0;
}
