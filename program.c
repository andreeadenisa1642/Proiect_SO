#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <math.h>
#include <stdint.h>

typedef struct
{
    unsigned int inaltime, lungime, dim;
    int user_id;
    char timp_modif[20];
    int legaturi;
    char permissions[200];
} BMP_info;

char *permisiuni(char *str, mode_t mod)
{
    char cont[100];

    // user
    strcat(str, "drepturi de acces user: ");
    sprintf(cont, "%s%s%s\n", mod & S_IRUSR ? "R" : "-", mod & S_IWUSR ? "W" : "-", mod & S_IXUSR ? "X" : "-");
    strcat(str, cont);

    // grup
    strcat(str, "drepturi de acces grup: ");
    sprintf(cont, "%s%s%s\n", mod & S_IRGRP ? "R" : "-", mod & S_IWGRP ? "W" : "-", mod & S_IXGRP ? "X" : "-");
    strcat(str, cont);

    // altii
    strcat(str, "drepturi de acces altii: ");
    sprintf(cont, "%s%s%s\n", mod & S_IROTH ? "R" : "-", mod & S_IWOTH ? "W" : "-", mod & S_IXOTH ? "X" : "-");
    strcat(str, cont);

    return str;
}

void readBMP_info(const char *filename, BMP_info *info, char *caracter)
{

    struct stat fis_info;
    if (lstat(filename, &fis_info) == -1)
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

    char str[200];

    if (S_ISDIR(fis_info.st_mode))
    {
        info->lungime = 0;
        info->inaltime = 0;

        info->user_id = fis_info.st_uid;

        strcpy(info->permissions, permisiuni(str, fis_info.st_mode));
    }
    else if (S_ISLNK(fis_info.st_mode))
    {
        info->lungime = 0;
        info->inaltime = 0;

        info->user_id = fis_info.st_uid;
        strcpy(info->permissions, permisiuni(str, fis_info.st_mode));
    }
    else if (strstr(filename, ".bmp") != NULL)
    {
        int bmp_file = open(filename, O_RDONLY);
        if (bmp_file == -1)
        {
            perror("Eroare la deschiderea fisierului BMP.\n");
            exit(EXIT_FAILURE);
        }

        unsigned char f_header[54];
        if (read(bmp_file, f_header, sizeof(f_header)) == -1)
        {
            perror("Eroare la citirea header-ului fisierului BMP.\n");
            close(bmp_file);
            exit(EXIT_FAILURE);
        }

        info->user_id = fis_info.st_uid;
        strcpy(info->permissions, permisiuni(str, fis_info.st_mode));

        info->lungime = *(unsigned int *)&f_header[18];
        info->inaltime = *(unsigned int *)&f_header[22];

        if (close(bmp_file) == -1)
        {
            perror("Eroare la inchiderea fisierului BMP.\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int pfd[2];
        if (pipe(pfd) < 0)
        {
            printf("Eroare pipe.\n");
            exit(EXIT_FAILURE);
        }
        info->lungime = 0;
        info->inaltime = 0;
/*
        info->user_id = fis_info.st_uid;
        strcpy(info->permissions, permisiuni(str, fis_info.st_mode));
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        execlp("cat", "cat", filename, NULL);
        perror("Eroare la primul copil.\n");
        exit(EXIT_FAILURE);
        int pid = fork();
        if (pid < 0)
        {
            perror("Eroare la creare copil");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            close(pfd[1]);
            dup2(pfd[0], STDIN_FILENO);
            execlp("/bin/bash", "/bin/bash", "script.sh", caracter , NULL);
        }*/
    }
}

void afisarea_info(const char *filename, const BMP_info *info, int info_file)
{
    char info_str[512];

    if (S_ISDIR(info->dim))
    {
        sprintf(info_str, "nume director: %s\nidentificatorul utilizatorului: %d\n %s\n\n",
                filename, info->user_id, info->permissions);
    }
    else if (S_ISLNK(info->dim))
    {
        char target[1024];
        ssize_t target_size = readlink(filename, target, sizeof(target) - 1);
        if (target_size == -1)
        {
            perror("Eroare la citirea targetului pentru legatura simbolica.\n");
            exit(EXIT_FAILURE);
        }
        target[target_size] = '\0';

        sprintf(info_str, "nume legatura: %s\ndimensiune: %u\ndimensiune fisier: %u\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\n%s\n\n",
                filename, info->dim, info->dim, info->user_id, info->timp_modif, info->permissions);
    }
    else
    {
        sprintf(info_str, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %u\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\n%s\n\n",
                filename, info->inaltime, info->lungime, info->dim, info->user_id, info->timp_modif, info->legaturi,
                info->permissions);
    }

    if (write(info_file, info_str, strlen(info_str)) == -1)
    {
        perror("Eroare la scrierea in fisierul de statistici.\n");
        close(info_file);
        exit(EXIT_FAILURE);
    }
}

bool has_bmp_extension(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    return (ext != NULL && strcmp(ext, ".bmp") == 0);
}

void process_bmp_conversion(const char *bmp_path)
{
    int inaltime, latime;
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("Eroare la crearea procesului fiu pentru conversie .bmp.\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        int bmp_file = open(bmp_path, O_RDWR);
        if (bmp_file == -1)
        {
            perror("Eroare la deschiderea fisierului .bmp pentru conversie.\n");
            exit(EXIT_FAILURE);
        }

        lseek(bmp_file, 18, SEEK_SET);
        // citim latime inaltime
        read(bmp_file, &inaltime, 4); // trece la urm 4 bytes
        read(bmp_file, &latime, 4);
        lseek(bmp_file, -18, SEEK_CUR);
        lseek(bmp_file, 54, SEEK_SET);

        unsigned char pixel[3];

        for (int i = 0; i < latime * inaltime; i++)
        {
            uint8_t colors[3];
            read(bmp_file, colors, 3);
            uint8_t p_gri = 0.299 * colors[2] + 0.587 * colors[1] + 0.114 * colors[0];

            // Set all RGB values to the grayscale value
            colors[0] = p_gri;
            colors[1] = p_gri;
            colors[2] = p_gri;

            // mut cursor inapoi pentru a seta valorile pe gri
            lseek(bmp_file, -3, SEEK_CUR);
            write(bmp_file, colors, 3);
        }

        if (close(bmp_file) == -1)
        {
            perror("Eroare la inchiderea fisierului .bmp pentru conversie.\n");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
    else
    { // Proces părinte
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf("S-a incheiat procesul pentru conversie .bmp cu PID-ul %d si codul %d.\n", getpid(), WEXITSTATUS(status));
        }
        else
        {
            printf("Procesul pentru conversie .bmp cu PID-ul %d a terminat cu eroare.\n", getpid());
        }
    }
}

void process_entry(const char *entry_path, int info_file, const char *output_dir, char *caracter)
{
    char path[1024], str[200];
    struct stat fis_info;
    stat(entry_path, &fis_info);
    snprintf(path, sizeof(path), "%s/%s", output_dir, entry_path);

    BMP_info *info = malloc(sizeof(BMP_info));
    readBMP_info(entry_path, info, caracter);
    // strcpy(info->permissions, permisiuni(str, fis_info.st_mode));
    afisarea_info(entry_path, info, info_file);

    if (has_bmp_extension(entry_path))
    {
        process_bmp_conversion(entry_path);
    }
}
void process_directory(const char *dirname, int info_file, const char *output_dir, char *caracter)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(dirname)))
    {
        perror("Eroare la deschiderea directorului.\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (!entry->d_name || entry->d_name[0] == '.')
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        process_entry(path, info_file, output_dir, caracter);
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <director_intrare> <director_iesire>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (access(argv[1], F_OK) == -1)
    {
        perror("Eroare: Directorul de intrare nu exista.\n");
        return EXIT_FAILURE;
    }

    if (access(argv[2], F_OK) == -1)
    {
        if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            perror("Eroare la crearea directorului de iesire.\n");
            return EXIT_FAILURE;
        }
    }

    char statistica_path[1024];
    snprintf(statistica_path, sizeof(statistica_path), "%s/statistica.txt", argv[2]);
    int info_file = open(statistica_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (info_file == -1)
    {
        perror("Eroare la deschiderea fisierului de statistici.\n");
        return EXIT_FAILURE;
    }

    process_directory(argv[1], info_file, argv[2], argv[3]);

    if (close(info_file) == -1)
    {
        perror("Eroare la inchiderea fisierului de statistici.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}