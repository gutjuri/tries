#ifndef TRIE_HPP
#define TRIE_HPP

#include <array>
#include <cassert>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>

template <typename A, typename B>
concept same_as_disregard_ref =
    std::same_as<A, B> || std::same_as<A, B &> || std::same_as<A &, B>;

// A naturally suited trie key type must support the default constructor,
// operator[] and std::size(KeyType).
template <typename KeyType,
          // We use the second parameter because it's not possible to have a
          // "using" statement inside a concept. This parameter is not supposed
          // to be bound by users.
          typename KeyContent =
              typename std::remove_reference<decltype(KeyType{}[0])>::type>
concept NaturallySuitedTrieKeyType = requires(KeyType &key, std::size_t ind) {
  {
    KeyType {}
  }
  ->std::same_as<KeyType>;

  { key[ind] }
  ->same_as_disregard_ref<KeyContent>;

  { std::size(key) }
  ->std::same_as<std::size_t>;
};

// Default converter that is used for all KeyTypes that are
// naturally suited to be keys in a trie. This includes all keys that support
// size() and operator[].
template <NaturallySuitedTrieKeyType KeyType> struct DummyConverter {
  // KeyContent is the type of the symbols in the key.
  // For example, if the key is a std::string, then KeyContent is char.
  // If the key is a vector<int>, then KeyContent is int.
  using KeyContent =
      typename std::remove_reference<decltype(KeyType{}[0])>::type;

  static KeyContent get_at_index(const KeyType &key, const std::size_t ind) {
    // no bounds check necessary because we only use this function internally
    // and guarantee that ind < size(key).
    return key[ind];
  }

  static std::size_t size(const KeyType &key) noexcept {
    return std::size(key);
  }
};

// An example converter that enables ints as keys by interpreting an int as a
// sequence of bits. Note that the the lowest-valued bit is considered the first
// symbol and the highest-values bit represents the last symbol.
// This means, that e.g. the ints with prefix '0' are all even-valued ints, and
// all ints with prefix '00' are all ints divisible by 4.
struct IntBitwiseConverter {
  using KeyContent = bool;
  static KeyContent get_at_index(const int &key,
                                 const std::size_t ind) noexcept {
    // no bounds check necessary because we only use this function internally
    // and guarantee that ind < size(key).
    return key & (1 << ind);
  }

  static std::size_t size(const int &) noexcept {
    return sizeof(int) * 8; // assuming that char has 8 bits.
  }
};

// A converter lets the trie view some object as a sequence of symbols.
// It must provide methods to acquire the symbol at a certain position and the
// key's size. Further it must provide the data-type that symbols have
// (KeyContent).
template <typename C, typename KeyType>
concept ConverterType = requires(const KeyType &key, const std::size_t ind) {
  typename C::KeyContent;

  { C::size(key) }
  ->std::same_as<std::size_t>;

  { C::get_at_index(key, ind) }
  ->std::same_as<typename C::KeyContent>;
};

template <typename KeyType, typename KeyContent, typename ValueType,
          typename StorageType>
struct TrieNode {
  using TrieNode_instance =
      TrieNode<KeyType, KeyContent, ValueType, StorageType>;

  TrieNode(TrieNode_instance *parent, KeyContent prefixed_by)
      : elem(), key(), children(), parent(parent), prefixed_by(prefixed_by) {}

  TrieNode(const TrieNode_instance &other)
      : TrieNode_instance(other, (TrieNode_instance *)nullptr) {}

  TrieNode(const TrieNode_instance &other, TrieNode_instance *parent)
      : elem(other.elem), key(other.key), children(other.children, this),
        parent(parent), prefixed_by(other.prefixed_by) {}

  ~TrieNode() {}

  TrieNode_instance &operator=(const TrieNode_instance &other) = delete;

  bool has_child(KeyContent &ind) const { return children.has_child(ind); }

  std::optional<ValueType> elem;
  std::optional<KeyType> key;
  StorageType children;
  TrieNode_instance *parent;
  KeyContent prefixed_by;
};

