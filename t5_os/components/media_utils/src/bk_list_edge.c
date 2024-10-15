#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_assert.h>

#include "bk_list_edge.h"

void *list_node_pop_edge(LIST_HEADER_T *head, uint32_t member_offset)
{
	LIST_HEADER_T *pos, *n;
	void *request = NULL;

	list_for_each_safe(pos, n, head)
	{
		request = (void *)((char *)pos - member_offset);
		if (request != NULL)
		{
			list_del(pos);
			break;
		}
	}

	return request;
}



