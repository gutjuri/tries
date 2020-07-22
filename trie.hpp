#ifndef TRIE_HPP
#define TRIE_HPP

#include <array>
#include <bitset>
#include <cstdlib>
#include <deque>
#include <functional>

constexpr std::size_t pow2(std::size_t exp) {
  std::size_t ret = 1;
  for (size_t i = 0; i < exp; i++) {
    ret *= 2;
  }
  return ret;
}

template <typename K, typename V, std::size_t O> class Trie {
public:
  Trie(std::function<std::deque<std::bitset<O>>(K)> converter)
      : elem(nullptr), children(std::array<Trie *, O>()), converter(converter) {
  }

  V insert(K key, V val) {
    std::deque<std::bitset<O>> key_encoded = converter(key);
    V *val_copied = new V{val};
    return insert(key_encoded, val_copied, 0);
  }

  // STATIC CONVERTERS
  static std::deque<std::bitset<O>> to_key(int key) {
    std::deque<std::bitset<O>> keybits{};
    while (key != 0) {
      keybits.push_front(std::bitset<O>(key & pow2(O)));
      key >>= O;
    }
    return keybits;
  }

private:
  V *elem;
  std::array<Trie *, pow2(O)> children;
  std::function<std::deque<std::bitset<O>>(K)> converter;

  V insert(std::deque<std::bitset<O>> &key_encoded, V *val,
           std::size_t key_ind) {
    if (key_ind == key_encoded.size()) {
      auto prev = elem;
      elem = val;
      return *prev;
    }
    auto next_subtrie_index = key_encoded[key_ind].to_ulong();
    if (!children[next_subtrie_index]) {
      children[next_subtrie_index] = new Trie(converter);
    }
    return children[next_subtrie_index]->insert(key_encoded, val, key_ind + 1);
  }
};

#endif