#include <stdlib.h>
#include <stdio.h>

#define TYPE int
#define EMPTY_VAL 0
#define PRINTER(data) printf("%d ", data);

typedef struct node {
    TYPE data;
    struct node *next;
} node_t;

typedef struct list {
    node_t *header;
} single_list_t;

node_t *create_node(TYPE val) {
    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->data = val; 
    node->next = NULL;
    return node;
}

single_list_t *init_single_list(void) {
    single_list_t *list = (single_list_t *)malloc(sizeof(single_list_t));
    list->header = create_node(EMPTY_VAL);
    return list;
}

void destroy_single_list(single_list_t *list) {
    node_t *current = list->header;
    node_t *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

node_t *insert_after(node_t *node, TYPE new_data) {
    node_t *new_node = create_node(new_data);
    node_t *prev_next = node->next;
    node->next = new_node;
    new_node->next = prev_next;
    return new_node;
}

TYPE delete_after(node_t *node) {
    node_t *next = node->next;
    if (next) {
        node->next = next->next;
        TYPE val = next->data;
        free(next);
        return val;
    }
    return EMPTY_VAL;
}

void print_single_list(single_list_t *list) {
    node_t *current = list->header->next;
    printf("List content: \n");
    while (current != NULL) {
        PRINTER(current->data);
        current = current->next;
    }
    printf("End of List\n");
}

int main(void) {
    // simple test for single_list
    single_list_t *list = init_single_list();
    node_t *node = list->header;
    node_t *store[10];
    for (int i = 0; i < 10; i++) {
        node = insert_after(node, i);
        store[i] = node;
    }
    print_single_list(list);
    for (int i = 0; i < 10; i+=2) {
        delete_after(store[i]);
    }
    print_single_list(list);
    destroy_single_list(list);
    return 0;
}