#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<dirent.h>
#include<time.h>
#include<unistd.h>

#define MAX_LENGTH 512

//structura cu info despre BMP
typedef struct{
    unsigned int inaltime, lungime, dim;
    char *user_id;
    char timp_modif[20];
    int legaturi;
    char user_perm[4];
    char group_perm[4];
    char other_perm[4];
} BMP_info;

// citire info despre BMP
void readBMP_info(const char *filename, BMP_info *info) 
{
    struct stat fis_info;

    if (stat(filename, &fis_info) == -1) 
    {
        perror("Eroare la obtinerea informatiilor despre fisier.\n");
        exit(EXIT_FAILURE);
    }

    info->dim = fis_info.st_size;
    info->legaturi = fis_info.st_nlink;

    struct tm *tm_info;
    tm_info = localtime(&fis_info.st_mtime);

    if (tm_info == NULL)
    {
        perror("Eroare la detectarea timpului local.\n");
        exit(EXIT_FAILURE);
    }

    if (strftime(info->timp_modif, sizeof(info->timp_modif), "%d.%m.%Y", tm_info) == 0) 
    {
        fprintf(stderr, "Eroare la formatarea timpului.\n");
        exit(EXIT_FAILURE);
    }

    int bmp_file = open(filename, O_RDONLY);
    if (bmp_file == -1) 
    {
        perror("Eroare la deschiderea fisierului BMP.\n");
        exit(EXIT_FAILURE);
    }

    unsigned char f_header[54];
    if (read(bmp_file, f_header, sizeof(f_header)) == -1) {
        perror("Eroare la citirea header-ului fisierului BMP.\n");
        close(bmp_file);
        exit(EXIT_FAILURE);
    }

    info->user_id = malloc(strlen("123") + 1);
    if (info->user_id == NULL) {
        perror("Eroare la alocarea memoriei pentru UserId.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(info->user_id, "123");

    info->lungime = *(unsigned int*)&f_header[18];
    info->inaltime = *(unsigned int*)&f_header[22];

    if (close(bmp_file) == -1) {
        perror("Eroare la inchiderea fisierului BMP.\n");
        exit(EXIT_FAILURE);
    }
}

// scrierea info in statistica.txt
void afisarea_info(const char *filename, const BMP_info *info) 
{
    int info_file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (info_file == -1)
    {
        perror("Eroare la deschiderea fisierului de statistici.\n");
        exit(EXIT_FAILURE);
    }

    char info_str[MAX_LENGTH];
    sprintf(info_str, "nume fisier: %s\n", filename);

    if (strstr(filename, ".bmp") != NULL)
    {
        sprintf(info_str + strlen(info_str), "inaltime: %u\nlungime: %u\n", info->inaltime, info->lungime);
    } else
        sprintf(info_str + strlen(info_str), "inaltime: N/A\nlungime: N/A\n");

    sprintf(info_str + strlen(info_str), "dimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
            info->dim, info->user_id, info->timp_modif, info->legaturi, info->user_perm, info->group_perm, info->other_perm);

    if (write(info_file, info_str, strlen(info_str)) == -1)
    {
        perror("Eroare la scrierea in fisierul de statistici.\n");
        close(info_file);
        exit(EXIT_FAILURE);
    }

    if (close(info_file) == -1)
    {
        perror("Eroare la inchiderea fisierului de statistici.\n");
        exit(EXIT_FAILURE);
    }
    free(info->user_id);
}

void argument_director(const char *dir_name)
{
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului.\n");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char file_name[MAX_LENGTH];
        snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, entry->d_name);

        struct stat file_info;
        if (lstat(file_name, &file_info) == -1) {
            perror("Eroare la obtinerea informatiilor despre fisier.\n");
            continue;
        }

        BMP_info info;
        if (S_ISREG(file_info.st_mode) && strstr(entry->d_name, ".bmp") != NULL) //fisier bmp
        {
            readBMP_info(file_name, &info);
            strcpy(info.user_perm, "RWX");
            strcpy(info.group_perm, "R--");
            strcpy(info.other_perm, "---");
            afisarea_info(entry->d_name, &info);
        } 
        else if (S_ISREG(file_info.st_mode)) // fisier obisnuit
        {
            info.dim = file_info.st_size;
            info.legaturi = file_info.st_nlink;
            struct tm *tm_info = localtime(&file_info.st_mtime);

            if (tm_info == NULL)
            {
                perror("Eroare la detectarea timpului local.\n");
                continue;
            }

            if (strftime(info.timp_modif, sizeof(info.timp_modif), "%d.%m.%Y", tm_info) == 0) {
                fprintf(stderr, "Eroare la formatarea timpului.\n");
                continue;
            }

            info.user_id = malloc(strlen("123") + 1);
            if (info.user_id == NULL)
            {
                perror("Eroare la alocarea memoriei pentru UserId.\n");
                continue;
            }
            strcpy(info.user_id, "123");

            strcpy(info.user_perm, "RWX");
            strcpy(info.group_perm, "R--");
            strcpy(info.other_perm, "---");

            afisarea_info(entry->d_name, &info);
            free(info.user_id);
        } 
        else if (S_ISDIR(file_info.st_mode)) // este un director
        {
            printf("nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: RWX\ndrepturi de acces grup: R--\ndrepturi de acces altii: ---\n",
                    entry->d_name, file_info.st_uid);
        } 
        else if (S_ISLNK(file_info.st_mode)) // este o legatura simbolica
        {
            char target[MAX_LENGTH];
            ssize_t len = readlink(file_name, target, sizeof(target)-1);
            if (len != -1) {
                target[len] = '\0';

            printf("nume legatura: %s\ndimensiune: %ld\ndimensiune fisier: %ld\ndrepturi de acces user: RWX\ndrepturi de acces grup: R--\ndrepturi de acces altii: ---\n",
                entry->d_name, (long)file_info.st_size, (long)file_info.st_size);
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <director_intrare>\n", argv[0]);
        return EXIT_FAILURE;
    }

    argument_director(argv[1]);

    return EXIT_SUCCESS;
}
