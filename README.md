# About
This repository implements and compares different [suffix array][sa-wikipedia]
construction algorithms

## Usage
Build the executable using
```shell
make
```
The produced `sa` reads and writes from the standard input and output.
To run the `linear` algorithm on the contents of `inputs/banana.txt` for example, use
```shell
./sa linear < inputs/banana.txt | hexdump
```
As `sa` produces the suffix array in a binary format, it is useful to pipe its
output through `hexdump`.


## Algorithms
- `linear`: The `O(n)` *Difference Cover 3* algorithm proposed by [Kärkkäinen
  and Sanders (2003)][karkainen] ([video lecture][6851-video]).
- `nlogn`: A simpler `O(n log n)` radix sort algorithm ([tutorial](cp-algo]).
- `naive`: Standard `O(n^2 log n)` comparison sort with `O(n)` string comparison.


## Benchmarks

### Inputs
- `alice.txt` (146K): The text of Alice in Wonderland, Lewis Carroll.
- `aaaab.txt` (977K): A text of `1 000 000` random characters in which each character is a
  `b` with probability `1/5000` and an `a` otherwise.
  Generated using `python3 gen-aaab.py`.

### Results

### Performance
Each algorithm was executed 10 times to amortize the overhead of I/O.
The presented slowdown is relative to the execution time of `linear`.

input     | linear     | nlogn        | naive
----------|------------|--------------|--------------
alice.txt | 0.29s (1x) | 0.73s (2.5x) | 0.38s (1.3x)
aaab.txt  | 1.15s (1x) | 3.19s (2.7x) | >10 minutes

### Analysis
`naive` can performs well on `alice.txt` because its suffixes have short LCPs
and naive string comparison takes expected `O(1)` on random strings.
However, `aaab.txt` demonstrates that it is much slower in the worst case.

Since the execution of `nlogn` and `linear` is mostly independent from the
contents of the text, they perform consistently across both inputs.
With the provided implementations, `linear` is ~2.6x faster than `nlogn`.

[sa-wikipedia]: https://en.wikipedia.org/wiki/Suffix_array
[karkainen]: https://doi.org/10.1007/3-540-45061-0_73
[cp-algo]: https://cp-algorithms.com/string/suffix-array.html
[6851-video]: https://courses.csail.mit.edu/6.851/spring21/lectures/L16.html?notes=8
