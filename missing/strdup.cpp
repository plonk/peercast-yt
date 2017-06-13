#include <stdlib.h>
#include <string.h>

char *strdup(const char *s)
{
    char *res = (char*) malloc(strlen(s)+1);
    strcpy(res, s);
    return res;
}
