CC=g++
CFLAGS=--std=c++17 -Wall

init:
	mkdir -p out

unittests: init
	$(CC) $(CFLAGS) -o out/test test.cpp && out/test

unittests_cov: init
	$(CC) $(CFLAGS) --coverage -o out/test test.cpp && out/test && gcov test >/dev/null && cat trie.hpp.gcov
	
clean:
	rm *.gcov *.gcda *.gcno out/*
