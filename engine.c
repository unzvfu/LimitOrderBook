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
  struct price_level *prev;
  struct price_level *next;
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
  pl->prev = NULL;
  pl->next = NULL;
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
  if (pl->prev) {pl->prev->next = pl->next; }
  if (pl->next) {pl->next->prev = pl->prev; }
  free(pl);
}

void insert_order_at_tail(price_level_t *pl, t_orderid id, t_order order) {
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
  node->prev->next = node->next;
  node->next->prev = node->prev;
  free(node);
}

typedef struct book_side {
  t_side side;
  price_level_t *header_pl;
  price_level_t *tailer_pl;
} book_side_t;

book_side_t *init_book_side(int side) {
  book_side_t *book_side = (book_side_t *)malloc(sizeof(book_side_t));
  book_side->side = side;
  book_side->header_pl = init_price_level(side, 0);
  book_side->tailer_pl = init_price_level(side, 0);
  book_side->header_pl->next = book_side->tailer_pl;
  book_side->tailer_pl->prev = book_side->header_pl;
  return book_side;
}

void destroy_book_side(book_side_t *book_side) {
  price_level_t *curr_pl = book_side->header_pl->next;
  while (curr_pl != book_side->tailer_pl) {
    price_level_t *next_pl = curr_pl->next;
    delete_price_level(curr_pl);
    curr_pl = next_pl;
  }
  delete_price_level(book_side->header_pl);
  delete_price_level(book_side->tailer_pl);
  free(book_side);
}

static bool is_price_level_empty(price_level_t *pl) {
  return (pl->header->next == pl->tailer) && (pl->tailer->prev == pl->header);
}

price_level_t *insert_between_price_level(price_level_t *prev, price_level_t *next, t_side side, t_price price) {
  price_level_t *new_pl = init_price_level(side, price);
  new_pl->prev = prev;
  new_pl->next = next;
  prev->next = new_pl;
  next->prev = new_pl;
  return new_pl;
}

price_level_t *get_or_create_price_level(book_side_t *book_side, t_price price) {
  bool ask = is_ask(book_side->side);
  price_level_t *prev = book_side->header_pl, *curr = book_side->header_pl->next;
  while (curr != book_side->tailer_pl) {
    if (curr->price == price) {
      return curr;
    } else if ((ask && curr->price > price) || (!ask && curr->price < price)) {
      return insert_between_price_level(prev, curr, book_side->side, price);
    } else{
      prev = curr;
      curr = curr->next;
    }
  }
  return insert_between_price_level(prev, curr, book_side->side, price);
}

void add_order_to_book_side(book_side_t *book_side, t_orderid id, t_order order) {
  price_level_t *pl = get_or_create_price_level(book_side, order.price);
  insert_order_at_tail(pl, id, order);
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

void print_book_side(book_side_t *book_side) {
  printf("book side at %s\n", (is_ask(book_side->side)) ? "ASK" : "BID");
  price_level_t *curr = book_side->header_pl->next;
  while (curr != book_side->tailer_pl) {
    print_price_level(curr);
    curr = curr->next;
  }
  printf("\n");
}

// interface implementation
static book_side_t *bid_book_side;
static book_side_t *ask_book_side;
static t_orderid orderid_counter;
/* IN:
   OUT: */
void init() {
  bid_book_side = init_book_side(0);
  ask_book_side = init_book_side(1);
  orderid_counter = 1;
}

/* IN:
   OUT: */
void destroy() {
  destroy_book_side(bid_book_side);
  destroy_book_side(ask_book_side);
}

static void report_execution(t_order buy_order, t_order sell_order, t_price trade_price, t_size trade_qty) {
  t_execution exe;
  exe.price = trade_price;
  exe.size = trade_qty;
  exe.side = 0;
  strncpy(exe.symbol, buy_order.symbol, STRINGLEN);
  strncpy(exe.trader, buy_order.trader, STRINGLEN);
  execution(exe);
  exe.side = 1;
  strncpy(exe.symbol, sell_order.symbol, STRINGLEN);
  strncpy(exe.trader, sell_order.trader, STRINGLEN);
  execution(exe);
}

// recursive 
// return a order of qty 0 if all filled
// or non-zero qty means this order needs to rest on book
t_order match_trade(t_order order) {
  if (order.size == 0) {
    return order;
  }
  if (is_ask(order.side)) {
    price_level_t *best_pl = bid_book_side->header_pl->next;
    while (best_pl != bid_book_side->tailer_pl) {
      if (best_pl->price < order.price) {
        return order;
      } else {
        // some trade can happens
        // just grab first order on this pl
        order_node_t *first_order = best_pl->header->next;
        t_size trade_qty = MIN(first_order->order_obj.order.size, order.size);
        t_price trade_px = first_order->order_obj.order.price;
        report_execution(first_order->order_obj.order, order, trade_px, trade_qty);
        order.size -= trade_qty;
        first_order->order_obj.order.size -= trade_qty;
        if (first_order->order_obj.order.size == 0) {
          remove_order_node(first_order);
          if (is_price_level_empty(best_pl)) {
            delete_price_level(best_pl);
          }
        }
        return match_trade(order); // recurse
      }
    }
    return order;
  } else {
    price_level_t *best_pl = ask_book_side->header_pl->next;
    while (best_pl != ask_book_side->tailer_pl) {
      if (best_pl->price > order.price) {
        return order;
      } else {
        order_node_t *first_order = best_pl->header->next;
        t_side trade_qty = MIN(first_order->order_obj.order.size, order.size);
        t_price trade_px = first_order->order_obj.order.price;
        report_execution(order, first_order->order_obj.order, trade_px, trade_qty);
        order.size -= trade_qty;
        first_order->order_obj.order.size -= trade_qty;
        if (first_order->order_obj.order.size == 0) {
          remove_order_node(first_order);
          if (is_price_level_empty(best_pl)) {
            delete_price_level(best_pl);
          }
        }
        return match_trade(order);
      }
    }
    return order;
  }
}

/* IN: order: limit order to add to book
   OUT: orderid assigned to order 
        start from 1 and increment with each call */
t_orderid limit(t_order order) {
  t_orderid id = orderid_counter++;
  t_order remain_order = match_trade(order);
  if (remain_order.size) {
    add_order_to_book_side((is_ask(remain_order.side)) ? ask_book_side : bid_book_side, id, remain_order);
  }
  return id;
}

bool try_remove_order(book_side_t *book_side, t_orderid orderid) {
  price_level_t *pl = book_side->header_pl->next;
  while (pl != book_side->tailer_pl) {
    order_node_t *node = pl->header->next;
    while (node != pl->tailer) {
      if (node->order_obj.id == orderid) {
        remove_order_node(node);
        if (is_price_level_empty(pl)) {
          delete_price_level(pl);
        }
        return true;
      }
      node = node->next;
    }
    pl = pl->next;
  }
  return false;
}

/* IN: orderid: id of order to cancel
   OUT:
   cancel request ignored if orderid not in book
*/
void cancel(t_orderid orderid) {
  if (!try_remove_order(bid_book_side, orderid)) {
    try_remove_order(ask_book_side, orderid);
  }
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


