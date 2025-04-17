#include "config.h"
char *read_file(const char *filename, size_t *read_size) {
	*read_size = 0;
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    // Move to end to get size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate memory for the file content
    char *buffer = malloc(size + 1); // +1 for null terminator
    if (!buffer) {
        perror("malloc");
        fclose(fp);
        return NULL;
    }

    // Read the file
    *read_size = fread(buffer, 1, size, fp);
    buffer[*read_size] = '\0'; // null-terminate it just in case

    fclose(fp);
    return buffer;
}
