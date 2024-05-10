#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

typedef struct {
    const char *dir_path;
    int file_count;
} DirData;

void *count_files(void *arg) {
    DirData *data = (DirData *)arg;
    struct dirent *entry;
    DIR *dir = opendir(data->dir_path);

    if (dir == NULL) {
        perror("opendir");
        pthread_exit((void *)1);
    }

    int local_file_count = 0;
    pthread_t threads[256];  // Assuming a reasonable limit on thread count
    int thread_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Skip the special directories "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", data->dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Create a new thread to count files in the subdirectory
            DirData *subdir_data = malloc(sizeof(DirData));
            subdir_data->dir_path = strdup(full_path);
            subdir_data->file_count = 0;

            if (pthread_create(&threads[thread_count], NULL, count_files, subdir_data) == 0) {
                thread_count++;
            } else {
                perror("pthread_create");
                free(subdir_data);
            }
        } else if (S_ISREG(st.st_mode)) {
            // If it's a regular file, increment the count
            local_file_count++;
        }
    }

    closedir(dir);

    // Join all threads and accumulate the file counts
    for (int i = 0; i < thread_count; i++) {
        void *result;
        pthread_join(threads[i], &result);
        if (result == (void *)1) {
            continue;
        }
        DirData *subdir_data = (DirData *)result;
        local_file_count += subdir_data->file_count;
        free((void *)subdir_data->dir_path);
        free(subdir_data);
    }

    data->file_count = local_file_count;
    pthread_exit(data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *start_dir = argv[1];
    DirData *root_data = malloc(sizeof(DirData));
    root_data->dir_path = start_dir;
    root_data->file_count = 0;

    pthread_t root_thread;
    if (pthread_create(&root_thread, NULL, count_files, root_data) != 0) {
        perror("pthread_create");
        free(root_data);
        return EXIT_FAILURE;
    }

    void *result;
    pthread_join(root_thread, &result);

    if (result == (void *)1) {
        fprintf(stderr, "Error counting files\n");
        free(root_data);
        return EXIT_FAILURE;
    }

    DirData *final_data = (DirData *)result;
    printf("Total files: %d\n", final_data->file_count);

    free((void *)final_data->dir_path);
    free(final_data);

    return EXIT_SUCCESS;
}
