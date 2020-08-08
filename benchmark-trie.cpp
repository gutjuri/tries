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

// A custom converter for using strings only containing the characters A-Za-z as
// keys. This precondition is not checked.
// In order to use this converter, instatiate a trie with
// Trie<std::string, ValueType, AlphabeticalStringConverter> and compile
// the programme with the preprocessor-variable TRIE_USE_ARRAY set and the
// preprocessor-variable TRIE_SIGMA set to 52
struct AlphabeticalStringConverter {
  using KeyContent = char;
  static KeyContent get_at_index(const std::string &key,
                                 const std::size_t ind) {
    return key[ind] <= 90 ? key[ind] - 65 : key[ind] - (97 - 26);
  }

  static std::size_t size(const std::string &key) { return key.size(); }
};

#ifdef BM_ARRAY
using ContainerType =
    Trie<std::string, std::size_t, DummyConverter<std::string>,
         ArrayStorage<std::string, char, std::size_t, 256>>;
#elif BM_ARRAY_CUSTOM
using ContainerType =
    Trie<std::string, std::size_t, AlphabeticalStringConverter,
         ArrayStorage<std::string, char, std::size_t, 52>>;
#elif BM_UNORDERED_MAP
using ContainerType =
    Trie<std::string, std::size_t, DummyConverter<std::string>,
         UnorderedMapStorage<std::string, char, std::size_t>>;
#elif BM_STD_MAP
using ContainerType = std::map<std::string, std::size_t>;
#elif BM_GNU_TRIE
using ContainerType = __gnu_pbds::trie<std::string, std::size_t>;
#else
using ContainerType = Trie<std::string, std::size_t>;
#endif

std::vector<std::pair<std::string, std::size_t>> read_words() {
  std::vector<std::pair<std::string, std::size_t>> v{};
  std::ifstream wordlist("res/unix-words.txt");
  std::string cline;
  while (getline(wordlist, cline)) {
    v.push_back(std::make_pair(cline, cline.length()));
  }
  wordlist.close();
  return v;
}

ContainerType
prepare_word_container(std::vector<std::pair<std::string, std::size_t>> &v) {
  ContainerType structure;
  for (auto p : v) {
    structure[p.first] = p.second;
  }
  return structure;
}

TEST_CASE("Make trie from vector") {
  auto v = read_words();
  BENCHMARK("Convert vector to data-structure") {
    return prepare_word_container(v);
  };

  auto structure = prepare_word_container(v);
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
  auto vec = read_words();
  ContainerType structure = prepare_word_container(vec);

  BENCHMARK("Query for existent short word") { return structure["a"]; };
  BENCHMARK("Query for existent long word") {
    return structure["pseudoconglomeration"];
  };
  BENCHMARK("Query for non-existent word") { return structure["kadfjjsdiof"]; };
  BENCHMARK("Query for non-existent word with existing prefix") {
    return structure["pseudoconglomeratiox"];
  };
}

TEST_CASE("Iterate over trie") {
  auto vec = read_words();
  ContainerType structure = prepare_word_container(vec);

  BENCHMARK("Sum of word lengths") {
    std::size_t sum = 0;
    for (auto entry : structure) {
      sum += entry.second;
    }
    CHECK(sum == 2257221);
    return sum;
  };
}