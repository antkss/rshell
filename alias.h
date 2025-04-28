#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define TABLE_SIZE 1000
typedef struct Alias {
    char *name;
    char *value;
    struct Alias *next; 
} Alias;
void add_alias(const char *name, const char *value);
const char *find_alias(const char *name);
void print_aliases();
unsigned int hash(const char *str);
void unalias(const char *name);



