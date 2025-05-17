#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_PATH 1024
#define LOG_PATH "/var/log/it24.log"

const char *base_dir = "/mnt/source";
char log_path[] = LOG_PATH;

void log_activity(const char *type, const char *path) {
    FILE *logf = fopen(log_path, "a");
    if (logf) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);

        va_list args;
        va_start(args, format);
        vfprintf(log, format, args);
        va_end(args);

        fprintf(log, "\n");
        fclose(log);
    }
}

int is_dangerous(const char *name) {
    return strstr(name, "nafis") || strstr(name, "kimcun");
}

void reverse_name(const char *src, char *dest) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; i++)
        dest[i] = src[len - 1 - i];
    dest[len] = '\0';
}

void apply_rot13(char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if ('a' <= buf[i] && buf[i] <= 'z')
            buf[i] = 'a' + (buf[i] - 'a' + 13) % 26;
        else if ('A' <= buf[i] && buf[i] <= 'Z')
            buf[i] = 'A' + (buf[i] - 'A' + 13) % 26;
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);
    return lstat(fullpath, stbuf);
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);

    dp = opendir(fullpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        char shown_name[NAME_MAX];
        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, shown_name);
            log_activity("REVERSE", de->d_name);
            log_activity("ALERT", de->d_name);
        } else {
            strcpy(shown_name, de->d_name);
        }

        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, shown_name, &st, 0);
    }

    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);

    int res = open(fullpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd;
    int res;

    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);

    fd = open(fullpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else if (strstr(path, ".txt") && !is_dangerous(path)) {
        apply_rot13(buf, res);
        log_activity("ENCRYPT", path);
    }

    close(fd);
    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
