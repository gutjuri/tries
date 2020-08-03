#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "trie.hpp"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE("Construct and destroy trie", "[trie]") {
  SECTION("String trie") {
    { Trie<std::string> t{}; }
  }
  SECTION("int trie") {
    { Trie<int> t{}; }
  }
  SECTION("char trie") {
    { Trie<char> t{}; }
  }
}

TEST_CASE("Add elements to trie and check if they're in the trie", "[trie]") {
  Trie<std::string> trie{};
  trie.insert("key1", "hello");
  trie.insert("key2", "world");
  trie.insert("otherPrefixKey", "!!");

  REQUIRE(trie.has_key("key1"));
  REQUIRE(trie.has_key("key2"));
  REQUIRE(trie.has_key("otherPrefixKey"));

  REQUIRE_FALSE(trie.has_key("key3"));
  REQUIRE_FALSE(trie.has_key("someKey"));
  REQUIRE_FALSE(trie.has_key("key"));
  REQUIRE_FALSE(trie.has_key("key21"));
  REQUIRE_FALSE(trie.has_key(""));
}

TEST_CASE("Iterating over a trie", "[trie iterator]") {
  Trie<std::string> trie{};
  trie.insert("A", "A");
  trie.insert("B", "B");
  trie.insert("C", "C");
  trie.insert("AA", "AA");
  trie.insert("BB", "BB");
  trie.insert("CC", "CC");

  SECTION("Iterating once with for-each loop") {
    std::vector<std::string> results{};
    std::vector<std::string> expected{"AA", "A", "BB", "B", "CC", "C"};
    for (auto x : trie) {
      results.push_back(x);
    }

    REQUIRE(results == expected);
  }

  SECTION("Parallely iterating") {
    auto it1 = trie.begin();
    auto it1_end = trie.end();
    auto it2 = trie.begin();
    auto it2_end = trie.end();

    REQUIRE(*it1 == "AA");
    REQUIRE(*it2 == "AA");

    ++it1;
    REQUIRE(*it1 == "A");
    REQUIRE(*it2 == "AA");
  }
}
