#include "alias.h"
#include "utils.h"
#include <stdio.h>   // for printf
#include <stdlib.h>  // for free, NULL, malloc
#include <string.h>  // for strcmp


Alias *alias_table[TABLE_SIZE];

unsigned int hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; 
    return hash % TABLE_SIZE;
}

Alias *create_alias(const char *name, const char *value) {
    Alias *new_alias = malloc(sizeof(Alias));
    new_alias->name = dupstr(name);
    new_alias->value = dupstr(value);
    new_alias->next = NULL;
    return new_alias;
}

void add_alias(const char *name, const char *value) {
    unsigned int index = hash(name);
    Alias *head = alias_table[index];

    while (head) {
        if (strcmp(head->name, name) == 0) {
            free(head->value);
            head->value = dupstr(value);
            return;
        }
        head = head->next;
    }

    Alias *new_alias = create_alias(name, value);
    new_alias->next = alias_table[index];
    alias_table[index] = new_alias;
}

const char *find_alias(const char *name) {
    unsigned int index = hash(name);
    Alias *head = alias_table[index];

    while (head) {
        if (strcmp(head->name, name) == 0)
            return head->value;
        head = head->next;
    }
    return NULL;
}

void print_aliases() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Alias *head = alias_table[i];
        while (head) {
            printf("alias %s='%s'\n", head->name, head->value);
            head = head->next;
        }
    }
}
void unalias(const char *name) {
    unsigned int index = hash(name);
    Alias *head = alias_table[index];
    Alias *prev = NULL;
    while (head) {
        if (strcmp(head->name, name) == 0) {
            if (prev) {
                prev->next = head->next; 
            } else {
                alias_table[index] = head->next; 
            }

            free(head->name);
            free(head->value);
            free(head);
            return;
        }
        prev = head;
        head = head->next;
    }
    printf("Alias '%s' not found.\n", name);
}
