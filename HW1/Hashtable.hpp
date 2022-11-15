//
// Created by XingyuZhu on 11/13/22.
//

#ifndef MTH9815_HW1_HASHTABLE_HPP
#define MTH9815_HW1_HASHTABLE_HPP

#include <vector>
#include <forward_list>

using namespace std;

template <typename K>
class Hasher {
public:
    virtual size_t operator() (const K& key)=0;
};

template <typename K>
class EqualityPredicate {
public:
    virtual bool operator() (const K& key1, const K& key2)=0;
};

template <typename K, typename V>
class Hashtable {
protected:
vector<forward_list<pair<K,V> > > table;
size_t size;
Hasher<K>* hasher;
EqualityPredicate<K>* equalityPredicate;

public:
    explicit Hashtable(size_t n):table(vector<forward_list<pair<K,V> > >(n)), size(n) {}

    bool Contain(const K& key) {
        for (auto & i: table[*hasher(key)]) {
            if (*equalityPredicate(i.first, key))
                return true;
        }
        return false;
    }

    V& operator[] (const K& key) {
        for (auto & i: table[*hasher(key)]) {
            if (*equalityPredicate(i.first, key))
                return i.second;
        }
        table[*hasher(key)].push_front(make_pair(key, V{}));
        return table[*hasher(key)].front().second;
    }

    void Insert(const K& key, const V& value) {
        this->operator[](key) = value;
    }

    void Erase(const K& key) {
        for (auto iter = table[*hasher(key)].begin(); iter != table[*hasher(key)].end(); ++iter) {
            if (*equalityPredicate(iter->first, key)) {
                table[*hasher(key)].erase_after(--iter);
                return;
            }
        }
    }

};




#endif //MTH9815_HW1_HASHTABLE_HPP
