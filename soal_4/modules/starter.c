#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Starter module: Files saved normally, but .mai suffix added in the original dir
// In fuse_dir, the file appears without .mai

// Transform path for starter module
char* starter_transform_path(const char *path, int to_source) {
    char *new_path = strdup(path);
    if (!new_path) {
        return NULL;
    }
    
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
    
    return new_path;
}

// No content transformation for starter module
void starter_transform_content(char *buf, size_t size, int to_source) {
    // No transformation needed
    return;
}