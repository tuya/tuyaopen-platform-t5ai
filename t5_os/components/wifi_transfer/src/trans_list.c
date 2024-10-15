
#include <os/os.h>
#include <string.h>
#include <trans_list.h>
#include <common/bk_assert.h>


void trans_list_init(struct trans_list *list)
{
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    list->first = NULL;
    list->last = NULL;

    GLOBAL_INT_RESTORE();
}

void trans_list_push_back(struct trans_list *list,
                       struct trans_list_hdr *list_hdr)
{
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    // Sanity check
    BK_ASSERT(list_hdr != NULL);

    // check if list is empty
    if (trans_list_is_empty(list))
    {
        // list empty => pushed element is also head
        if (list_hdr == (struct trans_list_hdr *)0xa5a5a5a5)
			BK_ASSERT(0);
        list->first = list_hdr;
    }
    else
    {
	if (list->first == (struct trans_list_hdr *)0xa5a5a5a5)
			BK_ASSERT(0);
        // list not empty => update next of last
        list->last->next = list_hdr;
    }

    // add element at the end of the list
    list->last = list_hdr;
    list_hdr->next = NULL;

    GLOBAL_INT_RESTORE();
}

void trans_list_push_front(struct trans_list *list,
                        struct trans_list_hdr *list_hdr)
{
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    // Sanity check
    BK_ASSERT(list_hdr != NULL);

    // check if list is empty
    if (trans_list_is_empty(list))
    {
        // list empty => pushed element is also head
        list->last = list_hdr;
    }

    // add element at the beginning of the list
    list_hdr->next = list->first;
    list->first = list_hdr;

    GLOBAL_INT_RESTORE();
}

struct trans_list_hdr *trans_list_pop_front(struct trans_list *list)
{
    struct trans_list_hdr *element;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();

    // check if list is empty
    element = list->first;
    if (element != NULL)
    {
        // The list isn't empty : extract the first element
        list->first = list->first->next;
        if (list->last == element)
            list->last = NULL;
    }

    GLOBAL_INT_RESTORE();

    return element;
}

/// @}
