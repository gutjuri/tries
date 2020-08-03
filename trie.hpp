#ifndef TRIE_HPP
#define TRIE_HPP

#include <array>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

template <typename V> class Trie {
private:
  friend class Iterator;
  class TrieNode;

public:
  Trie() : root(nullptr, '\0') {}

  std::optional<V> insert(std::string key, V to_insert) {
    TrieNode *insert_at_node = &root;
    for (std::size_t pos_in_key = 0; pos_in_key != key.size(); pos_in_key++) {
      if (!insert_at_node->children[key[pos_in_key]]) {
        insert_at_node->children[key[pos_in_key]] =
            new TrieNode(insert_at_node, key[pos_in_key]);
      }
      insert_at_node = insert_at_node->children[key[pos_in_key]];
    }
    std::optional to_insert_o(to_insert);
    insert_at_node->elem.swap(to_insert_o);
    return to_insert_o;
  }

  bool has_key(std::string key) {
    TrieNode *current_node = &root;
    for (std::size_t pos_in_key = 0; pos_in_key != key.size(); pos_in_key++) {
      if (!current_node->children[key[pos_in_key]]) {
        return false;
      }
      current_node = current_node->children[key[pos_in_key]];
    }
    return current_node->elem.has_value();
  }

  class Iterator {
  public:
    Iterator(Trie &trie)
        : current_node(leftmost_bottommost_node(&trie.root)), root(&trie.root) {}
    Iterator() : current_node(nullptr) {}

    V &operator*() { return current_node->elem.value(); }
    Iterator& operator++() {
      advance();
      return *this;
    }

    bool operator==(const Iterator& other) {
      return current_node == other.current_node;
    }

    bool operator!=(const Iterator& other) {
      return !(*this == other);
    }

  private:
    void advance() { next_postorder(); }

    void next_postorder() {
      if (current_node == root) {
        current_node = nullptr;
        return;
      }

      for (std::size_t child_i = current_node->prefixed_by + 1; child_i < 256;
           ++child_i) {
        TrieNode *next =
            leftmost_bottommost_node(current_node->parent->children[child_i]);
        if (next && next->elem.has_value()) {
          current_node = next;
          return;
        }
      }
      if (current_node->parent->elem.has_value()) {
        current_node = current_node->parent;
        return;
      }
      current_node = current_node->parent;
      next_postorder();
    }

    // Returns a pointer to the leftmost-bottommost node in the structure
    // (i.e. the one that is to be traversed first in this structure).
    static TrieNode *leftmost_bottommost_node(TrieNode *subroot) {
      if (!subroot) {
        return nullptr;
      }
      TrieNode *leftmost = nullptr;
      for (auto child : subroot->children) {
        leftmost = leftmost_bottommost_node(child);
        if (leftmost && leftmost->elem.has_value()) {
          return leftmost;
        }
      }
      return subroot->elem.has_value() ? subroot : nullptr;
    }

    TrieNode *current_node;
    const TrieNode *root;
  };

  Iterator begin() { return Iterator(*this); }

  Iterator end() { return Iterator(); }

private:
  TrieNode root;

  class TrieNode {
    friend class Trie;
    friend class Iterator;

  public:
    TrieNode(TrieNode *parent, char prefixed_by)
        : elem(), children(), parent(parent), prefixed_by(prefixed_by) {}

    ~TrieNode() {}

  private:
    std::optional<V> elem;
    std::array<TrieNode *, 256> children;
    TrieNode *parent;
    char prefixed_by;
  };
};

#endif