// A StorageType is a type that a TrieNode can use to store pointers to its
// children.
template <typename ST, typename KeyType, typename KeyContent,
          typename ValueType>
concept StorageType =
    requires(ST storage, KeyType key, KeyContent keycont,
             TrieNode<KeyType, KeyContent, ValueType, ST> *parent) {

  {
    // Modified copy constructor must support an additional parameter.
    // This is the node's (the storage is part of) parent.
    // This is necessary because copying is non-trivial.
    ST { storage, parent }
  }
  ->std::same_as<ST>;

  {
    ST {}
  }
  ->std::same_as<ST>;

  // is there a child for a given symbol?
  { storage.has_child(keycont) }
  ->std::same_as<bool>;

  // reference to shared_ptr is fine here because this is only used internally
  // inside the trie.
  { storage[keycont] }
  ->std::same_as<
      std::shared_ptr<TrieNode<KeyType, KeyContent, ValueType, ST>> &>;

  // It doesn't really matter if we have a shared_ptr or a reference to a
  // shared_ptr, because these functions are only called internally in the trie.
  { *storage.begin() }
  ->same_as_disregard_ref<
      std::shared_ptr<TrieNode<KeyType, KeyContent, ValueType, ST>>>;

  { *storage.end() }
  ->same_as_disregard_ref<
      std::shared_ptr<TrieNode<KeyType, KeyContent, ValueType, ST>>>;

  { *storage.find(keycont) }
  ->same_as_disregard_ref<
      std::shared_ptr<TrieNode<KeyType, KeyContent, ValueType, ST>>>;
};

// A StorageType using std::map.
template <typename KeyType, typename KeyContent, typename ValueType>
class MapStorage {
public:
  using TrieNode_instance =
      TrieNode<KeyType, KeyContent, ValueType,
               MapStorage<KeyType, KeyContent, ValueType>>;

  using InternalStorageType =
      std::map<KeyContent, std::shared_ptr<TrieNode_instance>>;

  MapStorage() : children() {}
  MapStorage(const MapStorage &other, TrieNode_instance *parent)
      : children([&] {
          InternalStorageType new_children;
          for (auto x : other.children) {
            new_children[x.first] =
                std::make_shared<TrieNode_instance>(*x.second, parent);
          }
          return new_children;
        }()) {}

  ~MapStorage() {}

  MapStorage &operator=(const MapStorage &other) = delete;

  bool has_child(KeyContent ind) const noexcept {
    return children.end() != children.find(ind);
  }
  std::shared_ptr<TrieNode_instance> &operator[](KeyContent key) noexcept {
    return children[key];
  }

  // Custom iterator needed since std::map<..>::iterator iterates over key-value
  // pairs (while we want to iterate over values only).
  class Iterator {
  public:
    Iterator(typename InternalStorageType::iterator it,
             typename InternalStorageType::iterator end)
        : it(it), end(end) {}

    Iterator &operator++() noexcept {
      // no bounds check necessary because we only use this function internally
      // and guarantee that no UB can occur.
      ++it;
      return *this;
    }
    bool operator!=(const Iterator &other) noexcept { return it != other.it; }

    std::shared_ptr<TrieNode_instance> operator*() {
      // no bounds check necessary because we only use this function internally
      // and guarantee that no UB can occur.
      return it->second;
    }

  private:
    typename InternalStorageType::iterator it;
    typename InternalStorageType::iterator end;
  };

  Iterator begin() noexcept {
    return Iterator(children.begin(), children.end());
  }

  Iterator end() noexcept { return Iterator(children.end(), children.end()); }

  Iterator find(KeyContent ind) noexcept {
    return Iterator(children.find(ind), children.end());
  }

private:
  InternalStorageType children;
};

