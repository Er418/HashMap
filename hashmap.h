#pragma once

#include <array>
#include <cassert>
#include <algorithm>
#include <functional>
#include <vector>
#include <iterator>
#include <cmath>
#include <climits>
#include <memory>


template<class Key,
        class T,
        class Hash = std::hash<Key> >
class HashMap {  // using coalesced hashing
private:
    class Element {
    public:
        std::pair<const Key, T> data = std::make_pair(Key(), T());
        const Key &key = data.first;
        T &value = data.second;
        bool is_used = false;
        bool is_empty = true;
        bool last = true;
        size_t used = 0;
        size_t next = SIZE_MAX;

        Element() = default;

        Element(Key key, T value, bool is_used = true, bool is_empty = true, bool last = true, size_t next = SIZE_MAX) :
                data(std::make_pair(key, value)), is_used(is_used), is_empty(is_empty), last(last), next(next) {}
    };

    Hash hasher;

    double CELLAR_FACTOR = 1;
    double MAX_LOAD_FACTOR = 0.3; // MAX_LOAD_FACTOR < 1


    const std::array<size_t, 23> sizes = {{5, 11, 23, 47, 97, 197, 397, 797, 1597,
                                           3203, 6421, 12853, 25717, 51437, 102877,
                                           205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977, 28973957}};
    size_t current_size = 0;
    size_t size_id = 1;
    size_t hash_capacity = sizes[size_id];
    size_t capacity = std::ceil(hash_capacity + hash_capacity * CELLAR_FACTOR);
    size_t collision_bucket_id = capacity - 1;

    std::vector<Element *> data = std::vector<Element *>(capacity, nullptr);

    void init() {
        for (auto &element: data) {
            delete element;
            element = new Element();
        }
    }

    void update_collision_bucket_id() {
        while (collision_bucket_id > 0 && data[collision_bucket_id]->is_used) {
            collision_bucket_id--;
        }
        assert(collision_bucket_id != SIZE_MAX);
        if (collision_bucket_id == 0) {
            rehash();
        }
    }

    void rehash() {
        if (current_size > hash_capacity * MAX_LOAD_FACTOR) {
            size_id++;
        }
        hash_capacity = sizes[size_id];
        capacity = std::ceil(hash_capacity + hash_capacity * CELLAR_FACTOR);
        std::vector<Element *> old_data = data;
        data = std::vector<Element *>(capacity, nullptr);
        collision_bucket_id = capacity - 1;
        current_size = 0;
        init();
        for (auto &element: old_data) {
            if (!element->is_empty) {
                insert(element->data);
            }
            delete element;
        }
    }

    size_t get_key_id(const Key &key) const {
        size_t key_hash = hasher(key) % hash_capacity;
        size_t i = key_hash;
        while (!data[i]->last) {
            if (!data[i]->is_empty && data[i]->key == key) {
                return i;
            }
            i = data[i]->next;
            assert(i < capacity);
        }
        if (!data[i]->is_empty && data[i]->key == key) {
            return i;
        }
        return capacity;
    }

public:
    class iterator {
    private:
        friend class HashMap;

        HashMap *table;
        size_t id = 0;

    public:
        iterator() = default;

        iterator(const iterator &other) : table(other.table), id(other.id) {
        }

        iterator &operator++() {
            ++id;
            while (id < table->capacity && table->data[id]->is_empty) {
                ++id;
            }
            return *this;
        }

        iterator &operator=(const iterator &other) {
            if (this == &other) {
                return *this;
            }
            table = other.table;
            id = other.id;
            return *this;
        }

        iterator operator++(int) {
            iterator it = *this;
            ++(*this);
            return it;
        }

        bool operator==(const iterator &other) const {
            return (table == other.table && id == other.id);
        }

        bool operator!=(const iterator &other) const {
            return !((*this) == other);
        }

        std::pair<const Key, T> &operator*() const {
            return table->data[id]->data;
        }

        std::pair<const Key, T> *operator->() const {
            return &(table->data[id]->data);
        }
    };

    class const_iterator {
    private:
        friend class HashMap;

        const HashMap *table;
        size_t id = 0;
    public:
        const_iterator() = default;

        const_iterator(const const_iterator &other) : table(other.table), id(other.id) {
        }

        explicit const_iterator(iterator it) : table(it.table), id(it.id) {}

        const_iterator &operator=(const const_iterator &other) {
            if (this == &other) {
                return *this;
            }
            table = other.table;
            id = other.id;
            return *this;
        }

