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

aka, this singe line from benchmark implementation is what my implementation is missing.

```c
    # after finish matching incoming order against resting order book
    ppEntry->listHead = bookEntry;
```

After this optimization, my implementation's performance gets closer to the benchmark performance, about 5% inferior to it. So there might be some other places of details that we need to profile and iterate improvement.

```bash
Across 10 scoring:
my implementation:        mean 2914.82 std 803.91  score 1859.37
benchmark implementation: mean 2754.70 std 857.01  score 1805.85
```

### Profiling 2

So we continue our optimization. One intriguing point the benchmark implementation mentioned is that "The memory layout of struct orderBookEntry has been optimized". In particular, despite the given macro `#define STRINGLEN 5`, the benchmark defines the trader and symbol char buffer of size 4. My first thought: "Why? Does this really matter?". 

Let dive deep into this and look at the generated assembly code for the part of interests.

```asm
# engine.c:49:   STR_COPY(o_slot->trader, order.trader);
	movq	-16(%rbp), %rax	# o_slot, tmp109
	addq	$16, %rax	#, _5
	movl	21(%rbp), %edx	# MEM <unsigned char[5]> [(char * {ref-all})&order + 5B], tmp110
	movl	%edx, (%rax)	# tmp110, MEM <unsigned char[5]> [(char * {ref-all})_5]
	movzbl	25(%rbp), %edx	# MEM <unsigned char[5]> [(char * {ref-all})&order + 5B], tmp111
	movb	%dl, 4(%rax)	# tmp111, MEM <unsigned char[5]> [(char * {ref-all})_5]
```

It takes 4 steps to copy the 5 bytes from `order.trader` to `o_slot->trader`.

1. copy first 4 bytes from `order.trader` into 4-byte-long register `%edx`
2. copy `%edx` to first 4 bytes of `o_slot->trader`
3. copy the 5th byte from `order.trader` into the 4-byte-long register `%edx` and zero extend
4. copy this last byte from lower byte of `%edx` into `o_slot->trader`'s 5th byte.

Now we can see, the 1 extra byte copying doubles the entire operation cycles required. We could safely discard the null terminator at the fifth poistion and manage the string copy ourselves.

we set the `t_execution exe` to be a static variable to avoid allocating it on the stack everytime we enter the `execution()` function and ensure it's zero initialized. and then allocate and only copy the first 4 bytes of `trader` and `symbol`, ignoring the null terminator.

we end up with the new assembly code as follows:

```asm
# engine.c:50:   STR_COPY(o_slot->trader, order.trader);
	movq	-16(%rbp), %rax	# o_slot, tmp110
	leaq	16(%rax), %rdx	#, _5
	movl	21(%rbp), %eax	# MEM <unsigned int> [(char * {ref-all})&order + 5B], _26
	movl	%eax, (%rdx)	# _26, MEM <unsigned int> [(char * {ref-all})_5]
```

As we expected, we effectively reduce the 4 steps to 2 steps. With this optimzation, we slightly improve the score from 5% inferior to 4% inferior:

```bash
Across 10 scoring:
my implementation:        mean 3132.51 std 842.94  score 1987.73
benchmark implementation: mean 2984.59 std 887.512  score 1936.05
```

There is probably more to be profiled and optimized.