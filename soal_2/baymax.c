#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#define CHUNK_SIZE 1024
#define RELICS_DIR "relics/"
#define LOG_FILE "activity.log"

void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

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

static int vfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char realpath[256];
        snprintf(realpath, sizeof(realpath), RELICS_DIR"%s.000", path + 1);
        if (access(realpath, F_OK) == 0) {
            stbuf->st_mode = S_IFREG | 0644;

            off_t total = 0;
            int i = 0;
            while (1) {
                snprintf(realpath, sizeof(realpath), RELICS_DIR"%s.%03d", path + 1, i++);
                FILE *f = fopen(realpath, "rb");
                if (!f) break;
                fseek(f, 0, SEEK_END);
                total += ftell(f);
                fclose(f);
            }
            stbuf->st_size = total;
            stbuf->st_nlink = 1;
        } else {
            return -ENOENT;
        }
    }
    return 0;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset; (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp = opendir(RELICS_DIR);
    if (!dp) return -errno;

    struct dirent *de;
    char lastfile[256] = "";

    while ((de = readdir(dp)) != NULL) {
        char *dot = strrchr(de->d_name, '.');
        if (!dot || strlen(dot) != 4) continue;

        char filename[256];
        strncpy(filename, de->d_name, dot - de->d_name);
        filename[dot - de->d_name] = '\0';

        if (strcmp(lastfile, filename) != 0) {
            filler(buf, filename, NULL, 0);
            strcpy(lastfile, filename);
        }
    }

    closedir(dp);
    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    char chunkpath[256];
    snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.000", path + 1);
    if (access(chunkpath, F_OK) == -1)
        return -ENOENT;
    
    write_log("READ: %s", path + 1);
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t total_read = 0;
    int chunk_index = offset / CHUNK_SIZE;
    size_t chunk_offset = offset % CHUNK_SIZE;

    while (size > 0) {
        char chunkpath[256];
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, chunk_index);

        FILE *f = fopen(chunkpath, "rb");
        if (!f) break;

        fseek(f, chunk_offset, SEEK_SET);
        size_t bytes = fread(buf + total_read, 1, CHUNK_SIZE - chunk_offset, f);
        fclose(f);

        if (bytes == 0) break;

        total_read += bytes;
        size -= bytes;
        chunk_offset = 0;
        chunk_index++;
    }

    return total_read;
}

static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t bytes_written = 0;
    char chunk_list[1024] = "";  // untuk menyimpan daftar chunk (untuk log)

    while (bytes_written < size) {
        int chunk_index = (offset + bytes_written) / CHUNK_SIZE;
        int chunk_offset = (offset + bytes_written) % CHUNK_SIZE;
        size_t write_size = CHUNK_SIZE - chunk_offset;
        if (write_size > size - bytes_written) {
            write_size = size - bytes_written;
        }

        char chunkpath[256];
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, chunk_index);

        FILE *f = fopen(chunkpath, "r+b");
        if (!f) f = fopen(chunkpath, "wb");
        if (!f) return -EACCES;

        fseek(f, chunk_offset, SEEK_SET);
        fwrite(buf + bytes_written, 1, write_size, f);
        fclose(f);

        // Tambahkan ke daftar chunk untuk log
        char chunkname[64];
        snprintf(chunkname, sizeof(chunkname), "%s.%03d", path + 1, chunk_index);
        if (!strstr(chunk_list, chunkname)) {  // hindari duplikasi jika write ke offset yg sama
            if (strlen(chunk_list) > 0) strcat(chunk_list, ", ");
            strcat(chunk_list, chunkname);
        }

        bytes_written += write_size;
    }

    write_log("CREATE: %s -> %s", path + 1, chunk_list);  // log hasil pemecahan
    return size;
}

static int vfs_unlink(const char *path) {
    int deleted = 0;
    char chunkpath[256];
    char delrange[256] = "";

    for (int i = 0;; i++) {
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, i);
        if (access(chunkpath, F_OK) != 0) break;

        if (deleted == 0)
            snprintf(delrange, sizeof(delrange), "%s.%03d", path + 1, i);
        deleted++;

        remove(chunkpath);
    }

    if (deleted > 0)
        write_log("DELETE: %s - %s.%03d", delrange, path + 1, deleted - 1);

    return 0;
}

static int vfs_truncate(const char *path, off_t size) {
    // Hapus semua chunk lama
    char chunkpath[256];
    for (int i = 0;; i++) {
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, i);
        if (access(chunkpath, F_OK) != 0) break;
        remove(chunkpath);
    }
    return 0;
}


static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char chunkpath[256];
    snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.000", path + 1);

    FILE *f = fopen(chunkpath, "wb");
    if (!f) return -EACCES;

    fclose(f);

    write_log("WRITE: %s", path + 1);
    return 0;
}

static struct fuse_operations vfs_oper = {
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .open    = vfs_open,
    .read    = vfs_read,
    .write   = vfs_write,
    .unlink  = vfs_unlink,
    .create  = vfs_create,
    .truncate = vfs_truncate,
};

int main(int argc, char *argv[]) {
    mkdir(RELICS_DIR, 0755);
    return fuse_main(argc, argv, &vfs_oper, NULL);
}
