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

### Price Level List

add order: O(n)

delete order: O(n)

The next approach I took is to create a double linked list of all the orders of the same price, and use another double linked list to link through all these price levels.

The benefits of this is that it's very easy to scan through the price level to look for a particular price, and every time adding a new order just means to add the order to the very end of its own price level.

However, surprisingly the performance score does not improve by much. Either some details of my implementation is not working as expected, or there is more work to be done to make it run faster.

```
mean(latency) = 6701.25, sd(latency) = 3746.64
You scored 5223.94. Try to minimize this.
```

### Statically allocated Price Level List

add order: O(1)

delete order: O(1)

I referenced the winning solution from `winning_engine.c` and get some inspirations.

This solution relies a lot on the constraints that there is lower limit and upper limit for the possible price range. So we can allocate a single list of resting orders per price, and statically allocate an array, whose index is the price and points to the price level head.

In the price level, we maintain a tail pointer so that to add an order to this price level is O(1) operations. And to delete an order, since we have an statically allocated order slot, the index is order id. We can directly find that order and mark its size to 0.

The downside of this approach is, when you are iterating over a price level, you might find some "junk order" who is already deleted and thus cause internal fragmentations in the list. Luckily, once the whole price level is exhaust, the head order pointer  for this price level will be reset, which essentially remove all the framentations in this price leve.

Technically my current solution is the exact same as the winning solution. However, there is still a huge difference in the final scoring. I didn't apply some optimization tricks like the `strcpy` as the winning solution author did. But I would be surprised if that could bring that much of a difference.

I would try to profile the program a little bit more to see where is the bottle neck difference between my solution and the winning solution.

```
mean(latency) = 5847.56, sd(latency) = 1497.84
You scored 3672.70. Try to minimize this.
```

### String copy optimization

So I open a t2.micro free AWS Linux server to better do benchmark. The winning solution has the following scores:

```
mean(latency) = 749.75, sd(latency) = 10772.27
You scored 5761.01. Try to minimize this.
```

I found one critical performance difference between my almost-same solution and the winning solution is in string copy.

I originally use the vanilia `strcpy` yet he used the loop unrolling version of hard-coded 4 character copy.

I tried it out and indeed it brings considerable performance boost. The principle is on, it eliminates the condition branching.

`strcpy` performance:

```
mean(latency) = 1118.85, sd(latency) = 13952.68
You scored 7535.77. Try to minimize this.
```

`strncpy` performance:

```
mean(latency) = 961.76, sd(latency) = 13507.53
You scored 7234.65. Try to minimize this.
```

`loop unrolling` performance:

```
mean(latency) = 916.31, sd(latency) = 12983.65
You scored 6949.98. Try to minimize this.
```

So by doing this string optimization, we get closer to the winning solution