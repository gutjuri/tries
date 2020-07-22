#include "trie.hpp"


int main() {
  Trie<int, std::string, 2> trie(Trie<int, std::string, 2>::to_key);

  trie.insert(2, "Leute");

}