// Basically the same as MapStorage but using a std::unordered_map.
// It has been found that UnorderedMapStorage performs much worse than
// MapStorage in almost all use cases.
template <typename KeyType, typename KeyContent, typename ValueType>
class UnorderedMapStorage {
public:
  using TrieNode_instance =
      TrieNode<KeyType, KeyContent, ValueType,
               UnorderedMapStorage<KeyType, KeyContent, ValueType>>;
  using InternalStorageType =
      std::unordered_map<KeyContent, std::shared_ptr<TrieNode_instance>>;
  UnorderedMapStorage() : children() {}
  UnorderedMapStorage(const UnorderedMapStorage &other,
                      TrieNode_instance *parent)
      : children([&] {
          InternalStorageType new_children;
          for (auto x : other.children) {
            new_children[x.first] =
                std::make_shared<TrieNode_instance>(*x.second, parent);
          }
          return new_children;
        }()) {}
  ~UnorderedMapStorage() {}

  UnorderedMapStorage &operator=(const UnorderedMapStorage &other) = delete;

  bool has_child(KeyContent ind) const noexcept {
    return children.end() != children.find(ind);
  }
  std::shared_ptr<TrieNode_instance> &operator[](KeyContent key) noexcept {
    return children[key];
  }

  class Iterator {
  public:
    Iterator(typename InternalStorageType::iterator it,
             typename InternalStorageType::iterator end)
        : it(it), end(end) {}

    Iterator &operator++() noexcept {
      // no bounds check necessary because we only use this function internally
      // and guarantee that no UB can occur.
      ++it;
      return *this;
    }
    bool operator!=(const Iterator &other) noexcept { return it != other.it; }

    std::shared_ptr<TrieNode_instance> operator*() {
      // no bounds check necessary because we only use this function internally
      // and guarantee that no UB can occur.
      return it->second;
    }

  private:
    typename InternalStorageType::iterator it;
    typename InternalStorageType::iterator end;
  };

  Iterator begin() noexcept {
    return Iterator(children.begin(), children.end());
  }

  Iterator end() noexcept { return Iterator(children.end(), children.end()); }

  Iterator find(KeyContent ind) noexcept {
    return Iterator(children.find(ind), children.end());
  }

private:
  InternalStorageType children;
};

// A StorageType using std::array. Generally speaking, an ArrayStorage has less
// efficient memory usage than a MapStorage but much better performance in
// some cases. See the benchmarks for details.
// The template-parameter size must be equal to the size of the set of possible
// KeyContents (e.g. 256 when KeyContent is char and every char may appear in
// the key).
template <typename KeyType, std::convertible_to<std::size_t> KeyContent,
          typename ValueType, std::size_t size>
class ArrayStorage {
public:
  using TrieNode_instance =
      TrieNode<KeyType, KeyContent, ValueType,
               ArrayStorage<KeyType, KeyContent, ValueType, size>>;
  using InternalStorageType =
      std::array<std::shared_ptr<TrieNode_instance>, size>;

  ArrayStorage() : children() {}

  ArrayStorage(const ArrayStorage &other, TrieNode_instance *parent)
      : children([&] {
          InternalStorageType new_children;
          for (std::size_t i = 0; i < other.children.size(); ++i) {
            if (!other.children[i]) {
              continue;
            }
            new_children[i] =
                std::make_shared<TrieNode_instance>(*other.children[i], parent);
          }
          return new_children;
        }()) {}

  ~ArrayStorage() {}

  ArrayStorage &operator=(const ArrayStorage &other) = delete;

  bool has_child(KeyContent key) const {
    return children.at(static_cast<std::size_t>(key)) != nullptr;
  }

  std::shared_ptr<TrieNode_instance> &operator[](KeyContent key) {
    return children.at(static_cast<std::size_t>(key));
  }

  // No custom iterator type that performs bounds checking needed: We are
  // certain that invalid iterators are never dereferenced, because we only use
  // them internally (Trie::leftmost_bottommost_node()).
  std::shared_ptr<TrieNode_instance> *begin() noexcept {
    return children.begin();
  }

  std::shared_ptr<TrieNode_instance> *end() noexcept { return children.end(); }

  std::shared_ptr<TrieNode_instance> *find(KeyContent key) {
    return &children.at(static_cast<std::size_t>(key));
  }

private:
  InternalStorageType children;
};

