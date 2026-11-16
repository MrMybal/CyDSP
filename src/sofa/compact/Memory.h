#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *cydsp_sofa_malloc(size_t n);
void *cydsp_sofa_calloc(size_t n,size_t size);
void *cydsp_sofa_realloc(void *p,size_t n);
void cydsp_sofa_free(void *p);
void cydsp_sofa_abort(int reason);
void cydsp_sofa_step(size_t n);
void cydsp_sofa_enter(void);
void cydsp_sofa_leave(void);
#ifdef __cplusplus
}
#else
#define malloc cydsp_sofa_malloc
#define calloc cydsp_sofa_calloc
#define realloc cydsp_sofa_realloc
#define free cydsp_sofa_free
#endif
