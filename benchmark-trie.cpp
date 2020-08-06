#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "catch2/catch.hpp"
#include "trie.hpp"
#include <ext/pb_ds/assoc_container.hpp>
#include <map>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef TYPE_TESTED
#define TYPE_TESTED Trie<std::string, std::size_t>
#endif

using ContainerType = TYPE_TESTED;

std::vector<std::pair<std::string, std::size_t>> read_enable1() {
  std::vector<std::pair<std::string, std::size_t>> v(127824);
  std::ifstream enable1("res/enable1.txt");
  std::string cline;
  while (getline(enable1, cline)) {
    v.push_back(std::make_pair(cline, cline.length()));
  }
  enable1.close();
  return v;
}

ContainerType
prepare_enable1_trie(std::vector<std::pair<std::string, std::size_t>> &v) {
  ContainerType trie;
  for (auto p : v) {
    trie[p.first] = p.second;
  }
  return trie;
}

TEST_CASE("Make trie from vector") {
  auto v = read_enable1();
  BENCHMARK("Convert vector to trie") { return prepare_enable1_trie(v); };
}

TEST_CASE("Query large trie") {
  auto v = read_enable1();
  ContainerType trie = prepare_enable1_trie(v);

  BENCHMARK("Query for existent short word") { return trie["ab"]; };
  BENCHMARK("Query for existent long word") {
    return trie["rememberabilities"];
  };
  BENCHMARK("Query for non-existent word") { return trie["kadfjjsdiof"]; };
  BENCHMARK("Query for non-existent word with existing prefix") {
    return trie["rememberabilitiex"];
  };
}

TEST_CASE("Iterate over trie") {
  auto v = read_enable1();
  ContainerType trie = prepare_enable1_trie(v);

  BENCHMARK("Sum of word lengths") {
    std::size_t sum = 0;
    for (auto entry : trie) {
      sum += entry.second;
    }
    CHECK(sum == 1570540);
    return sum;
  };
}