// KeyType: Type of Key
// ValueType: Type of values
// Converter: Provides functions to get symbols in the key at specific
// positions and the key's size. If none is specified, operator[] and
// std::size() are used. Must adhere to concept ConverterType<KeyType>.
// Storage: The type of storage to use. Default: MapStorage. Must adhere to
// concept StorageType<KeyType, Converter::KeyContent, ValueType>.
template <
    typename KeyType, typename ValueType,
    ConverterType<KeyType> Converter = DummyConverter<KeyType>,
    StorageType<KeyType, typename Converter::KeyContent, ValueType> Storage =
        MapStorage<KeyType, typename Converter::KeyContent, ValueType>>
class Trie {
private:
  using KeyContent = typename Converter::KeyContent;
  using TrieNode_instance = TrieNode<KeyType, KeyContent, ValueType, Storage>;

public:
  class Iterator;

  Trie() : root(std::make_shared<TrieNode_instance>(nullptr, KeyContent{})) {}

  Trie(const Trie &trie)
      : root(std::make_shared<TrieNode_instance>(*trie.root)) {}

  Trie(Trie &&other) { swap(*this, other); }

  ~Trie() {}

  friend void swap(Trie &t1, Trie &t2) { std::swap(t1.root, t2.root); }

  Trie &operator=(const Trie &other) { return *this = Trie(other); }

  Trie &operator=(Trie &&other) {
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
    std::shared_ptr<TrieNode_instance> insert_at_node = mk_path_to_node(key);
    std::optional to_insert_o(to_insert);
    insert_at_node->elem.swap(to_insert_o);
    return to_insert_o;
  }

  std::optional<ValueType> at(KeyType &key) const {
    std::shared_ptr<TrieNode_instance> current_node = find_node(key);
    return current_node ? current_node->elem : std::optional<ValueType>();
  }

  std::optional<ValueType> at(KeyType &&key) const {
    std::shared_ptr<TrieNode_instance> current_node = find_node(key);
    return current_node ? current_node->elem : std::optional<ValueType>();
  }

  std::optional<ValueType> &operator[](KeyType key) {
    std::shared_ptr<TrieNode_instance> insert_at_node = mk_path_to_node(key);
    return insert_at_node->elem;
  }

  bool has_key(const KeyType &key) const {
    std::shared_ptr<TrieNode_instance> target_node = find_node(key);
    return target_node && target_node->elem.has_value();
  }

  Iterator begin() { return Iterator(root); }

  Iterator end() { return Iterator(); }

  // note that this also works if there is no node with the given prefix.
  Iterator subtrie_iterator(const KeyType &prefix) const {
    std::shared_ptr<TrieNode_instance> subroot = find_node(prefix);
    return Iterator(subroot);
  }

  Iterator subtrie_iterator(const KeyType &&prefix) const {
    std::shared_ptr<TrieNode_instance> subroot = find_node(prefix);
    return Iterator(subroot);
  }

  Iterator subtrie_iterator(const KeyType &prefix, std::size_t len) const {
    std::shared_ptr<TrieNode_instance> subroot = find_node(prefix, len);
    return Iterator(subroot);
  }

  Iterator subtrie_iterator(const KeyType &&prefix, std::size_t len) const {
    std::shared_ptr<TrieNode_instance> subroot = find_node(prefix, len);
    return Iterator(subroot);
  }

  class Iterator {
    friend class Trie<KeyType, ValueType, Converter, Storage>;

  public:
    std::pair<KeyType, ValueType> operator*() {
      assert(current_node);

      // it is not necessary to check if the optionals key and elem actually
      // contain values, because in operator++ we guarantee that only entries
      // containing a value (and thereby also a key) are visited.
      return std::pair<KeyType, ValueType>(current_node->key.value(),
                                           current_node->elem.value());
    }

    KeyType key() {
      assert(current_node);
      return current_node->key.value();
    }

    // A value can be modified via iterator.
    ValueType &value() {
      assert(current_node);
      return current_node->elem.value();
    }

