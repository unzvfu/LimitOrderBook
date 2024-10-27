#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "engine.h"

#define MIN(a, b) ((a < b) ? (a) : (b))

// Internal Implementation
typedef struct order {
  t_orderid id;
  t_order order;
} order_t;

#define TYPE order_t
#define EMPTY_VAL (TYPE) {0, {'\0', '\0', 0, 0, 0}}
#define PRINTER(data) printf("[order_id: %ld symbol: %s trader: %s side: %d price: %d size: %ld] ", data.id, data.order.symbol, data.order.trader, data.order.side, data.order.price, data.order.size);
#include "single_list.c"

// -- GLOBALS -- 
static single_list_t *bid_list;
static single_list_t *ask_list;
static t_orderid id_counter;

/* return if the new price is superior in priority with the old price 
   in bid book it means new price is strictly higher than old price
   in ask book it means new price is strictly lower than old price
*/
static bool is_superior(bool ask, t_price new_price, t_price old_price) {
  return (ask) ? (new_price < old_price) : (new_price > old_price);
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

/* add an order to the list, assuming it will not cross the book and no trade can happen with its addition */
static void add_order(t_order order, t_orderid id) {
  if (order.size <= 0) { return; }
  bool ask = is_ask(order.side);
  order_t new_order = {id, order};
  single_list_t *list = (ask) ? ask_list : bid_list;
  node_t *curr = list->header;
  while (curr->next) {
    if (is_superior(ask, order.price, curr->next->data.order.price)) {
      insert_after(curr, new_order);
      return;
    }
    curr = curr->next;
  }
  insert_after(curr, new_order);
}

/* uncross the book and execute trade accordingly */
static t_order uncross_book(t_order incoming_order) {
  if (incoming_order.size == 0) { return incoming_order; }
  bool ask = is_ask(incoming_order.side);
  if (ask) {
    node_t *bid_current = bid_list->header;
    if (bid_current->next && bid_current->next->data.order.price >= incoming_order.price) {
      t_order best_bid_order = bid_current->next->data.order;
      t_price trade_price = best_bid_order.price;
      t_size trade_size = MIN(best_bid_order.size, incoming_order.size);
      report_execution(best_bid_order, incoming_order, trade_price, trade_size);
      incoming_order.size -= trade_size;
      bid_current->next->data.order.size -= trade_size;
      if (bid_current->next->data.order.size == 0) {
        delete_after(bid_current);
      }
      return uncross_book(incoming_order);
    } else {
      return incoming_order;
    }
  } else {
    node_t *ask_current = ask_list->header;
    if (ask_current->next && ask_current->next->data.order.price <= incoming_order.price) {
      t_order best_ask_order = ask_current->next->data.order;
      t_price trade_price = best_ask_order.price;
      t_size trade_size = MIN(best_ask_order.size, incoming_order.size);
      report_execution(incoming_order, best_ask_order, trade_price, trade_size);
      incoming_order.size -= trade_size;
      ask_current->next->data.order.size -= trade_size;
      if (ask_current->next->data.order.size == 0) {
        delete_after(ask_current);
      }
      return uncross_book(incoming_order);
    } else {
      return incoming_order;
    }
  }
}

/* try to delete an order with id specified from a list. return True if successfully find and delete it. */
static bool try_delete_order(single_list_t *list, t_orderid id) {
  for (node_t *node = list->header; node != NULL; node = node->next) {
    if (node->next && node->next->data.id == id) {
      delete_after(node);
      return true;
    }
  }
  return false;
}

// Interface Implementation
void init() {
  bid_list = init_single_list();
  ask_list = init_single_list();
  id_counter = 1;
}

/* IN:
   OUT: */
void destroy() {
  destroy_single_list(bid_list);
  destroy_single_list(ask_list);
}

/* IN: order: limit order to add to book
   OUT: orderid assigned to order 
        start from 1 and increment with each call */
t_orderid limit(t_order order) {
  t_orderid new_id = id_counter++;
  t_order remaining_order = uncross_book(order);
  add_order(remaining_order, new_id);
  return new_id;
}

/* IN: orderid: id of order to cancel
   OUT:
   cancel request ignored if orderid not in book
*/
void cancel(t_orderid orderid) {
  if (!try_delete_order(bid_list, orderid)) {
    try_delete_order(ask_list, orderid);
  }
}