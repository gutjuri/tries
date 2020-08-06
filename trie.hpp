#ifndef TRIE_HPP
#define TRIE_HPP

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

#ifdef TRIE_USE_ARRAY
#ifndef TRIE_SIGMA
#define TRIE_SIGMA 256
#endif
#include <array>
#define STORAGE_TYPE std::array<std::shared_ptr<TrieNode>, TRIE_SIGMA>

#elif TRIE_USE_UNORDERED_MAP
#include <map>
#define STORAGE_TYPE std::unordered_map<KeyContent, std::shared_ptr<TrieNode>>

#else
#include <map>
#define STORAGE_TYPE std::map<KeyContent, std::shared_ptr<TrieNode>>
#endif

// Default converter that is used for all KeyTypes that are
// naturally suited to be keys in a trie. This includes all keys that support
// size() and operator[].
template <typename KeyType> struct DummyConverter {
  // KeyContent is the type of the symbols in the key.
  // For example, if the key is a std::string, then KeyContent is char.
  // If the key is a vector<int>, then KeyContent is int.
  using KeyContent =
      typename std::remove_reference<decltype(KeyType{}[0])>::type;

  static KeyContent get_at_index(const KeyType &key, const std::size_t ind) {
    return key[ind];
  }
  static std::size_t size(const KeyType &key) { return key.size(); }
};

// An example converter that enables ints as keys by interpreting an int as a
// sequence of bits. Note that the the lowest-valued bit is considered the first
// symbol and the highest-values bit represents the last symbol.
// This means, that e.g. the ints with prefix '0' are all even-valued ints, and
// all ints with prefix '00' are all ints divisible by 4.
struct IntBitwiseConverter {
  using KeyContent = bool;
  static KeyContent get_at_index(const int &key, const std::size_t ind) {
    return key & (1 << ind);
  }

  static std::size_t size(const int &) {
    return sizeof(int) * 8; // assuming that char has 8 bits.
  }
};

template <typename C, typename KeyType>
concept ConverterType = requires(const KeyType &key, const std::size_t ind) {
  { C::size(key) }
  ->std::same_as<std::size_t>;

  { C::get_at_index(key, ind) }
  ->std::same_as<typename C::KeyContent>;
};

template <typename KeyType, typename ValueType,
          ConverterType<KeyType> Converter = DummyConverter<KeyType>>
class Trie {
private:
  class TrieNode;
  friend class TrieNode;

  using KeyContent = Converter::KeyContent;
  using StorageType = STORAGE_TYPE;

public:
  class Iterator;
  friend class Iterator;

  Trie() : root(std::make_shared<TrieNode>(nullptr, KeyContent{})) {}

  Trie(const Trie &trie) : root(std::make_shared<TrieNode>(*trie.root)) {}

  Trie(Trie &&other) { swap(*this, other); }

  ~Trie() {}

  friend void swap(Trie &t1, Trie &t2) { std::swap(t1.root, t2.root); }

  Trie &operator=(Trie other) {
    swap(*this, other);
    return *this;
  }

  // Inserts a key-value pair into the trie.
  // If the given key is already associated with a value,
  // then this old value is overridden with the new value.
  // This method returns an optional that contains the
  // value previously associated with the given key, or an empty one
  // if there is no such value.
  std::optional<ValueType> insert(const KeyType key,
                                  const ValueType to_insert) {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    std::optional to_insert_o(to_insert);
    insert_at_node->elem.swap(to_insert_o);
    return to_insert_o;
  }

  std::optional<ValueType> at(KeyType &key) const {
    std::shared_ptr<TrieNode> current_node = find_node(key);
    return current_node ? current_node->elem : std::optional<ValueType>();
  }

  std::optional<ValueType> at(KeyType &&key) const {
    std::shared_ptr<TrieNode> current_node = find_node(key);
    return current_node ? current_node->elem : std::optional<ValueType>();
  }

  std::optional<ValueType> &operator[](KeyType key) {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    return insert_at_node->elem;
  }

  const std::optional<ValueType> &operator[](KeyType key) const {
    std::shared_ptr<TrieNode> insert_at_node = mk_path_to_node(key);
    return insert_at_node->elem;
  }

  bool has_key(const KeyType &key) const {
    std::shared_ptr<TrieNode> target_node = find_node(key);
    return target_node && target_node->elem.has_value();
  }

  Iterator begin() { return Iterator(root); }

  Iterator end() { return Iterator(); }

  Iterator subtrie_iterator(const KeyType &prefix) const {
    auto subroot = find_node(prefix);
    return Iterator(subroot);
  }

  Iterator subtrie_iterator(const KeyType &&prefix) const {
    auto subroot = find_node(prefix);
    return Iterator(subroot);
  }

  Iterator subtrie_iterator(const KeyType &prefix, std::size_t len) const {
    auto subroot = find_node(prefix, len);
    return Iterator(subroot);
  }
  Iterator subtrie_iterator(const KeyType &&prefix, std::size_t len) const {
    auto subroot = find_node(prefix, len);
    return Iterator(subroot);
  }

