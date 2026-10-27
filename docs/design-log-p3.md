# Project 3 Design Log
**Author:** Alex Zhang · **P2 code used:** Instructor Reference
**Tools used:** Google Gemini 3.1 Pro used primarily in debugging and maintaining proper error management and naming schemes 
in the registry, calc_evaluate, and tests.

## 1. Hash table
For the hash table, I chose to use chaining. Each bucket is a std::vector of entries, and if two names land in the same bucket, the bucket is shared between the two. This is a very simple implementation, resulting in slower lookups, but it won't break. Open addressing is faster on cache, but easier to go wrong with when going for a load factor below 1.0. The hash function I used is a 64-bit FNV-1a, which uses every character and features sensitivity to the order of its input. Before inserting, the add() function checks if size+1 will exceed the bucket count, doubling the table size and re-inserting entries if this is the case. When add() returns, the load factor is never above 1.0. The add() function is amortized O(1) because resizes happen at sizes 8, 16, 32, ...,8192, and each one re-inserts that many entries. The total is 16,376 extra moves for 10,000 adds, less than two per add. Each expensive resize is balanced with many cheap insertions.

My registry ended with bucket_count = 16384, load_factor = 0.6104, max_probe_length = 6. The weak hash puts 9,000 of the 10,000 names in one bucket because the names share a first character and have few different lengths.

## 2. Pointer stability
Each entry holds a std::unique_ptr<Tool>, and when the table grows, only the pointer is moved to the new bucket while the tool object stays at the same address. So a Tool* from find() is still valid after any number of resizes.

## 3. Calculator trace
2 is the first token, and it's pushed onto values. The '*' operation is then pushed onto ops. '(' is pushed onto ops next, and then 3 is pushed onto values. When '+' is pushed onto ops, nothing happens because '(' is on top of ops, and this is followed by 4 being pushed onto values. Once ')' is pushed onto ops, the addition operator is applied (3 + 4 = 7), making values now contain 2 and 7, while ops only contains the multiply operator. When '-' is pushed, * has higher precedence and 2 * 7 = 14 is calculated. 5 is then pushed onto values, and 14 - 5 = 9 is calculated, giving the final output of the expression.

## 4. Errors as values

Because tool failures such as missing files, unusable paths, and incorrect expressions are normal in an agent loop, the model simply reads the error and tries something else. The error provides data to the model. Exceptions should be used for bugs, like calling as_string() on a number, or running out of memory, where the program is in a state not planned for.

## 5. What I would design differently






