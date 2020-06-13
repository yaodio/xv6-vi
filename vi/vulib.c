#include "vulib.h"

char
compare(char* a, const char* b)
{
  int i = 0;
  while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i])
    i++;
  return (a[i] == '\0' && b[i] == '\0');
}