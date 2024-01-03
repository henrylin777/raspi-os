#include <stddef.h>
#pragma once
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - (offsetof(type, member))))


/* list_entry() - get address of entry that contains list node
 * @node: pointer to list node
 */
#define list_entry(node, type, member) container_of(node, type, member)

/* list_first_entry() - get first entry of the list
 * @head: pointer to the head of the list
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 */
#define list_first_entry(head, type, member) list_entry((head)->next, type, member)

/* list_for_each - iterate over list nodes
 * @node: list_head pointer used as iterator
 * @head: pointer to the head of the list
 */
#define list_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)

typedef struct list_node {
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

void list_init(list_node_t *p);
void list_add_tail(list_node_t *node, list_node_t *head);
void list_remove(list_node_t *node);
int list_empty(list_node_t *head);
void list_insert(list_node_t *prev, list_node_t *node);
