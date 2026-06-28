#pragma once

struct path {
    char* path;
    struct path* next;
};
extern struct path* paths;

void path_initialization();
void free_path();