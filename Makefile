CC=g++
CFLAGS=--std=c++17 -Wall

test_main.o: test.cpp
	$(CC) $(CFLAGS) test.cpp -c -o test_main.o

unittests: test_main.o testcases.cpp
	$(CC) $(CFLAGS) -o test-exe test_main.o testcases.cpp && ./test-exe

unittests_cov: test_main.o
	$(CC) $(CFLAGS) --coverage -o test-exe test_main.o testcases.cpp && ./test-exe && gcov testcases >/dev/null && cat trie.hpp.gcov
	
clean:
	rm *.gcov *.gcda *.gcno *.o
