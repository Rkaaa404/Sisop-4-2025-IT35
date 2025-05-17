#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>

// Global variables
static char *source_dir = NULL;
static char *mount_dir = NULL;

// Helper functions
static char* get_full_path(const char *path, int is_source) {
    char *base_dir = is_source ? source_dir : mount_dir;
    char *full_path = malloc(strlen(base_dir) + strlen(path) + 1);
    if (!full_path) {
        return NULL;
    }
    strcpy(full_path, base_dir);
    strcat(full_path, path);
    return full_path;
}

// Determine the module type based on the path
static char* get_module_type(const char *path) {
    if (strstr(path, "/starter/") != NULL) {
        return "starter";
    } else if (strstr(path, "/metro/") != NULL) {
        return "metro";
    } else if (strstr(path, "/dragon/") != NULL) {
        return "dragon";
    } else if (strstr(path, "/blackrose/") != NULL) {
        return "blackrose";
    } else if (strstr(path, "/heaven/") != NULL) {
        return "heaven";
    } else if (strstr(path, "/youth/") != NULL) {
        return "youth";
    } else if (strstr(path, "/7sref/") != NULL) {
        return "7sref";
    }
    return NULL;
}

// Transform path based on module type
static char* transform_path(const char *path, const char *module_type, int to_source) {
    char *new_path = strdup(path);
    if (!new_path) {
        return NULL;
    }
    
    if (strcmp(module_type, "starter") == 0) {
        if (to_source) {
            // No suffix change when going from mount to source
            // Keep the path as is
        } else {
            // Add .mai suffix when going from source to mount
            char *temp = malloc(strlen(new_path) + 5); // +5 for ".mai" and null terminator
            if (temp) {
                strcpy(temp, new_path);
                strcat(temp, ".mai");
                free(new_path);
                new_path = temp;
            }
        }
    } else if (strcmp(module_type, "metro") == 0) {
        if (to_source) {
            // No suffix change when going from mount to source
            // Keep the path as is
        } else {
            // Add .ccc suffix when going from source to mount
            char *temp = malloc(strlen(new_path) + 5); // +5 for ".ccc" and null terminator
            if (temp) {
                strcpy(temp, new_path);
                strcat(temp, ".ccc");
                free(new_path);
                new_path = temp;
            }
        }
    }
    
    return new_path;
}

// Transform content based on module type
static void transform_content(char *buf, size_t size, const char *module_type, int to_source) {
    if (strcmp(module_type, "metro") == 0) {
        // For metro module, shift characters by position (mod 256)
        if (to_source) {
            // Reverse the shift when going from mount to source
            for (size_t i = 0; i < size; i++) {
                buf[i] = (buf[i] - (i % 256) + 256) % 256;
            }
        } else {
            // Apply the shift when going from source to mount
            for (size_t i = 0; i < size; i++) {
                buf[i] = (buf[i] + (i % 256)) % 256;
            }
        }
    }
}

// FUSE operations
static int maimai_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    // Handle root directory
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    char *module_type = get_module_type(path);
    if (!module_type) {
        // Check if it's a top-level directory
        char *path_copy = strdup(path);
        char *dir_name = dirname(path_copy);
        if (strcmp(dir_name, "/") == 0) {
            free(path_copy);
            char *full_path = get_full_path(path, 1);
            int res = stat(full_path, stbuf);
            free(full_path);
            return res == 0 ? 0 : -errno;
        }
        free(path_copy);
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    int res = lstat(full_path, stbuf);
    free(full_path);
    
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    
    (void) offset;
    (void) fi;
    
    char *full_path = get_full_path(path, 1);
    if (!full_path) {
        return -ENOMEM;
    }
    
    dp = opendir(full_path);
    free(full_path);
    
    if (dp == NULL) {
        return -errno;
    }
    
    char *module_type = get_module_type(path);
    
    // Add . and .. entries
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        char *entry_name = strdup(de->d_name);
        
        // Transform entry name if needed
        if (module_type && de->d_type != DT_DIR) {
            // No transformation needed for directory listing
            // Files in source directory have no suffix
        }
        
        filler(buf, entry_name, &st, 0);
        free(entry_name);
    }
    
    closedir(dp);
    return 0;
}

