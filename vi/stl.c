#include "stl.h"
#include "vi.h"

#include "../user.h"

struct list* new_list() {
  struct list* lst = (struct list*) malloc(sizeof(list));
  lst->head = lst->tail = NULL;
  lst->size = 0;
  return lst;
}

void
push_back(struct list* lst, int data)
{
  struct int_node* node = (struct int_node*) malloc(sizeof(int_node));
  node->data = data;
  if (lst->tail == NULL)
    lst->head = lst->tail = node;
  else {
    lst->tail->next = node;
    lst->tail = node;
  }
  lst->size ++;
}