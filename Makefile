CC=g++-10
CFLAGS=--std=c++20 -Wall

test_main.o: test.cpp
	$(CC) $(CFLAGS) -O3 test.cpp -c -o test_main.o

unittests: test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-exe test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-ar-exe -D TRIE_USE_ARRAY test_main.o testcases.cpp
	./test-exe
	./test-ar-exe


unittests_cov: test_main.o
	$(CC) $(CFLAGS) --coverage -o test-exe test_main.o testcases.cpp
	./test-exe
	gcov testcases >/dev/null
	cat trie.hpp.gcov
	
benchmark: test_main.o benchmark-trie.cpp trie.hpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-exe test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-um-exe -D TRIE_USE_UNORDERED_MAP test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-ar-exe -D TRIE_USE_ARRAY test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-map-exe -D TYPE_TESTED="std::map<std::string,std::size_t>" test_main.o benchmark-trie.cpp
	$(CC) $(CFLAGS) -O3 -o benchmark-trie-bi-exe -D TYPE_TESTED="__gnu_pbds::trie<std::string,std::size_t>" test_main.o benchmark-trie.cpp
	./benchmark-trie-exe > benchmark/benchmark-results-trie.txt
	./benchmark-trie-um-exe > benchmark/benchmark-results-trie-um.txt
	./benchmark-trie-ar-exe > benchmark/benchmark-results-trie-ar.txt
	./benchmark-trie-bi-exe > benchmark/benchmark-results-trie-bi.txt
	./benchmark-map-exe > benchmark/benchmark-results-map.txt

clean:
	rm *.gcov *.gcda *.gcno *.o *-exe
