
#pragma once

#include <stdbool.h>

#include <stddef.h>


/// structure of a list element header
struct trans_list_hdr
{
    /// Pointer to the next element in the list
    struct trans_list_hdr *next;
};

/// structure of a list
struct trans_list
{
    /// pointer to first element of the list
    struct trans_list_hdr *first;
    /// pointer to the last element
    struct trans_list_hdr *last;
};


/**
 ****************************************************************************************
 * @brief Initialize a list to defaults values.
 * @param[in] list           Pointer to the list structure.
 ****************************************************************************************
 */
void trans_list_init(struct trans_list *list);

/**
 ****************************************************************************************
 * @brief Add an element as last on the list.
 *
 * @param[in] list           Pointer to the list structure
 * @param[in] list_hdr       Pointer to the header to add at the end of the list
 ****************************************************************************************
 */
void trans_list_push_back(struct trans_list *list,
                       struct trans_list_hdr *list_hdr);

/**
 ****************************************************************************************
 * @brief Add an element as first on the list.
 *
 * @param[in] list           Pointer to the list structure
 * @param[in] list_hdr       Pointer to the header to add at the beginning of the list
 ****************************************************************************************
 */
void trans_list_push_front(struct trans_list *list,
                        struct trans_list_hdr *list_hdr);
/**
 ****************************************************************************************
 * @brief Extract the first element of the list.
 *
 * @param[in] list           Pointer to the list structure
 *
 * @return The pointer to the element extracted, and NULL if the list is empty.
 ****************************************************************************************
 */
struct trans_list_hdr *trans_list_pop_front(struct trans_list *list);


/**
 ****************************************************************************************
 * @brief Test if the list is empty.
 *
 * @param[in] list           Pointer to the list structure.
 *
 * @return true if the list is empty, false else otherwise.
 ****************************************************************************************
 */
inline bool trans_list_is_empty(const struct trans_list *const list)
{
    bool listempty;
    listempty = (list->first == NULL);
    return (listempty);
}


/**
 ****************************************************************************************
 * @brief Pick the first element from the list without removing it.
 *
 * @param[in] list           Pointer to the list structure.
 *
 * @return First element address. Returns NULL pointer if the list is empty.
 ****************************************************************************************
 */
static inline struct trans_list_hdr *trans_list_pick(const struct trans_list *const list)
{
    return(list->first);
}

/**
 ****************************************************************************************
 * @brief Pick the last element from the list without removing it.
 *
 * @param[in] list           Pointer to the list structure.
 *
 * @return Last element address. Returns invalid value if the list is empty.
 ****************************************************************************************
 */
inline struct trans_list_hdr *trans_list_pick_last(const struct trans_list *const list)
{
    return(list->last);
}

/**
 ****************************************************************************************
 * @brief Return following element of a list element.
 *
 * @param[in] list_hdr     Pointer to the list element.
 *
 * @return The pointer to the next element.
 ****************************************************************************************
 */
inline struct trans_list_hdr *trans_list_next(const struct trans_list_hdr *const list_hdr)
{
    return(list_hdr->next);
}


