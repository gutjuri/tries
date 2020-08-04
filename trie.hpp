#ifndef TRIE_HPP
#define TRIE_HPP

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <utility>

template <typename K, typename V> class Trie {
private:
  class Iterator;
  friend class Iterator;
  class TrieNode;
  friend class TrieNode;

  // KeyContent is the type of the symbols in the key.
  // For example, if the key is a std::string, then KeyContent is char.
  // If the key is a vector<int>, then KeyContent is int.
  using KeyContent = typename std::remove_reference<decltype(K{}[0])>::type;

public:
  Trie() : root(std::make_shared<TrieNode>(nullptr, KeyContent{})) {}

  Trie(const Trie &trie) : root(std::make_shared<TrieNode>(*trie.root)) {}

  Trie(Trie &&other) { swap(*this, other); }

  ~Trie() {}

  friend void swap(Trie &t1, Trie &t2) { std::swap(t1.root, t2.root); }

  Trie &operator=(Trie other) {
    swap(*this, other);
    return *this;
  }

  // Inserts a key-value pair in into the trie.
  // If the given key is already associated with a value,
  // then this old value is overridden with the new value.
  // This method returns an optional that contains the
  // value previously associated with the given key, or an empty one
  // if there is no such value.
  std::optional<V> insert(K key, V to_insert) {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    std::optional to_insert_o(to_insert);
    insert_at_node->elem.swap(to_insert_o);
    return to_insert_o;
  }

  std::optional<V> at(K key) const {
    std::shared_ptr<TrieNode> current_node = find_node(key);
    return current_node ? current_node->elem : std::optional<V>();
  }

  std::optional<V> &operator[](K key) {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    return insert_at_node->elem;
  }

  const std::optional<V> &operator[](K key) const {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    return insert_at_node->elem;
  }

  bool has_key(K key) {
    std::shared_ptr<TrieNode> target_node = find_node(key);
    return target_node && target_node->elem.has_value();
  }

  Iterator begin() { return Iterator(root); }

  Iterator end() { return Iterator(); }

  Trie<K, V> subtrie(K prefix) {
    auto subroot = find_node(prefix);
    return Trie(subroot);
  }

private:
  std::shared_ptr<TrieNode> root;

  // internal constructor for making a subtrie
  Trie(std::shared_ptr<TrieNode> root) : root(root) {}

  // Returns a shared_ptr pointing to the node corresponding
  // to the specified key. If no such key exists in the trie,
  // nullptr is returned.
  std::shared_ptr<TrieNode> find_node(K key) const {
    auto current_node = root;
    for (std::size_t pos_in_key = 0; pos_in_key != key.size(); pos_in_key++) {
      if (!current_node->has_child(key[pos_in_key])) {
        return nullptr;
      }
      current_node = current_node->children.at(key[pos_in_key]);
    }
    return current_node;
  }

  // Makes a path to the node corresponding to the key.
  // If the entire path or parts are already available,
  // they are reused.
  std::shared_ptr<TrieNode> mk_path_to_node(K key) {
    std::shared_ptr<TrieNode> current_node = root;
    for (std::size_t pos_in_key = 0; pos_in_key != key.size(); pos_in_key++) {
      if (!current_node->has_child(key[pos_in_key])) {
        current_node->children[key[pos_in_key]] = std::shared_ptr<TrieNode>(
            new TrieNode(current_node.get(), key[pos_in_key]));
      }
      current_node = current_node->children.at(key[pos_in_key]);
    }
    current_node->key = key;
    return current_node;
  }

  class Iterator {
  public:
    // no rule-of-three needed because Iterators don't own resources!
    Iterator(std::shared_ptr<TrieNode> root_node)
        : current_node(leftmost_bottommost_node(root_node.get())),
          root(root_node) {}

    Iterator() : current_node(nullptr) {}

    std::pair<K, V> operator*() {
      return std::pair<K, V>(current_node->key.value(),
                             current_node->elem.value());
    }

    K key() { return current_node->key.value(); }

    V &value() { return current_node->elem.value(); }

    Iterator &operator++() {
      advance();
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return current_node == other.current_node;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    void advance() { next_postorder(); }

    void next_postorder() {
      if (current_node == root.get()) {
        current_node = nullptr;
        return;
      }

      // maps are ordered
      auto child_it =
          current_node->parent->children.find(current_node->prefixed_by);
      ++child_it;
      for (; child_it != current_node->parent->children.end(); ++child_it) {
        TrieNode *next = leftmost_bottommost_node(child_it->second.get());
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
      TrieNode *leftmost = nullptr;
      for (auto child : subroot->children) {
        leftmost = leftmost_bottommost_node(child.second.get());
        if (leftmost && leftmost->elem.has_value()) {
          return leftmost;
        }
      }
      return subroot->elem.has_value() ? subroot : nullptr;
    }

    TrieNode *current_node;
    const std::shared_ptr<TrieNode> root;
  };

  class TrieNode {
    friend class Trie;
    friend class Iterator;

  public:
    TrieNode(TrieNode *parent, KeyContent prefixed_by)
        : elem(), key(), children(), parent(parent), prefixed_by(prefixed_by) {}

    TrieNode(const TrieNode &other)
        : elem(other.elem), key(other.key), children([&] {
            std::map<KeyContent, std::shared_ptr<TrieNode>> new_children;
            for (auto x : other.children) {
              new_children[x.first] = std::make_shared<TrieNode>(*x.second);
            }
            return new_children;
          }()),
          parent(other.parent), prefixed_by(other.prefixed_by) {}

    ~TrieNode() {}

    TrieNode &operator=(const TrieNode &other) = delete;

    bool has_child(KeyContent ind) const {
      return children.end() != children.find(ind);
    }

  private:
    std::optional<V> elem;
    std::optional<K> key;
    std::map<KeyContent, std::shared_ptr<TrieNode>> children;
    TrieNode *parent;
    KeyContent prefixed_by;
  };
};

#endif