  class Iterator {
  public:
    // no rule-of-three needed because Iterators don't own resources!
    Iterator(std::shared_ptr<TrieNode> root_node)
        : current_node(leftmost_bottommost_node(root_node.get())),
          root(root_node) {}

    Iterator() : current_node(nullptr) {}

    std::pair<KeyType, ValueType> operator*() {
      return std::pair<KeyType, ValueType>(current_node->key.value(),
                                           current_node->elem.value());
    }

    KeyType key() { return current_node->key.value(); }

    ValueType &value() { return current_node->elem.value(); }

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

#ifdef TRIE_USE_ARRAY
      std::shared_ptr<TrieNode> *child_it =
          &(current_node->parent->children[current_node->prefixed_by]);
#else
      // maps are ordered
      auto child_it =
          current_node->parent->children.find(current_node->prefixed_by);
#endif
      ++child_it;
      for (; child_it != current_node->parent->children.end(); ++child_it) {
#ifdef TRIE_USE_ARRAY
        TrieNode *next = leftmost_bottommost_node(child_it->get());
#else
        TrieNode *next = leftmost_bottommost_node(child_it->second.get());
#endif
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
#ifdef TRIE_USE_ARRAY
        leftmost = leftmost_bottommost_node(child.get());
#else
        leftmost = leftmost_bottommost_node(child.second.get());
#endif
        if (leftmost && leftmost->elem.has_value()) {
          return leftmost;
        }
      }
      return subroot->elem.has_value() ? subroot : nullptr;
    }

    TrieNode *current_node;
    const std::shared_ptr<TrieNode> root;
  };

private:
  std::shared_ptr<TrieNode> root;

  // internal constructor for making a subtrie
  Trie(std::shared_ptr<TrieNode> root) : root(root) {}

  // Returns a shared_ptr pointing to the node corresponding
  // to the specified key. If no such key exists in the trie,
  // nullptr is returned.
  std::shared_ptr<TrieNode> find_node(const KeyType &key) const {
    return find_node(key, Converter::size(key));
  }

  std::shared_ptr<TrieNode> find_node(const KeyType &key,
                                      const std::size_t key_size) const {
    std::shared_ptr<TrieNode> current_node = root;

    for (std::size_t pos_in_key = 0; pos_in_key != key_size; pos_in_key++) {
      KeyContent next_node_index = Converter::get_at_index(key, pos_in_key);
      if (!current_node->has_child(next_node_index)) {
        return nullptr;
      }
      current_node = current_node->children.at(next_node_index);
    }
    return current_node;
  }

  // Makes a path to the node corresponding to the key.
  // If the entire path or parts are already available,
  // they are reused.
  std::shared_ptr<TrieNode> mk_path_to_node(const KeyType &key) {
    std::shared_ptr<TrieNode> current_node = root;
    std::size_t key_size = Converter::size(key);

    for (std::size_t pos_in_key = 0; pos_in_key != key_size; pos_in_key++) {
      KeyContent next_node_index = Converter::get_at_index(key, pos_in_key);

      if (!current_node->has_child(next_node_index)) {
        current_node->children[next_node_index] = std::shared_ptr<TrieNode>(
            new TrieNode(current_node.get(), next_node_index));
      }
      current_node = current_node->children.at(next_node_index);
    }
    current_node->key = key;
    return current_node;
  }

  class TrieNode {
    friend class Trie;
    friend class Iterator;

  public:
    TrieNode(TrieNode *parent, KeyContent prefixed_by)
        : elem(), key(), children(), parent(parent), prefixed_by(prefixed_by) {}

    TrieNode(const TrieNode &other) : TrieNode(other, (TrieNode *)nullptr) {}

    TrieNode(const TrieNode &other, TrieNode *parent)
        : elem(other.elem), key(other.key), children([&] {
            StorageType new_children;

      // copy all children and set parent to this.
#ifdef TRIE_USE_ARRAY
            for (std::size_t i = 0; i < other.children.size(); ++i) {
              if (!other.children[i]) {
                continue;
              }
              new_children[i] =
                  std::make_shared<TrieNode>(*other.children[i], this);
            }
#else
            for (auto x : other.children) {
              new_children[x.first] =
                  std::make_shared<TrieNode>(*x.second, this);
            }
#endif
            return new_children;
          }()),
          parent(parent), prefixed_by(other.prefixed_by) {
    }

    ~TrieNode() {}

    TrieNode &operator=(const TrieNode &other) = delete;

    bool has_child(KeyContent ind) const {
#ifdef TRIE_USE_ARRAY
      return children[ind] != nullptr;
#else
      return children.end() != children.find(ind);
#endif
    }

  private:
    std::optional<ValueType> elem;
    std::optional<KeyType> key;
    StorageType children;
    TrieNode *parent;
    KeyContent prefixed_by;
  };
};

#endif