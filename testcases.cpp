#include "catch2/catch.hpp"
#include "trie.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#ifdef TEST_USE_ARRAY
using StringStringTrie =
    Trie<std::string, std::string, DummyConverter<std::string>,
         ArrayStorage<std::string, char, std::string, 256>>;
#else
using StringStringTrie = Trie<std::string, std::string>;
#endif

TEST_CASE("Constructing/copying/moving tries", "[trie constructor]") {
  SECTION("Default constructors") {
    { StringStringTrie t{}; }

    { Trie<std::string, int> t{}; }

    { Trie<std::vector<int>, char> t{}; }
  }

  SECTION("Copy constructor") {

    StringStringTrie trie{};
    trie.insert("A", "A");
    trie.insert("B", "B");
    trie.insert("AB", "AB");

    StringStringTrie newTrie(trie);

    newTrie.insert("A", "X");
    REQUIRE(newTrie.at("A") == "X");
    REQUIRE(trie.at("A") == "A");

    newTrie["B"] = "C";

    auto it = newTrie.begin();
    REQUIRE(it.value() == "AB");
    ++it;
    REQUIRE(it.value() == "X");
    ++it;
    REQUIRE(it.value() == "C");
  }

  SECTION("move constructor") {
    auto mk_trie = [](Trie<std::string, int> trie) {
      trie["X"] = 42;
      return trie;
    };
    Trie<std::string, int> old_trie;
    Trie<std::string, int> moved_trie = mk_trie(old_trie);
  }

  SECTION("operator= (copy)") {
    StringStringTrie trie{};
    trie.insert("A", "A");
    trie.insert("B", "B");
    trie.insert("AB", "AB");

    StringStringTrie newTrie;
    newTrie = trie;

    newTrie.insert("A", "X");
    REQUIRE(newTrie.at("A") == "X");
    REQUIRE(trie.at("A") == "A");
  }

  SECTION("operator= (move)") {
    StringStringTrie trie{};
    trie = StringStringTrie{};
    trie["A"] = "A";
    REQUIRE(trie.at("A") == "A");
  }
}

