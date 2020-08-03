#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "trie.hpp"

#include <iostream>
#include <optional>
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

  std::vector<std::string> expected{"AA", "A", "BB", "B", "CC", "C"};

  SECTION("Iterating once with for-each loop") {
    std::vector<std::string> results{};
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

    REQUIRE(*it1 == expected[0]);
    REQUIRE(*it2 == expected[0]);

    ++it1;
    REQUIRE(*it1 == expected[1]);
    REQUIRE(*it2 == expected[0]);

    ++++it2;
    REQUIRE(*it1 == expected[1]);
    REQUIRE(*it2 == expected[2]);
    REQUIRE(it1 != it2);

    auto it3 = trie.begin();
    REQUIRE(*it3 == expected[0]);

    ++it1;
    REQUIRE(it1 == it2);
    REQUIRE(it1 != it3);
    REQUIRE(it2 != it3);

    for (std::size_t i = 0; i < 3; ++i) {
      ++it1;
      ++it2;
    }
    REQUIRE(*it1 == expected[5]);
    REQUIRE(*it2 == expected[5]);

    ++it2;
    ++it1;
    REQUIRE(it1 == it1_end);
    REQUIRE(it2 == it2_end);
    REQUIRE(it1 == trie.end());
    REQUIRE(it2 == trie.end());
  }
}

TEST_CASE("Retrieve elements from trie", "[trie retrieve]") {
  Trie<std::string> trie{};
  trie.insert("A", "A");
  trie.insert("B", "B");
  trie.insert("AB", "AB");

  REQUIRE(trie.at("A") == "A");
  REQUIRE(trie.at("B") == "B");
  REQUIRE(trie.at("AB") == "AB");
}

TEST_CASE("Replace elements in the trie", "[trie replace]") {
  SECTION("Without checking the return value") {
    Trie<std::string> trie{};
    trie.insert("A", "A");
    trie.insert("B", "B");
    trie.insert("AB", "AB");

    REQUIRE(trie.at("A") == "A");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    trie.insert("A", "C");

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    trie.insert("AB", "CD");

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "CD");
  }
  SECTION("With checking the return value") {
    Trie<std::string> trie{};
    REQUIRE(trie.insert("A", "A") == std::optional<std::string>());
    REQUIRE(trie.insert("B", "B") == std::optional<std::string>());
    REQUIRE(trie.insert("AB", "AB") == std::optional<std::string>());

    REQUIRE(trie.at("A") == "A");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    REQUIRE(trie.insert("A", "C") == std::optional<std::string>("A"));

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    REQUIRE(trie.insert("AB", "CD") == std::optional<std::string>("AB"));

    // use return value as lvalue
    std::optional<std::string> returned = trie.insert("AB", "Hello World!");
    REQUIRE(returned.value() == "CD");

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "Hello World!");
  }
}
