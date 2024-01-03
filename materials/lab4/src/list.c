#include "list.h"

void list_init(list_node_t *head) {
    head->next = head; head->prev = head;
}

inline void list_add_tail(list_node_t *node, list_node_t *head) {
    list_node_t *prev = head->prev;
    prev->next = node;
    node->next = head;
    node->prev = prev;
    head->prev = node;
}

inline void list_remove(list_node_t *node) {
    list_node_t *next = node->next;
    list_node_t *prev = node->prev;
    next->prev = prev;
    prev->next = next;
}

inline int list_empty(list_node_t *head) {
    return (head->next == head);
}