TEST_CASE("Add elements to trie and check if they're in the trie", "[trie]") {
  StringStringTrie trie{};
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
  StringStringTrie trie{};
  trie.insert("A", "A");
  trie.insert("B", "B");
  trie.insert("C", "C");
  trie.insert("AA", "AA");
  trie.insert("BB", "BB");
  trie.insert("CC", "CC");

  std::vector<std::pair<std::string, std::string>> expected{
      std::make_pair("AA", "AA"), std::make_pair("A", "A"),
      std::make_pair("BB", "BB"), std::make_pair("B", "B"),
      std::make_pair("CC", "CC"), std::make_pair("C", "C")};

  SECTION("Iterating once with for-each loop") {
    std::vector<std::pair<std::string, std::string>> results{};
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

  SECTION("Iterate on prefix-subtree") {
    StringStringTrie trie{};
    std::vector<std::pair<std::string, std::string>> expected{
        std::make_pair("ABC", "D"), std::make_pair("AB", "B"),
        std::make_pair("AC", "C"), std::make_pair("A", "A")};
    std::vector<std::pair<std::string, std::string>> received;
    std::vector<std::pair<std::string, std::string>> received2;

    trie.insert("A", "A");
    trie.insert("AB", "B");
    trie.insert("AC", "C");
    trie.insert("ABC", "D");
    trie.insert("X", "X");

    for (auto it = trie.subtrie_iterator("A"); it != trie.end(); ++it) {
      received.push_back(*it);
    }

    REQUIRE(received == expected);

    std::string lvalue_prefix = "A";

    for (auto it = trie.subtrie_iterator(lvalue_prefix); it != trie.end();
         ++it) {
      received2.push_back(*it);
    }
    REQUIRE(received2 == expected);

    // Trie not deleted after the subtree iterator is no longer visible
    REQUIRE(trie.at("ABC") == "D");
  }

  SECTION("Iterate on non-existent prefix-subtree") {
    StringStringTrie trie{};

    trie.insert("A", "A");
    trie.insert("AB", "B");
    trie.insert("AC", "C");
    trie.insert("ABC", "D");
    trie.insert("X", "X");

    auto it = trie.subtrie_iterator("I like Haskell");

    REQUIRE(it == trie.end());
  }

  SECTION("Iterating with an explicitly named iterator") {
    std::vector<std::pair<std::string, std::string>> results{};
    for (auto it = trie.begin(); it != trie.end(); ++it) {
      results.push_back(*it);
    }
    REQUIRE(results == expected);
  }

  SECTION("Doing assignments via an iterator") {
    StringStringTrie trie2{};
    trie2.insert("A", "A");
    trie2.insert("B", "B");
    trie2.insert("C", "C");
    trie2.insert("AA", "AA");
    trie2.insert("BB", "BB");
    trie2.insert("CC", "CC");

    std::string teststring;

    for (auto it = trie.begin(); it != trie.end(); ++it) {
      if (it.key() == "AA") {
        it.value() = "XX";
      }
      if (it.key() == "CC") {
        const std::string teststring2(it.value());
        teststring = teststring2;
      }
    }

    REQUIRE(trie["AA"] == "XX");
    REQUIRE(teststring == "CC");
  }
}

TEST_CASE("Retrieve elements from trie", "[trie retrieve]") {
  StringStringTrie trie{};
  trie.insert("A", "A");
  trie.insert("B", "B");
  trie.insert("AB", "AB");

  SECTION("Query for elements that are really there") {
    REQUIRE(trie.at("A") == "A");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    std::string lvalue_key = "A";
    REQUIRE(trie.at(lvalue_key) == "A");
  }

  SECTION("Query for elements that are not there") {
    REQUIRE(trie.at("C") == std::optional<std::string>());
    REQUIRE(trie.at("") == std::optional<std::string>());
    REQUIRE(trie.at("ABC") == std::optional<std::string>());
  }
}

TEST_CASE("Replace elements in the trie", "[trie replace]") {
  SECTION("Without checking the return value") {
    StringStringTrie trie{};
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
    StringStringTrie trie{};
    REQUIRE(trie.insert("A", "A") == std::optional<std::string>());
    REQUIRE(trie.insert("B", "B") == std::optional<std::string>());
    REQUIRE(trie.insert("AB", "AB") == std::optional<std::string>());

    REQUIRE(trie.at("A") == "A");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    REQUIRE(trie.insert("A", "C") == "A");

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "AB");

    REQUIRE(trie.insert("AB", "CD") == "AB");

    // use return value as lvalue
    std::optional<std::string> returned = trie.insert("AB", "Hello World!");
    REQUIRE(returned.value() == "CD");

    REQUIRE(trie.at("A") == "C");
    REQUIRE(trie.at("B") == "B");
    REQUIRE(trie.at("AB") == "Hello World!");
  }
}

TEST_CASE("operator[]", "[trie operator[]]") {
  StringStringTrie trie{};
  trie["A"] = "A";
  REQUIRE(trie.at("A") == "A");
  REQUIRE(trie["A"] == "A");
}

TEST_CASE("Checking all with key type vector<int>",
          "[trie<vector<int>, std::string>]") {
  Trie<std::vector<int>, std::string> trie{};
  trie.insert({1, 2}, "A");
  trie.insert({1}, "B");
  trie.insert({1, 2, 3}, "AB");

  SECTION("Query for elements that are really there") {
    REQUIRE(trie.at({1, 2}) == "A");
    REQUIRE(trie.at({1}) == "B");
    REQUIRE(trie.at({1, 2, 3}) == "AB");
  }

  SECTION("Query for elements that are not there") {
    REQUIRE(trie.at({}) == std::optional<std::string>());
    REQUIRE(trie.at({4, 5}) == std::optional<std::string>());
    REQUIRE(trie.at({1, 3}) == std::optional<std::string>());
  }
}

TEST_CASE("Using the non-default key-converter", "[trie converter]") {
  Trie<int, std::string, IntBitwiseConverter> trie{};
  trie.insert(1, "A");
  trie.insert(0, "B");
  trie.insert(100, "C");
  trie.insert(123873, "D");
  REQUIRE(trie.at(1) == "A");
  REQUIRE(trie[0] == "B");
  REQUIRE(trie[100] == "C");
  REQUIRE(trie[123873] == "D");

  // iterate over all even keys, i.e. all keys starting with bit 0
  auto it = trie.subtrie_iterator(0, 1);
  REQUIRE((*it).first == 0);
  ++it;
  REQUIRE(it.key() == 100);
  ++it;
  REQUIRE(it == trie.end());

  int lval_prefix = 0;
  auto it2 = trie.subtrie_iterator(lval_prefix, 1);
  REQUIRE((*it2).first == 0);
  ++it2;
  REQUIRE((*it2).first == 100);
  ++it2;
  REQUIRE(it2 == trie.end());
}
/***/