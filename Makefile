CC=g++-10
CFLAGS=--std=c++20 -Wextra -pedantic -Werror

test_main.o: test.cpp
	$(CC) $(CFLAGS) -O3 test.cpp -c -o test_main.o

unittests: test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-exe test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-ar-exe -D TEST_USE_ARRAY test_main.o testcases.cpp
	./test-exe
	./test-ar-exe

unittests_cov: test_main.o
	$(CC) $(CFLAGS) --coverage -o test-exe test_main.o testcases.cpp
	./test-exe
	gcov testcases >/dev/null
	cat trie.hpp.gcov

bm_bins: test_main.o benchmark-trie.cpp trie.hpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-exe test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-um-exe -D BM_UNORDERED_MAP test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-ar-exe -D BM_ARRAY test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-ar-custom-exe -D BM_ARRAY_CUSTOM test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-map-exe -D BM_STD_MAP test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-bi-exe -D BM_GNU_TRIE test_main.o benchmark-trie.cpp

benchmark: bm_bins
	./benchmark-trie-exe >/dev/null # warm up caches
	./benchmark-trie-exe > benchmark/benchmark-results-trie.txt
	./benchmark-trie-um-exe > benchmark/benchmark-results-trie-um.txt
	./benchmark-trie-ar-exe > benchmark/benchmark-results-trie-ar.txt
	./benchmark-trie-bi-exe > benchmark/benchmark-results-trie-gnu-trie.txt
	./benchmark-map-exe > benchmark/benchmark-results-map.txt
	./benchmark-trie-ar-custom-exe > benchmark/benchmark-results-trie-ar-custom.txt

benchmark_memory: bm_bins
	time -v ./benchmark-trie-exe >/dev/null 2> benchmark/memory-usage-trie-map.txt
	time -v ./benchmark-trie-um-exe >/dev/null 2> benchmark/memory-usage-trie-umap.txt
	time -v ./benchmark-trie-ar-exe >/dev/null 2> benchmark/memory-usage-trie-array.txt
	time -v ./benchmark-trie-ar-custom-exe >/dev/null 2> benchmark/memory-usage-trie-array-custom.txt
	time -v ./benchmark-trie-bi-exe >/dev/null 2> benchmark/memory-usage-trie-gnutrie.txt
	time -v ./benchmark-map-exe >/dev/null 2> benchmark/memory-usage-map.txt

clean:
	rm *.gcov *.gcda *.gcno *.o *-exe
