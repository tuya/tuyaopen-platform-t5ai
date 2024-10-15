#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef void *(*alloc_fn)(size_t size);
typedef void (*free_fn)(void *ptr);

typedef struct
{
	alloc_fn alloc;
	free_fn free;
} allocator_t;

extern const allocator_t allocator_malloc;
extern const allocator_t allocator_calloc;


struct media_list_node_t;
typedef struct media_list_node_t media_list_node_t;

struct media_list_t;
typedef struct media_list_t media_list_t;

typedef void (*media_list_free_cb)(void *data);
typedef bool (*media_list_iter_cb)(void *data, void *context);
media_list_t *media_list_new(media_list_free_cb callback);
void media_list_free(media_list_t *media_list);
bool media_list_is_empty(const media_list_t *media_list);
bool media_list_contains(const media_list_t *media_list, const void *data);
size_t media_list_length(const media_list_t *media_list);
void *media_list_front(const media_list_t *media_list);
void *media_list_back(const media_list_t *media_list);
media_list_node_t *media_list_back_node(const media_list_t *media_list);
bool media_list_insert_after(media_list_t *media_list, media_list_node_t *prev_node, void *data);
bool media_list_prepend(media_list_t *media_list, void *data);
bool media_list_append(media_list_t *media_list, void *data);
bool media_list_remove(media_list_t *media_list, void *data);
void media_list_clear(media_list_t *media_list);
media_list_node_t *media_list_foreach(const media_list_t *media_list, media_list_iter_cb callback,
                                      void *context);
media_list_node_t *media_list_begin(const media_list_t *media_list);
media_list_node_t *media_list_end(const media_list_t *media_list);
media_list_node_t *media_list_next(const media_list_node_t *node);
void *media_list_node(const media_list_node_t *node);
void *media_list_foreach_pop(media_list_t *media_list, media_list_iter_cb callback,
                             void *context);

