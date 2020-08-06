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

struct AlphabeticalStringConverter {
  using KeyContent = char;
  static KeyContent get_at_index(const std::string &key, const std::size_t ind) {
    return key[ind] <= 90 ? key[ind] - 65 : key[ind] - (97 - 26);
  }

  static std::size_t size(const std::string &key) {
    return key.size();
  }
};

using ContainerType = TYPE_TESTED;

std::vector<std::pair<std::string, std::size_t>> read_words() {
  std::vector<std::pair<std::string, std::size_t>> v(127824);
  std::ifstream enable1("res/unix-words.txt");
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
  auto v = read_words();
  BENCHMARK("Convert vector to trie") { return prepare_enable1_trie(v); };

  auto structure = prepare_enable1_trie(v);
  std::string long_word = "testwordtestword";
  std::string short_word = "ab";

  BENCHMARK("Insert long key into large structure") {
    return structure[long_word] = long_word.size();
  };

  BENCHMARK("Insert short key into large structure") {
    return structure[short_word] = short_word.size();
  };
}

TEST_CASE("Query large trie") {
  auto v = read_words();
  ContainerType trie = prepare_enable1_trie(v);

  BENCHMARK("Query for existent short word") { return trie["a"]; };
  BENCHMARK("Query for existent long word") {
    return trie["pseudoconglomeration"];
  };
  BENCHMARK("Query for non-existent word") { return trie["kadfjjsdiof"]; };
  BENCHMARK("Query for non-existent word with existing prefix") {
    return trie["pseudoconglomeratiox"];
  };
}

TEST_CASE("Iterate over trie") {
  auto v = read_words();
  ContainerType trie = prepare_enable1_trie(v);

  BENCHMARK("Sum of word lengths") {
    std::size_t sum = 0;
    for (auto entry : trie) {
      sum += entry.second;
    }
    CHECK(sum == 2257221);
    return sum;
  };
}