    Iterator &operator++() {
      assert(current_node);
      advance();
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return current_node == other.current_node;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    Iterator(std::shared_ptr<TrieNode_instance> root_node)
        : current_node(leftmost_bottommost_node(root_node.get())),
          root(root_node) {}

    Iterator() : current_node(nullptr), root(nullptr) {}

    void advance() { next_postorder(); }

    void next_postorder() {
      if (current_node == root.get()) {
        current_node = nullptr;
        return;
      }

      // go to parent; find the leftmost node in all subtries to the right that
      // has a value.
      auto child_it =
          current_node->parent->children.find(current_node->prefixed_by);
      ++child_it;
      for (; child_it != current_node->parent->children.end(); ++child_it) {
        TrieNode_instance *next = leftmost_bottommost_node((*child_it).get());
        if (next && next->elem.has_value()) {
          current_node = next;
          return;
        }
      }

      // if there is no node in the subtries to the right with a value, go to
      // parent.
      if (current_node->parent->elem.has_value()) {
        current_node = current_node->parent;
        return;
      }

      // if parent has no value either, go one step up and continue search.
      current_node = current_node->parent;
      next_postorder();
    }

    // Returns a pointer to the leftmost-bottommost node with an attached
    // element in the structure where subroot is the root node. (i.e. the one
    // that is to be traversed first in this structure). If there are no
    // children, return nullptr
    static TrieNode_instance *
    leftmost_bottommost_node(TrieNode_instance *subroot) {
      if (!subroot) {
        return nullptr;
      }

      TrieNode_instance *leftmost = nullptr;
      for (auto it = subroot->children.begin(); it != subroot->children.end();
           ++it) {
        leftmost = leftmost_bottommost_node((*it).get());
        if (leftmost && leftmost->elem.has_value()) {
          return leftmost;
        }
      }
      return subroot->elem.has_value() ? subroot : nullptr;
    }

    TrieNode_instance *current_node;
    const std::shared_ptr<TrieNode_instance> root;
  };

private:
  std::shared_ptr<TrieNode_instance> root;

  // internal constructor for making a subtrie
  Trie(std::shared_ptr<TrieNode_instance> root) : root(root) {}

  // Returns a shared_ptr pointing to the node corresponding
  // to the specified key. If no such key exists in the trie,
  // nullptr is returned.
  std::shared_ptr<TrieNode_instance> find_node(const KeyType &key) const {
    return find_node(key, Converter::size(key));
  }

  // Returns a shared_ptr pointing to the node corresponding
  // to the specified key. If no such key exists in the trie,
  // nullptr is returned. Only the first key_size symbols of the key are
  // considered.
  std::shared_ptr<TrieNode_instance>
  find_node(const KeyType &key, const std::size_t key_size) const {
    std::shared_ptr<TrieNode_instance> current_node = root;

    for (std::size_t pos_in_key = 0; pos_in_key != key_size; pos_in_key++) {
      KeyContent next_node_index = Converter::get_at_index(key, pos_in_key);
      if (!current_node->has_child(next_node_index)) {
        return nullptr;
      }
      current_node = current_node->children[next_node_index];
    }
    return current_node;
  }

  // Makes a path to the node corresponding to the key.
  // If the entire path or parts are already available,
  // they are reused.
  std::shared_ptr<TrieNode_instance> mk_path_to_node(const KeyType &key) {
    std::shared_ptr<TrieNode_instance> current_node = root;
    std::size_t key_size = Converter::size(key);

    for (std::size_t pos_in_key = 0; pos_in_key != key_size; pos_in_key++) {
      KeyContent next_node_index = Converter::get_at_index(key, pos_in_key);

      if (!current_node->has_child(next_node_index)) {
        current_node->children[next_node_index] =
            std::shared_ptr<TrieNode_instance>(
                new TrieNode_instance(current_node.get(), next_node_index));
      }
      current_node = current_node->children[next_node_index];
    }
    current_node->key = key;
    return current_node;
  }
};

#endif