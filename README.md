# LimitOrderBook-quantcup

Reimplementation of the QuantCup (http://www.quantcup.org/) limit order book contest in vanilia C.

For reference, the scoring is done on my Macbook with 2 GHz Quad-Core Intel Core i5

The winnning C engine has the following scores:

```
mean(latency) = 3845.96, sd(latency) = 2630.17
You scored 3238.06. Try to minimize this.
```

## Iterations:

### Single List

add order : O(n)

delete order: O(n)

Use 2 single linked list for the bid side book and ask side book, linked with the price-time priority. When a new order is added, try to trade it against the opposite book as much as possible. If any quantity remaining unfilled, add it to the book.

The pain point of this approach is, to delete a note in the single linked list, you always have to hold its previous node. And all the find-node iterations need to start from the head of the list.

score

```
mean(latency) = 6841.67, sd(latency) = 3875.97
You scored 5358.82. Try to minimize this.
```
