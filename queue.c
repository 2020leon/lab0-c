#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create an element with string initialized.
 * Return NULL if could not allocate space or `s` is NULL.
 */
static element_t *alloc_helper(const char *s);

/*
 * Attempt to remove node from a queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 */
static element_t *my_q_remove(struct list_head *node, char *sp, size_t bufsize);

/*
 * Get the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return the middle node.
 *
 * Note: `head` must not be empty
 */
static struct list_head *get_mid_node(const struct list_head *head);

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    /* Malloc queue. */
    struct list_head *const q = malloc(sizeof(struct list_head));
    /* Initialize queue if `q` is not NULL. */
    if (q)
        INIT_LIST_HEAD(q);
    return q;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    element_t *i, *tmp;
    if (!l)
        return;
    list_for_each_entry_safe (i, tmp, l, list)
        q_release_element(i);
    free(l);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    element_t *element;
    if (!head)
        return false;
    element = alloc_helper(s);
    if (!element)
        return false;
    list_add(&element->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    element_t *element;
    if (!head)
        return false;
    element = alloc_helper(s);
    if (!element)
        return false;
    list_add_tail(&element->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    return !head || list_empty(head) ? NULL
                                     : my_q_remove(head->next, sp, bufsize);
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    return !head || list_empty(head) ? NULL
                                     : my_q_remove(head->prev, sp, bufsize);
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    int len = 0;
    struct list_head *li;
    if (!head || list_empty(head))
        return 0;
    list_for_each (li, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    q_release_element(my_q_remove(get_mid_node(head), NULL, 0));
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    struct list_head *i, *j;
    bool is_i_dup = false;
    if (!head)
        return false;
    i = head->next;
    for (j = i->next; j != head; j = j->next) {
        if (strcmp(list_entry(i, element_t, list)->value,
                   list_entry(j, element_t, list)->value) == 0) {
            is_i_dup = true;
            // Remove j from the queue and release it
            q_release_element(my_q_remove(j, NULL, 0));
            // Assign i to j after j is deleted
            // For the next loop, j == i->next
            j = i;
        } else {
            i = i->next;
            // Delete the last node whose string was duplicated
            if (is_i_dup) {
                q_release_element(my_q_remove(i->prev, NULL, 0));
                is_i_dup = false;
            }
        }
    }
    if (is_i_dup)
        q_release_element(my_q_remove(i, NULL, 0));
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head))
        return;
    for (struct list_head *i = head->next; i != head && i->next != head;
         i = i->next)
        list_move_tail(i->next, i);
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    for (struct list_head *i = head; i->next != head->prev; i = i->next)
        list_move(head->prev, i);
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    // Merge sort
    struct list_head *i, *j, *tmp;
    LIST_HEAD(new_head);
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    // Split the list
    list_cut_position(&new_head, head, get_mid_node(head)->prev);
    // Call recursively
    q_sort(&new_head);
    q_sort(head);
    // Insert nodes in new_head to head
    i = head->next;
    for (j = new_head.next; !list_empty(&new_head); j = tmp) {
        while (i != head && strcmp(list_entry(i, element_t, list)->value,
                                   list_entry(j, element_t, list)->value) < 0) {
            i = i->next;
        }
        if (i == head) {
            // All of the remaining elements in new_head is greater than i
            list_splice_tail_init(&new_head, i);
        } else {
            tmp = j->next;
            list_del_init(j);
            list_add_tail(j, i);
            // i->prev == j
        }
    }
}

/*
 * Create an element with string initialized.
 * Return NULL if could not allocate space or `s` is NULL.
 */
static element_t *alloc_helper(const char *s)
{
    element_t *element;
    size_t slen;
    if (!s)
        return NULL;
    element = malloc(sizeof(element_t));
    if (!element)
        return NULL;

    INIT_LIST_HEAD(&element->list);

    slen = strlen(s) + 1;
    element->value = malloc(slen);
    if (!element->value) {
        free(element);
        return NULL;
    }
    memcpy(element->value, s, slen);
    return element;
}

/*
 * Attempt to remove node from a queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 */
static element_t *my_q_remove(struct list_head *node, char *sp, size_t bufsize)
{
    element_t *const element = list_entry(node, element_t, list);
    list_del_init(node);
    if (sp && bufsize) {
        size_t min = strlen(element->value) + 1;
        min = min > bufsize ? bufsize : min;
        memcpy(sp, element->value, min);
        sp[min - 1] = '\0';
    }
    return element;
}

/*
 * Get the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return the middle node.
 *
 * Note: `head` must not be empty
 */
static struct list_head *get_mid_node(const struct list_head *head)
{
    struct list_head *i = head->next, *j = head->prev;
    while (i != j && i->next != j)
        i = i->next, j = j->prev;
    return j;
}
