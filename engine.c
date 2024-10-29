#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "engine.h"

#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

// Internal Implementation
typedef struct order {
  t_orderid id;
  t_order order;
} order_t;

typedef struct order_node {
  order_t order_obj;
  struct order_node *prev;
  struct order_node *next;
} order_node_t;

typedef struct price_level {
  t_side side;
  t_price price;
  order_node_t *header;
  order_node_t *tailer;
} price_level_t;

price_level_t *init_price_level(t_side side, t_price price) {
  price_level_t *pl = (price_level_t *)malloc(sizeof(price_level_t));
  pl->side = side;
  pl->price = price;
  pl->header = (order_node_t *)malloc(sizeof(order_node_t));
  pl->tailer = (order_node_t *)malloc(sizeof(order_node_t));
  pl->header->prev = NULL;
  pl->header->next = pl->tailer;
  pl->tailer->prev = pl->header;
  pl->tailer->next = NULL;
  return pl;
}

void delete_price_level(price_level_t *pl) {
  order_node_t *next;
  order_node_t *current = pl->header->next;
  while (current != pl->tailer) {
    next = current->next;
    free(current);
    current = next;
  }
  free(pl->header);
  free(pl->tailer);
  free(pl);
}

void insert_order_at_tail(price_level_t *pl, t_orderid id, t_order order) {
  assert(order.price == pl->price && order.side == pl->side); // order in this price_level should be of this price and side
  order_node_t *new_node = (order_node_t *)malloc(sizeof(order_node_t));
  new_node->order_obj.id = id;
  new_node->order_obj.order = order;
  new_node->next = pl->tailer;
  new_node->prev = pl->tailer->prev;
  pl->tailer->prev->next = new_node;
  pl->tailer->prev = new_node;
}

void remove_order_node(order_node_t *node) {
  // should never delete the header and tailer node of a price_level
  assert(node->prev != NULL);
  assert(node->next != NULL);
  node->prev->next = node->next;
  node->next->prev = node->prev;
  free(node);
}

void print_price_level(price_level_t *pl) {
  printf("price level %s at %d\n", (is_ask(pl->side)) ? "ASK" : "BID", pl->price);
  order_node_t *current = pl->header->next;
  while (current != pl->tailer) {
    printf("Order %lu by trader %s on symbol %s size %lu\n", current->order_obj.id, current->order_obj.order.trader, current->order_obj.order.symbol, current->order_obj.order.size);
    current = current->next;
  }
  printf("\n");
}

// int main(void) {

//   price_level_t *pl = init_price_level((t_side)0, (t_price)100);
//   t_order order = {"BMW", "JYK", 0, 100, 200};
//   insert_order_at_tail(pl, 1, order);
//   order.size = 50;
//   insert_order_at_tail(pl, 2, order);
//   order.size = 100;
//   insert_order_at_tail(pl, 3, order);
//   order.size = 150;
//   insert_order_at_tail(pl, 4, order);
//   print_price_level(pl);
//   order_node_t *to_remove = pl->header->next->next;
//   to_remove->order_obj.order.size = 0;
//   remove_order_node(to_remove);
//   print_price_level(pl);
//   return 0;
// }