        const_iterator &operator++() {
            ++id;
            while (id < table->capacity && table->data[id]->is_empty) {
                ++id;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator it = *this;
            ++(*this);
            return it;
        }

        bool operator==(const const_iterator &other) const {
            return (table == other.table && id == other.id);
        }

        bool operator!=(const const_iterator &other) const {
            return !((*this) == other);
        }

        const std::pair<const Key, T> &operator*() const {
            return table->data[id]->data;
        }

        const std::pair<const Key, T> *operator->() const {
            return &(table->data[id]->data);
        }
    };

    const_iterator begin() const {
        const_iterator it;
        it.table = this;
        it.id = 0;
        while (it.id < capacity && data[it.id]->is_empty) {
            ++it.id;
        }
        return it;
    }

    const_iterator end() const {
        const_iterator it;
        it.table = this;
        it.id = capacity;
        return it;
    }

    iterator begin() {
        iterator it;
        it.table = this;
        it.id = 0;
        while (it.id < capacity && data[it.id]->is_empty) {
            ++it.id;
        }
        return it;
    }

    iterator end() {
        iterator it;
        it.table = this;
        it.id = capacity;
        return it;
    }

    explicit HashMap(const Hash &hash = Hash()) : hasher(hash) {
        init();
    }

    HashMap(const HashMap &other) {
        hasher = other.hasher;
        init();
        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
    }

    HashMap(std::iterator<std::forward_iterator_tag, std::pair<Key, T>> begin,
            std::iterator<std::forward_iterator_tag, std::pair<Key, T>> end,
            const Hash &hash = Hash()) {
        hasher = hash;
        init();
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(iterator begin,
            iterator end,
            const Hash &hash = Hash()) {
        hasher = hash;
        init();
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<Key, T>> list, const Hash &hash = Hash()) {
        hasher = hash;
        init();
        for (auto it = list.begin(); it != list.end(); ++it) {
            insert(*it);
        }
    }

    HashMap &operator=(const HashMap &other) {
        if (this == &other) {
            return *this;
        }
        clear();
        hasher = other.hasher;
        init();
        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
        return *this;
    }

    size_t size() const {
        return current_size;
    }

    bool empty() const {
        return (current_size == 0);
    }

    Hash hash_function() const {
        return hasher;
    }

    void clear() {
        for (size_t i = 0; i < capacity; ++i) {
            if (data[i]) {
                delete data[i];
                data[i] = new Element();
            }
        }
        size_id = 0;
        rehash();
    }

    void insert(std::pair<const Key, T> element) {
        if (get_key_id(element.first) != capacity) {
            return;
        }
        size_t key_hash = hasher(element.first) % hash_capacity;
        size_t i = key_hash;
        while (data[i]->is_used && !data[i]->last) {
            i = data[i]->next;
            assert(i != SIZE_MAX);
        }
        current_size++;
        if (!data[i]->is_used) {
            Element *buf = data[i];
            data[i] = new Element(element.first, element.second, true, false, buf->last, buf->next);
            delete buf;
            if (i == collision_bucket_id) {
                update_collision_bucket_id();
            }
        } else {
            data[i]->last = false;
            data[i]->next = collision_bucket_id;
            delete data[collision_bucket_id];
            data[collision_bucket_id] = new Element(element.first, element.second, true, false, true);
            update_collision_bucket_id();
        }
        if (current_size >= capacity * MAX_LOAD_FACTOR) {
            rehash();
        }
    }

    const_iterator find(const Key &key) const {
        size_t id = get_key_id(key);
        const_iterator it;
        it.table = this;
        it.id = id;
        return it;
    }

    iterator find(const Key &key) {
        size_t id = get_key_id(key);
        iterator it;
        it.table = this;
        it.id = id;
        return it;
    }

    void erase(const Key &key) {
        size_t id = get_key_id(key);
        if (id == capacity) {
            return;
        }
        data[id]->is_empty = true;
        current_size--;
    }

    T &operator[](const Key &key) {
        size_t id = get_key_id(key);
        if (id == capacity) {
            insert(std::make_pair(key, T()));
            id = get_key_id(key);
            assert(id != capacity);
        }
        return data[id]->value;
    }

    const T &at(const Key &key) const {
        size_t id = get_key_id(key);
        if (id == capacity) {
            throw std::out_of_range("Key not found");
        }
        return data[id]->value;
    }

    ~HashMap() {
        for (auto it: data) {
            delete it;
        }
    }
};
