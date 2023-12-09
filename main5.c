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

typedef struct {
    unsigned int inaltime, lungime, dim;
    int user_id;
    char timp_modif[20];
    int legaturi;
    char user_perm[4];
    char group_perm[4];
    char other_perm[4];
} BMP_info;

char *permisiuni(char *str, mode_t mod) {
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

void readBMP_info(const char *filename, BMP_info *info) {
    struct stat fis_info;

    if (lstat(filename, &fis_info) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisier.\n");
        exit(EXIT_FAILURE);
    }

    info->dim = fis_info.st_size;
    info->legaturi = fis_info.st_nlink;

    struct tm *tm_info;
    tm_info = localtime(&fis_info.st_mtime);

    if (tm_info == NULL) {
        perror("Eroare la detectarea timpului local.\n");
        exit(EXIT_FAILURE);
    }

    if (strftime(info->timp_modif, sizeof(info->timp_modif), "%d.%m.%Y", tm_info) == 0) {
        fprintf(stderr, "Eroare la formatarea timpului.\n");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(fis_info.st_mode)) {
        info->lungime = 0;
        info->inaltime = 0;
        info->user_id = fis_info.st_uid;
        if (info->user_id == NULL) {
            perror("Eroare la alocarea memoriei pentru UserId.\n");
            exit(EXIT_FAILURE);
        }
        //strcpy(info->user_id, "123");

        strcpy(info->user_perm, "RWX");
        strcpy(info->group_perm, "R--");
        strcpy(info->other_perm, "---");
    } else if (S_ISLNK(fis_info.st_mode)) {
        info->lungime = 0;
        info->inaltime = 0;

        info->user_id = fis_info.st_uid;
        if (info->user_id == NULL) {
            perror("Eroare la alocarea memoriei pentru UserId.\n");
            exit(EXIT_FAILURE);
        }
        //strcpy(info->user_id, "123");

        strcpy(info->user_perm, "RWX");
        strcpy(info->group_perm, "R--");
        strcpy(info->other_perm, "---");
    } else if (strstr(filename, ".bmp") != NULL) {
        int bmp_file = open(filename, O_RDONLY);
        if (bmp_file == -1) {
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
        //strcpy(info->user_id, "123");

        info->lungime = *(unsigned int *)&f_header[18];
        info->inaltime = *(unsigned int *)&f_header[22];

        if (close(bmp_file) == -1) {
            perror("Eroare la inchiderea fisierului BMP.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        info->lungime = 0;
        info->inaltime = 0;

        //info->user_id = malloc(strlen("123") + 1);
        if (info->user_id == NULL) {
            perror("Eroare la alocarea memoriei pentru UserId.\n");
            exit(EXIT_FAILURE);
        }
        strcpy(info->user_id, "123");
    }
}

void afisarea_info(const char *filename, const BMP_info *info, int info_file) {
    char info_str[512];

    if (S_ISDIR(info->dim)) {
        sprintf(info_str, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n\n",
                filename, info->user_id, info->user_perm, info->group_perm, info->other_perm);
    } else if (S_ISLNK(info->dim)) {
        char target[1024];
        ssize_t target_size = readlink(filename, target, sizeof(target) - 1);
        if (target_size == -1) {
            perror("Eroare la citirea targetului pentru legatura simbolica.\n");
            exit(EXIT_FAILURE);
        }
        target[target_size] = '\0';

        sprintf(info_str, "nume legatura: %s\ndimensiune: %u\ndimensiune fisier: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n\n",
                filename, info->dim, info->dim, info->user_id, info->timp_modif, info->user_perm, info->group_perm, info->other_perm);
    } else {
        sprintf(info_str, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n\n",
                filename, info->inaltime, info->lungime, info->dim, info->user_id, info->timp_modif, info->legaturi,
                info->user_perm, info->group_perm, info->other_perm);
    }

    if (write(info_file, info_str, strlen(info_str)) == -1) {
        perror("Eroare la scrierea in fisierul de statistici.\n");
        close(info_file);
        exit(EXIT_FAILURE);
    }
}

bool has_bmp_extension(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext != NULL && strcmp(ext, ".bmp") == 0);
}

void process_bmp_conversion(const char *bmp_path, const char *output_dir) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Eroare la crearea procesului fiu pentru conversie .bmp.\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Proces fiu
        int bmp_file = open(bmp_path, O_RDWR);
        if (bmp_file == -1) {
            perror("Eroare la deschiderea fisierului .bmp pentru conversie.\n");
            exit(EXIT_FAILURE);
        }

        unsigned char pixel[3];
        while (read(bmp_file, pixel, sizeof(pixel)) > 0) {
            unsigned char pixel_gri = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
            lseek(bmp_file, -3, SEEK_CUR);
            write(bmp_file, &pixel_gri, sizeof(pixel_gri));
        }

        if (close(bmp_file) == -1) {
            perror("Eroare la inchiderea fisierului .bmp pentru conversie.\n");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    } else {
        // Părinte
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("S-a încheiat procesul pentru conversie .bmp cu PID-ul %d și codul %d.\n", pid, WEXITSTATUS(status));
        } else {
            printf("Procesul pentru conversie .bmp cu PID-ul %d a terminat cu eroare.\n", pid);
        }
    }
}

void process_entry(const char *entry_path, int info_file, const char *output_dir) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", output_dir, entry_path);

    BMP_info info;
    readBMP_info(entry_path, &info);
    strcpy(info.user_perm, "RWX");
    strcpy(info.group_perm, "R--");
    strcpy(info.other_perm, "---");
    afisarea_info(entry_path, &info, info_file);

    if (has_bmp_extension(entry_path)) {
        process_bmp_conversion(entry_path, output_dir);
    }
}

void process_directory(const char *dirname, int info_file, const char *output_dir) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(dirname))) {
        perror("Eroare la deschiderea directorului.\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!entry->d_name || entry->d_name[0] == '.')
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        process_entry(path, info_file, output_dir);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <director_intrare> <director_iesire>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (access(argv[1], F_OK) == -1) {
        perror("Eroare: Directorul de intrare nu exista.\n");
        return EXIT_FAILURE;
    }

    if (access(argv[2], F_OK) == -1) {
        if (mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            perror("Eroare la crearea directorului de iesire.\n");
            return EXIT_FAILURE;
        }
    }

    char statistica_path[1024];
    snprintf(statistica_path, sizeof(statistica_path), "%s/statistica.txt", argv[2]);
    int info_file = open(statistica_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (info_file == -1) {
        perror("Eroare la deschiderea fisierului de statistici.\n");
        return EXIT_FAILURE;
    }

    process_directory(argv[1], info_file, argv[2]);

    if (close(info_file) == -1) {
        perror("Eroare la inchiderea fisierului de statistici.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
