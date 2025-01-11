// standard library include
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// project include
#include "limits.h"
#include "types.h"
#include "engine.h"

// key types definition
typedef struct resting_order {
  t_size size;
  struct resting_order *next;
  char trader[STRINGLEN];
} resting_order_t;

typedef struct price_level {
  resting_order_t *head;
  resting_order_t *tail;
} price_level_t;

// globals
#define MAX_ORDER_NUM (1024 * 1024)
static resting_order_t order_slots[MAX_ORDER_NUM];
static price_level_t order_book[MAX_PRICE+1];

static t_price best_bid;
static t_price best_ask;

static t_orderid id_counter;

// #define STR_COPY(DEST, SRC) {strcpy(DEST, SRC);}
// #define STR_COPY(DEST, SRC) {strncpy(DEST, SRC, STRINGLEN);}
#define STR_COPY(DEST, SRC) {memcpy(DEST, SRC, STRINGLEN);}
// #define STR_COPY(DEST, SRC) {DEST[0] = SRC[0]; DEST[1] = SRC[1]; DEST[2] = SRC[2]; DEST[3] = SRC[3]; DEST[4] = SRC[4];}

// utility functions
void place_resting_order(t_orderid id, t_order order) {
  if (order.size == 0) { return; }
  resting_order_t *o_slot = &order_slots[id];
  price_level_t *pl = &order_book[order.price];

  // prepare the slot
  o_slot->size = order.size;
  STR_COPY(o_slot->trader, order.trader);

  if (!pl->head) {
    // first order in this price level
    // being head and tail at the same time
    pl->head = o_slot;
    pl->tail = o_slot;
  } else {
    // insert after tail and becomes the tail
    pl->tail->next = o_slot;
    pl->tail = o_slot;
  }

  // update the NBBO 
  if (is_ask(order.side) && order.price < best_ask) {
    best_ask = order.price;
  }
  if (!is_ask(order.side) && order.price > best_bid) {
    best_bid = order.price;
  }
}

void report_execution(t_price price, t_size size, const char *symbol, const char *buyer, const char *seller) {
  if (size == 0) { return; }
  t_execution exe;
  STR_COPY(exe.symbol, symbol);
  exe.price = price;
  exe.size = size;

  // report buyer
  exe.side = 0;
  STR_COPY(exe.trader, buyer);
  execution(exe);

  // report seller
  exe.side = 1;
  STR_COPY(exe.trader, seller);
  execution(exe);
}

// main interface
void init() {
  memset(order_slots, 0, sizeof(order_slots));
  memset(order_book, 0, sizeof(order_book));
  best_ask = MAX_PRICE;
  best_bid = MIN_PRICE;
  id_counter = 0;
}

void destroy() {
  // pass
}

t_orderid limit(t_order order) {
  t_orderid id = ++id_counter;
  // try match with resting counter side orders on book
  if (is_ask(order.side)) {
    while (order.price <= best_bid) {
      price_level_t *pl = &order_book[best_bid];
      resting_order_t *head_order = pl->head;
      while (head_order) {
        if (head_order->size >= order.size) {
          // fully filled incoming order
          report_execution(best_bid, order.size, order.symbol, head_order->trader, order.trader);
          head_order->size -= order.size;
          order.size = 0;
          pl->head = (head_order->size != 0) ? head_order : head_order->next; // discard all exhausted order entries
          return id;
        } else {
          // resting order is filled
          if (head_order->size > 0) {
            report_execution(best_bid, head_order->size, order.symbol, head_order->trader, order.trader);
            order.size -= head_order->size;
            head_order->size = 0;
          }
        }
        head_order = head_order->next;
      }
      // this price level is exhausted
      pl->head = NULL;
      best_bid--;
    }
  } else {
    while (order.price >= best_ask) {
      price_level_t *pl = &order_book[best_ask];
      resting_order_t *head_order = pl->head;
      while (head_order) {
        if (head_order->size >= order.size) {
          report_execution(best_ask, order.size, order.symbol, order.trader, head_order->trader);
          head_order->size -= order.size;
          order.size = 0;
          pl->head = (head_order->size != 0) ? head_order : head_order->next; // discard all exhausted order entries
          return id;
        } else {
          report_execution(best_ask, head_order->size, order.symbol, order.trader, head_order->trader);
          order.size -= head_order->size;
          head_order->size = 0;
        }
        head_order = head_order->next;
      }
      pl->head = NULL;
      best_ask++;
    }
  }

  // cannot be filled immediately, rest on book
  place_resting_order(id, order);
  return id;
}

void cancel(t_orderid orderid) {
  order_slots[orderid].size = 0;
}