#define FUSE_USE_VERSION 31
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define SOURCE_FOLDER "anomali"
#define IMAGE_FOLDER "image"
#define LOG_FILE "conversion.log"
#define DOWNLOAD_URL "https://drive.google.com/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_NAME "data.zip"

static int is_hex_char(char c) {
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

void create_image_directory() {
    struct stat st = {0};
    if (stat(IMAGE_FOLDER, &st) == -1) {
        mkdir(IMAGE_FOLDER, 0755);
    }
}

void log_conversion(const char *filename, int success) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(log, "[%s] %s: %s\n", time_str, filename, success ? "SUCCESS" : "FAILED");
        fclose(log);
    }
}

int hex_to_png(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *hex_data = malloc(size + 1);
    fread(hex_data, 1, size, fp);
    hex_data[size] = '\0';
    fclose(fp);

    size_t len = strlen(hex_data);
    char *bin = malloc(len / 2);
    size_t bin_idx = 0;
    for (size_t i = 0; i < len; i += 2) {
        if (!is_hex_char(hex_data[i]) || !is_hex_char(hex_data[i + 1])) continue;
        char byte_str[3] = {hex_data[i], hex_data[i + 1], '\0'};
        bin[bin_idx++] = (char)strtol(byte_str, NULL, 16);
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char imgname[100];
    const char *base = strrchr(filepath, '/');
    snprintf(imgname, sizeof(imgname), IMAGE_FOLDER"/%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png",
             base ? base + 1 : filepath,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    FILE *out = fopen(imgname, "wb");
    if (!out) {
        free(hex_data);
        free(bin);
        return 0;
    }
    fwrite(bin, 1, bin_idx, out);
    fclose(out);

    free(hex_data);
    free(bin);
    return 1;
}

void process_all_files() {
    create_image_directory();
    DIR *dir = opendir(SOURCE_FOLDER);
    if (!dir) return;

    struct dirent *ent;
    char path[1024];
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG && strstr(ent->d_name, ".txt")) {
            snprintf(path, sizeof(path), SOURCE_FOLDER"/%s", ent->d_name);
            int success = hex_to_png(path);
            log_conversion(ent->d_name, success);
        }
    }
    closedir(dir);
}

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void download_and_extract() {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "wget -q --show-progress -O %s \"%s\" && unzip -o %s && rm %s",
        ZIP_NAME, DOWNLOAD_URL, ZIP_NAME, ZIP_NAME);
    system(cmd);
    printf("Data berhasil diunduh dan diekstrak.\n");
}

static int x_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    return lstat(fullpath, st);
}

static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    DIR *dp;
    struct dirent *de;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);

    dp = opendir(fullpath);
    if (!dp) return -errno;

    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int x_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    int res = open(fullpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static int x_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    FILE *f = fopen(fullpath, "r");
    if (!f) return -errno;
    fseek(f, offset, SEEK_SET);
    int res = fread(buf, 1, size, f);
    fclose(f);
    return res;
}

static const struct fuse_operations operations = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open = x_open,
    .read = x_read,
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mountpoint>\n", argv[0]);
        return 1;
    }

    if (!file_exists(SOURCE_FOLDER)) {
        printf("Folder %s belum ada, mulai download...\n", SOURCE_FOLDER);
        download_and_extract();
    }

    process_all_files();

    return fuse_main(argc, argv, &operations, NULL);
}
