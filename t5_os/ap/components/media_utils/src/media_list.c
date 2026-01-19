#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_assert.h>

#include "media_utils.h"
#include "media_list.h"

struct media_list_node_t
{
	struct media_list_node_t *next;
	void *data;
};

typedef struct media_list_t
{
	media_list_node_t *head;
	media_list_node_t *tail;
	size_t length;
	media_list_free_cb free_cb;
	const allocator_t *allocator;
} media_list_t;

void *osi_malloc(size_t size);
void osi_free(void *ptr);


const allocator_t allocator_malloc = {osi_malloc, osi_free};

static media_list_node_t *media_list_free_node(media_list_t *media_list, media_list_node_t *node);

// Hidden constructor, only to be used by the hash map for the allocation
// tracker.
// Behaves the same as |media_list_new|, except you get to specify the allocator.
media_list_t *media_list_new_internal(media_list_free_cb callback,
                                      const allocator_t *zeroed_allocator)
{
	media_list_t *media_list = (media_list_t *)zeroed_allocator->alloc(sizeof(media_list_t));
	if (!media_list)
	{
		return NULL;
	}

	media_list->free_cb = callback;
	media_list->allocator = zeroed_allocator;
	return media_list;
}

media_list_t *media_list_new(media_list_free_cb callback)
{
	return media_list_new_internal(callback, &allocator_malloc);
}

void media_list_free(media_list_t *media_list)
{
	if (!media_list)
	{
		return;
	}

	media_list_clear(media_list);
	media_list->allocator->free(media_list);
}

bool media_list_is_empty(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	return (media_list->length == 0);
}

bool media_list_contains(const media_list_t *media_list, const void *data)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(data != NULL);

	for (const media_list_node_t *node = media_list_begin(media_list); node != media_list_end(media_list);
	     node = media_list_next(node))
	{
		if (media_list_node(node) == data)
		{
			return true;
		}
	}

	return false;
}

size_t media_list_length(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	return media_list->length;
}

void *media_list_front(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(!media_list_is_empty(media_list));

	return media_list->head->data;
}

void *media_list_back(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(!media_list_is_empty(media_list));

	return media_list->tail->data;
}

media_list_node_t *media_list_back_node(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(!media_list_is_empty(media_list));

	return media_list->tail;
}

bool media_list_insert_after(media_list_t *media_list, media_list_node_t *prev_node, void *data)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(prev_node != NULL);
	BK_ASSERT(data != NULL);

	media_list_node_t *node = (media_list_node_t *)media_list->allocator->alloc(sizeof(media_list_node_t));
	if (!node)
	{
		return false;
	}

	node->next = prev_node->next;
	node->data = data;
	prev_node->next = node;
	if (media_list->tail == prev_node)
	{
		media_list->tail = node;
	}
	++media_list->length;
	return true;
}

bool media_list_prepend(media_list_t *media_list, void *data)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(data != NULL);

	media_list_node_t *node = (media_list_node_t *)media_list->allocator->alloc(sizeof(media_list_node_t));
	if (!node)
	{
		return false;
	}
	node->next = media_list->head;
	node->data = data;
	media_list->head = node;
	if (media_list->tail == NULL)
	{
		media_list->tail = media_list->head;
	}
	++media_list->length;
	return true;
}

bool media_list_append(media_list_t *media_list, void *data)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(data != NULL);

	media_list_node_t *node = (media_list_node_t *)media_list->allocator->alloc(sizeof(media_list_node_t));
	if (!node)
	{
		return false;
	}
	node->next = NULL;
	node->data = data;
	if (media_list->tail == NULL)
	{
		media_list->head = node;
		media_list->tail = node;
	}
	else
	{
		media_list->tail->next = node;
		media_list->tail = node;
	}
	++media_list->length;
	return true;
}

bool media_list_remove(media_list_t *media_list, void *data)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(data != NULL);

	if (media_list_is_empty(media_list))
	{
		return false;
	}

	if (media_list->head->data == data)
	{
		media_list_node_t *next = media_list_free_node(media_list, media_list->head);
		if (media_list->tail == media_list->head)
		{
			media_list->tail = next;
		}
		media_list->head = next;
		return true;
	}

	for (media_list_node_t *prev = media_list->head, *node = media_list->head->next; node;
	     prev = node, node = node->next)
		if (node->data == data)
		{
			prev->next = media_list_free_node(media_list, node);
			if (media_list->tail == node)
			{
				media_list->tail = prev;
			}
			return true;
		}

	return false;
}

void media_list_clear(media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	for (media_list_node_t *node = media_list->head; node;)
	{
		node = media_list_free_node(media_list, node);
	}
	media_list->head = NULL;
	media_list->tail = NULL;
	media_list->length = 0;
}

media_list_node_t *media_list_foreach(const media_list_t *media_list, media_list_iter_cb callback,
                                      void *context)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(callback != NULL);

	for (media_list_node_t *node = media_list->head; node;)
	{
		media_list_node_t *next = node->next;
		if (!callback(node->data, context))
		{
			return node;
		}
		node = next;
	}
	return NULL;
}


void *media_list_foreach_pop(media_list_t *media_list, media_list_iter_cb callback,
                             void *context)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(callback != NULL);
	void *data = NULL;

	if (media_list_is_empty(media_list))
	{
		return false;
	}

	if (callback(media_list->head->data, context))
	{
		data = media_list->head->data;

		media_list_node_t *next = media_list_free_node(media_list, media_list->head);
		if (media_list->tail == media_list->head)
		{
			media_list->tail = next;
		}
		media_list->head = next;

		return data;
	}

	for (media_list_node_t *prev = media_list->head, *node = media_list->head->next; node;
	     prev = node, node = node->next)
		if (callback(node->data, context))
		{
			data = node->data;
			prev->next = media_list_free_node(media_list, node);
			if (media_list->tail == node)
			{
				media_list->tail = prev;
			}
			return data;
		}

	return NULL;
}

media_list_node_t *media_list_begin(const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	return media_list->head;
}

media_list_node_t *media_list_end(UNUSED_ATTR const media_list_t *media_list)
{
	BK_ASSERT(media_list != NULL);
	return NULL;
}

media_list_node_t *media_list_next(const media_list_node_t *node)
{
	BK_ASSERT(node != NULL);
	return node->next;
}

void *media_list_node(const media_list_node_t *node)
{
	BK_ASSERT(node != NULL);
	return node->data;
}

static media_list_node_t *media_list_free_node(media_list_t *media_list, media_list_node_t *node)
{
	BK_ASSERT(media_list != NULL);
	BK_ASSERT(node != NULL);

	media_list_node_t *next = node->next;

	if (media_list->free_cb)
	{
		media_list->free_cb(node->data);
	}
	media_list->allocator->free(node);
	--media_list->length;

	return next;
}

void *osi_malloc(size_t size)
{
	return os_malloc(size);
}

void osi_free(void *ptr)
{
	os_free(ptr);
}