static int maimai_open(const char *path, struct fuse_file_info *fi) {
    char *module_type = get_module_type(path);
    if (!module_type) {
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    int res = open(full_path, fi->flags);
    free(full_path);
    
    if (res == -1) {
        return -errno;
    }
    
    close(res);
    return 0;
}

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    int fd;
    int res;
    
    (void) fi;
    
    char *module_type = get_module_type(path);
    if (!module_type) {
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    fd = open(full_path, O_RDONLY);
    free(full_path);
    
    if (fd == -1) {
        return -errno;
    }
    
    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else {
        // Transform content if needed
        transform_content(buf, res, module_type, 0);
    }
    
    close(fd);
    return res;
}

static int maimai_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    
    (void) fi;
    
    char *module_type = get_module_type(path);
    if (!module_type) {
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    fd = open(full_path, O_WRONLY);
    if (fd == -1) {
        free(full_path);
        return -errno;
    }
    
    // Transform content if needed
    char *transformed_buf = malloc(size);
    if (!transformed_buf) {
        close(fd);
        free(full_path);
        return -ENOMEM;
    }
    
    memcpy(transformed_buf, buf, size);
    transform_content(transformed_buf, size, module_type, 1);
    
    res = pwrite(fd, transformed_buf, size, offset);
    free(transformed_buf);
    
    if (res == -1) {
        res = -errno;
    }
    
    close(fd);
    free(full_path);
    return res;
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int fd;
    
    char *module_type = get_module_type(path);
    if (!module_type) {
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    fd = open(full_path, fi->flags, mode);
    free(full_path);
    
    if (fd == -1) {
        return -errno;
    }
    
    fi->fh = fd;
    return 0;
}

static int maimai_unlink(const char *path) {
    char *module_type = get_module_type(path);
    if (!module_type) {
        return -ENOENT;
    }
    
    // Transform path if needed
    char *transformed_path = transform_path(path, module_type, 1);
    if (!transformed_path) {
        return -ENOMEM;
    }
    
    char *full_path = get_full_path(transformed_path, 1);
    free(transformed_path);
    
    if (!full_path) {
        return -ENOMEM;
    }
    
    int res = unlink(full_path);
    free(full_path);
    
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static int maimai_mkdir(const char *path, mode_t mode) {
    char *full_path = get_full_path(path, 1);
    if (!full_path) {
        return -ENOMEM;
    }
    
    int res = mkdir(full_path, mode);
    free(full_path);
    
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static int maimai_rmdir(const char *path) {
    char *full_path = get_full_path(path, 1);
    if (!full_path) {
        return -ENOMEM;
    }
    
    int res = rmdir(full_path);
    free(full_path);
    
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}

static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .open = maimai_open,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
    .mkdir = maimai_mkdir,
    .rmdir = maimai_rmdir,
};

int main(int argc, char *argv[]) {
    // Check for minimum arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s source_dir mount_point [FUSE options]\n", argv[0]);
        return 1;
    }
    
    // Get absolute paths for source and mount directories
    char source_path[PATH_MAX];
    char mount_path[PATH_MAX];
    
    // Process source directory
    if (realpath(argv[1], source_path) == NULL) {
        fprintf(stderr, "Failed to resolve source path: %s\n", argv[1]);
        return 1;
    }
    
    // Process mount point
    if (argv[2][0] == '/') {
        // Absolute path
        strncpy(mount_path, argv[2], PATH_MAX - 1);
    } else {
        // Relative path
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            fprintf(stderr, "Failed to get current working directory\n");
            return 1;
        }
        size_t cwd_len = strlen(cwd);
        size_t arg_len = strlen(argv[2]);
        
        if (cwd_len + arg_len + 2 > PATH_MAX) {
            fprintf(stderr, "Path too long\n");
            return 1;
        }
        
        snprintf(mount_path, PATH_MAX - 1, "%s/%s", cwd, argv[2]);
    }
    
    // Add trailing slash if not present
    size_t src_len = strlen(source_path);
    if (source_path[src_len - 1] != '/') {
        source_path[src_len] = '/';
        source_path[src_len + 1] = '\0';
    }
    
    size_t mnt_len = strlen(mount_path);
    if (mount_path[mnt_len - 1] != '/') {
        mount_path[mnt_len] = '/';
        mount_path[mnt_len + 1] = '\0';
    }
    
    source_dir = strdup(source_path);
    mount_dir = strdup(mount_path);
    
    if (!source_dir || !mount_dir) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    printf("Source directory: %s\n", source_dir);
    printf("Mount directory: %s\n", mount_dir);
    
    // Prepare FUSE arguments
    char **fuse_argv = malloc((argc + 4) * sizeof(char *));  // +4 for program name, -f, -o, and nonempty options
    if (!fuse_argv) {
        fprintf(stderr, "Memory allocation failed\n");
        free(source_dir);
        free(mount_dir);
        return 1;
    }
    
    // Create a new set of arguments for FUSE
    int fuse_argc = 0;
    fuse_argv[fuse_argc++] = argv[0];  // Program name
    
    // Add mount point
    fuse_argv[fuse_argc++] = mount_path;
    
    // Add foreground option if not already present
    int has_foreground = 0;
    int has_nonempty = 0;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-d") == 0 || 
            strcmp(argv[i], "-foreground") == 0 || strcmp(argv[i], "-debug") == 0) {
            has_foreground = 1;
        }
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc && strstr(argv[i+1], "nonempty") != NULL) {
            has_nonempty = 1;
        }
    }
    
    if (!has_foreground) {
        fuse_argv[fuse_argc++] = "-f";
    }
    
    // Add nonempty option if not already present
    if (!has_nonempty) {
        fuse_argv[fuse_argc++] = "-o";
        fuse_argv[fuse_argc++] = "nonempty";
    }
    
    // Copy any additional FUSE options
    for (int i = 3; i < argc; i++) {
        fuse_argv[fuse_argc++] = argv[i];
    }
    
    // Run FUSE
    int ret = fuse_main(fuse_argc, fuse_argv, &maimai_oper, NULL);
    
    free(fuse_argv);
    free(source_dir);
    free(mount_dir);
    
    return ret;
}