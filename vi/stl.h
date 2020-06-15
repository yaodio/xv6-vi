#ifndef VI_XV6_STL_H
#define VI_XV6_STL_H

typedef struct int_node {
    int data;
    struct int_node* next;
} int_node;

typedef struct list {
    struct int_node* head;
    struct int_node* tail;
    int size;
} list;

struct list* new_list();
void push_back(struct list*, int);

#endif //VI_XV6_STL_H
