#include "path.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct path* paths = NULL;

void path_initialization() {
    paths = (struct path *) malloc(sizeof(struct path));
    if(paths == NULL) {
        fprintf(stderr, "Initial memory allocation for path values failed\n");
        return;
    }
    paths->path = strdup("/bin");
    paths->next = NULL;
    return;
}

void free_path() {
    struct path* temp = paths;
    while(temp != NULL) {
        struct path* temp2 = temp->next;
        free(temp->path);
        free(temp);
        temp = temp2;
    }
    free(paths);
}