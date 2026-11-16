#include "hdf/reader.h"
#include <string.h>
char *mysofa_strdup(const char *text) {
    size_t n=strlen(text)+1;char *out=malloc(n);memcpy(out,text,n);return out;
}
