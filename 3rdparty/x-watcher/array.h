#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#include <stdlib.h>

struct _ArrayHeader {
   size_t count, capacity;
};

#define ARRAY_INITIAL_SIZE 8

#define _arr_header(a) ((struct _ArrayHeader*)(a) - 1)

#define arr_init(a) arr_init_n((a), ARRAY_INITIAL_SIZE)

#define arr_init_n(a, n) do { \
   struct _ArrayHeader *header; \
   header = malloc(sizeof(*header) + (sizeof(*(a)) * (n))); \
   header->count    = 0; \
   header->capacity = (n); \
   (a) = (void*)(header + 1); \
} while(0)

#define arr_count(a)    (_arr_header(a)->count)
#define arr_capacity(a) (_arr_header(a)->capacity)

#define arr_back(a) ((a)[arr_count(a) - 1])
#define arr_pop(a)  ((a)[_arr_header(a)->count--])

#define arr_reserve(a, n) do { \
   if(n <= arr_capacity(a)) break; \
   struct _ArrayHeader *header = _arr_header(a); \
   header->capacity = n; \
   (a) = (void*)((struct _ArrayHeader*)realloc( \
      header, sizeof(*header) + (sizeof(*(a)) * (n))) + 1); \
} while(0)

#define arr_resize(a, n) do { \
   arr_reserve((a), (n)); \
   _arr_header(a)->count = n; \
} while(0)

#define arr_resize_zero(a, n) do { \
   size_t initial_count = arr_count(a); \
   arr_resize((a), (n)); \
   if(arr_count(a) > initial_count) \
      memset( \
         &(a)[initial_count], 0, \
         (arr_count(a) - initial_count) * sizeof(*a)); \
} while(0)

// Take a vararg list to support compound literals
#define arr_add(a, ...) do { \
   struct _ArrayHeader *header = _arr_header(a); \
   if(header->count == header->capacity) \
      arr_reserve((a), header->capacity << 1); \
   (a)[_arr_header(a)->count++] = (__VA_ARGS__); \
} while(0)

#define arr_free(a) do { \
   free(_arr_header(a)); \
} while(0)

#define arr_find(a, val, idx) do { \
   *idx = -1; \
   for(size_t i = 0; i < arr_count((a)); i++) { \
      if((a)[i] == val) { \
         *idx = i; \
         break; \
      } \
   } \
} while(0)

#endif


