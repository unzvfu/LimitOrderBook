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
Across 50 scoring:
my implementation:        mean 6286.92   std 18119.89 score 12203.40
benchmark implementation: mean 2849.53   std 4748.58  score 3799.05
```

The benchmark implementation is twice as fast as this first version of implementation. Further profiling to be made and optimization to be made.
