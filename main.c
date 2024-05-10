#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int count_files(const char *dir_path) {
    int file_count = 0;
    struct dirent *entry;
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip the special directories "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // If it's a directory, recurse into it
            file_count += count_files(full_path);
        } else if (S_ISREG(st.st_mode)) {
            // If it's a regular file, increment the count
            file_count++;
        }
    }

    closedir(dir);
    return file_count;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *start_dir = argv[1];
    int total_files = count_files(start_dir);

    if (total_files >= 0) {
        printf("Total files: %d\n", total_files);
    } else {
        fprintf(stderr, "Error counting files\n");
    }

    return EXIT_SUCCESS;
}