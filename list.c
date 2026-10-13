/* Linked list operations use one-based positions, as in Lab 1. */
#include "list.h"
#include <stdio.h>
#include <stdlib.h>

list_t *list_alloc(void) { return calloc(1, sizeof(list_t)); }
node_t *getNode(elem value) {
    node_t *n = malloc(sizeof(*n));
    if (n) { n->value = value; n->next = NULL; }
    return n;
}
void list_free(list_t *l) {
    if (!l) return;
    node_t *n = l->head;
    while (n) { node_t *next = n->next; free(n); n = next; }
    free(l);
}
int list_length(list_t *l) {
    int count = 0;
    for (node_t *n = l ? l->head : NULL; n; n = n->next) ++count;
    return count;
}
/* Locate a link so head and interior changes follow the same rules. */
static node_t **link_at(list_t *l, int index) {
    if (!l || index < 1) return NULL;
    node_t **link = &l->head;
    while (--index && *link) link = &(*link)->next;
    return index == 0 ? link : NULL;
}
void list_add_at_index(list_t *l, elem value, int index) {
    node_t **link = link_at(l, index);
    if (!link) return;
    node_t *n = getNode(value);
    if (!n) return;
    n->next = *link; *link = n;
}
void list_add_to_front(list_t *l, elem value) { list_add_at_index(l, value, 1); }
void list_add_to_back(list_t *l, elem value) { list_add_at_index(l, value, list_length(l) + 1); }
elem list_remove_at_index(list_t *l, int index) {
    node_t **link = link_at(l, index);
    if (!link || !*link) return -1;
    node_t *n = *link;
    elem value = n->value;
    *link = n->next; free(n);
    return value;
}
elem list_remove_from_front(list_t *l) { return list_remove_at_index(l, 1); }
elem list_remove_from_back(list_t *l) { return list_remove_at_index(l, list_length(l)); }
elem list_get_elem_at(list_t *l, int index) {
    node_t **link = link_at(l, index);
    return link && *link ? (*link)->value : -1;
}
int list_get_index_of(list_t *l, elem value) {
    int index = 1;
    for (node_t *n = l ? l->head : NULL; n; n = n->next, ++index)
        if (n->value == value) return index;
    return -1;
}
bool list_is_in(list_t *l, elem value) { return list_get_index_of(l, value) != -1; }
/* Caller owns the returned string, sized for every integer and separator. */
char *listToString(list_t *l) {
    size_t size = 5;
    for (node_t *n = l ? l->head : NULL; n; n = n->next)
        size += snprintf(NULL, 0, "%d->", n->value);
    char *text = malloc(size);
    if (!text) return NULL;
    size_t used = 0;
    for (node_t *n = l ? l->head : NULL; n; n = n->next)
        used += snprintf(text + used, size - used, "%d->", n->value);
    snprintf(text + used, size - used, "NULL");
    return text;
}
void list_print(list_t *l) {
    char *text = listToString(l);
    if (text) { puts(text); free(text); }
}
