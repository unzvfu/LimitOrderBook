# LimitOrderBook

Implementation of the QuantCup (http://www.quantcup.org/) limit order book contest in vanilia C. We gradually improve its performance via profiling.

## Benchmark

We take the winning champion solution (by voyager) as the benchmark.

## Iterations

The first best-effort version I implemented features the following:

* each `price_level_t` has a singly-linked list of `resting_order_t` respect the price-time priority
* statically allocated all the price level and order entry to avoid dynamic memory allocation
* best bid price and best ask price helps to determine the side. so `side` could be omitted from the struct
* cancel is `O(1)` by marking the order size to 0

```
Across 10 scoring:
my implementation:        mean 5687.13 std 7872.69 score 6779.91
benchmark implementation: mean 2777.48 std 807.75  score 1792.62
```

The benchmark implementation is twice as fast as this first version of implementation. Further profiling to be made and optimization to be made.

### Profiling 1

There are only 2 actions in the LimitOrderBook: `limit` or `cancel`. We need to understand which one is the bottleneck between my implementation and the benchmark implementation.

```bash
My engine
limit() called 35624000 times, average 0.067 us
cancel() called 35576000 times, average 0.025 us

Winning engine
limit() called 35624000 times, average 0.042 us
cancel() called 35576000 times, average 0.026 us
```

So looks like it's the placing a `limit` order that becomes the bottleneck. Among many other metrics I took measurement of, one caught my attention in particular:

```bash
My engine
Among 35624000 walking through the orderbook to match, average steps 15.585

Winning engine
Among 35624000 walking through the orderbook to match, average steps 0.956
```

My implementation and the benchmark implementation are more or less the same algorithm and data structure, why in matching an incoming order my implementation would have to walk so many steps into the order list?

A closer look into the benchmark implementation revails that I forgot to "clear out" the exhausted order entries from the order list, accumulating a long of zombie order entries of size 0 in the resting order book.
