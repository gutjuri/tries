CC=g++-10
CFLAGS=--std=c++20 -Wall

test_main.o: test.cpp
	$(CC) $(CFLAGS) -O3 test.cpp -c -o test_main.o

unittests: test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-exe test_main.o testcases.cpp
	./test-exe

unittests_cov: test_main.o
	$(CC) $(CFLAGS) --coverage -o test-exe test_main.o testcases.cpp
	./test-exe
	gcov testcases >/dev/null
	cat trie.hpp.gcov
	
benchmark: test_main.o
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-exe test_main.o benchmark-trie.cpp 
	$(CC) $(CFLAGS) -O3 -o benchmark-map-exe test_main.o benchmark-map.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-bi-exe test_main.o benchmark-trie-bi.cpp
	./benchmark-trie-exe > benchmark/benchmark-results-trie.txt
	./benchmark-trie-bi-exe > benchmark/benchmark-results-trie-bi.txt
	./benchmark-map-exe > benchmark/benchmark-results-map.txt

clean:
	rm *.gcov *.gcda *.gcno *.o *-exe
