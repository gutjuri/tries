#include "trie.hpp"
#include <iostream>
#include <string>
#include <vector>

struct test {
  int x;
  char *s;
};

void f(test &t) {
  std::vector<test> v;
  v.push_back(t);
  v[0].x = 12;
}

int main() {
  Trie<std::string> t{};
  t.insert("ayy", "lmao");
  t.insert("ok", "ok");
  bool contains = t.has_key("ayy");
  bool contains2 = t.has_key("lmao");

  std::cout << contains << " " << contains2 << std::endl;

  for (auto x : t) {
    std::cout << x << std::endl;
  }

  t.insert("hallo", "welt");

  for (auto x : t) {
    std::cout << x << std::endl;
  }
}