#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "bk_list.h"

void *list_node_pop_edge(LIST_HEADER_T *head, uint32_t member_offset);

#define list_pop_edge(head, type, member)  list_node_pop_edge(head, offsetof(type,member))

