# Project 3 Design Log
**Author:** Alex Zhang · **P2 code used:** Instructor Reference
**Tools used:** Google Gemini 3.1 Pro used primarily in debugging and maintaining proper error management and naming schemes 
in the registry, calc_evaluate, and tests.

## 1. Hash table
For the hash table, I chose to use chaining. Each bucket is a std::vector of entries, and if two names land in the same bucket, the bucket is shared between the two. This is a very simple implementation, resulting in slower lookups, but it won't break. Open addressing is faster on cache, but easier to go wrong with when going for a load factor below 1.0. The hash function I used is a 64-bit FNV-1a, which uses every character and features sensitivity to the order of its input. Before inserting, the add() function checks if size+1 will exceed the bucket count, doubling the table size and re-inserting entries if this is the case. When add() returns, the load factor is never above 1.0. The add() function is amortized O(1) because resizes happen at sizes 8, 16, 32, ...,8192, and each one re-inserts that many entries. The total is 16,376 extra moves for 10,000 adds, less than two per add. Each expensive resize is balanced with many cheap insertions.

My registry ended with bucket_count = 16384, load_factor = 0.6104, max_probe_length = 6. The weak hash puts 9,000 of the 10,000 names in one bucket because the names share a first character and have few different lengths.



## 2. Pointer stability
<!-- Why does a Tool* returned by find() survive a resize in your design? -->

## 3. Calculator trace
<!-- Trace the values and ops stacks for 2 * (3 + 4) - 5, one token at a
     time, in your own words. -->

## 4. Errors as values
<!-- Why do tools, resolve() and calc_evaluate() return errors instead of
     throwing? When IS an exception the right choice? -->

## 5. What I would design differently

