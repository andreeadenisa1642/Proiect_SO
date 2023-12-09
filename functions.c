#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

// structura cu info despre BMP
typedef struct {
    unsigned int inaltime, lungime, dim;
    char *user_id;
    char timp_modif[20];
    int legaturi;
    char user_perm[4];
    char group_perm[4];
    char other_perm[4];
} BMP_info;

// citire info despre BMP
void readBMP_info(const char *filename, BMP_info *info) {
    struct stat fis_info;

    if (stat(filename, &fis_info) == -1) {
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
    strcpy(info->user_id, "123");

    info->lungime = *(unsigned int *)&f_header[18];
    info->inaltime = *(unsigned int *)&f_header[22];

    if (close(bmp_file) == -1) {
        perror("Eroare la inchiderea fisierului BMP.\n");
        exit(EXIT_FAILURE);
    }
}

// scrierea info in statistica.txt
void afisarea_info(const char *filename, const BMP_info *info, int is_bmp) {
    int info_file = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (info_file == -1) {
        perror("Eroare la deschiderea fisierului de statistici.\n");
        exit(EXIT_FAILURE);
    }

    char info_str[512];

    if (is_bmp) {
        sprintf(info_str, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
                filename, info->inaltime, info->lungime, info->dim, info->user_id, info->timp_modif, info->legaturi,
                info->user_perm, info->group_perm, info->other_perm);
    } else {
        sprintf(info_str, "nume fisier: %s\ndimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
                filename, info->dim, info->user_id, info->timp_modif,
                info->user_perm, info->group_perm, info->other_perm);
    }

    if (write(info_file, info_str, strlen(info_str)) == -1) {
        perror("Eroare la scrierea in fisierul de statistici.\n");
        close(info_file);
        exit(EXIT_FAILURE);
    }

    if (close(info_file) == -1) {
        perror("Eroare la inchiderea fisierului de statistici.\n");
        exit(EXIT_FAILURE);
    }

    free(info->user_id);
}

// functie pentru a verifica daca un fisier are extensia .bmp
int is_bmp_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (dot && strcmp(dot, ".bmp") == 0) {
        return 1; // Este fisier BMP
    }
    return 0; // Nu este fisier BMP
}

// functie pentru a verifica daca un fisier este un link simbolic
int is_symbolic_link(const char *filename) {
    struct stat buf;
    if (lstat(filename, &buf) != -1 && S_ISLNK(buf.st_mode)) {
        return 1; // Este link simbolic
    }
    return 0; // Nu este link simbolic
}

// parcurgere director si scrierea informatiilor
void parcurge_director(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului.\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

            BMP_info info;

            // Verifica tipul fisierului
            if (is_symbolic_link(file_path)) {
                // Este link simbolic
                struct stat link_info;
                lstat(file_path, &link_info);

                info.dim = link_info.st_size;

                // Obtine informatii despre fisierul target
                char target_path[512];
                ssize_t len = readlink(file_path, target_path, sizeof(target_path) - 1);
                if (len != -1) {
                    target_path[len] = '\0';
                    struct stat target_info;
                    if (stat(target_path, &target_info) != -1) {
                        afisarea_info(file_path, &info, 0);
                    }
                }
            } else if (is_bmp_file(file_path)) {
                // Este fisier BMP
                readBMP_info(file_path, &info);
                afisarea_info(file_path, &info, 1);
            } else {
                // Este fisier obisnuit fara extensia .bmp
                struct stat fis_info;
                if (stat(file_path, &fis_info) != -1) {
                    info.dim = fis_info.st_size;

                    info.user_id = malloc(strlen("123") + 1);
                    if (info.user_id == NULL) {
                        perror("Eroare la alocarea memoriei pentru UserId.\n");
                        exit(EXIT_FAILURE);
                    }
                    strcpy(info.user_id, "123");

                    strcpy(info.user_perm, "RWX");
                    strcpy(info.group_perm, "R--");
                    strcpy(info.other_perm, "---");

                    struct tm *tm_info;
                    tm_info = localtime(&fis_info.st_mtime);

                    if (tm_info != NULL && strftime(info.timp_modif, sizeof(info.timp_modif), "%d.%m.%Y", tm_info) == 0) {
                        fprintf(stderr, "Eroare la formatarea timpului.\n");
                        exit(EXIT_FAILURE);
                    }

                    afisarea_info(file_path, &info, 0);

                    //free(info.user_id);
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <director_intrare>\n", argv[0]);
        return EXIT_FAILURE;
    }

    parcurge_director(argv[1]);

    return EXIT_SUCCESS;
}
