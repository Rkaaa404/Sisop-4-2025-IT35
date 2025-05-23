#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Metropolis module: Stored normally but characters in file content shifted by position (mod 256)
// Example: ener → eogu

// Transform path for metro module
char* metro_transform_path(const char *path, int to_source) {
    char *new_path = strdup(path);
    if (!new_path) {
        return NULL;
    }
    
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
    
    return new_path;
}

// Transform content for metro module
void metro_transform_content(char *buf, size_t size, int to_source) {
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