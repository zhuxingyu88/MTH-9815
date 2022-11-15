//
// Created by XingyuZhu on 11/13/22.
//

#ifndef MTH9815_HW1_LINKEDLIST_HPP
#define MTH9815_HW1_LINKEDLIST_HPP

using namespace std;

template<typename T>
class Node {
protected:
    T val;
    Node<T>* next;

public:
    Node() = default;
    Node(T v, Node<T>* n):val(v), next(n) {}
    virtual ~Node() = default;

    virtual T& get_val() {return val;}

    virtual Node<T>*& get_next() {return next;}
};

template<typename T>
class ListIterator {
protected:
    Node<T>* cur;

public:
    ListIterator() = default;
    explicit ListIterator(Node<T>* node): cur(node) {}
    virtual ~ListIterator() = default;

    // Return whether there is another element to return in this iterator
    bool HasNext(){
        return cur->get_next() != nullptr;
    }

    // Return the next element in this iterator
    T& Next() {
        cur = cur->get_next();
        return cur->get_val();
    }

};


template <typename T>
class LinkedList {
protected:
    Node<T>* before_head;
    size_t size;

public:
    LinkedList():size(0) {
        before_head = new Node<T>();
    }

    ~LinkedList() {
        Node<T>* tmp = before_head;
        while (tmp) {
            before_head = before_head->get_next();
            delete tmp;
            tmp = before_head;
        }
    }

    virtual // Add the specified element at the end of the list
    void Add(T& value){
        Node<T>* tmp = before_head;
        while (tmp->get_next()) {
            tmp = tmp->get_next();
        }
        tmp->get_next() = new Node<T>(value, nullptr);
        size++;
    }

    virtual // Add the specified element at the specified index
    void Insert(T& value, int index) {
        if (index > size)
            throw std::out_of_range("LinkedList::_M_range_check");
        Node<T>* tmp = before_head;
        for (int i = 0; i < index; ++i) {
            tmp = tmp->get_next();
        }
        tmp->get_next() = new Node<T>(value, tmp->get_next());
        size++;
    }

    virtual // Get the element at the specified index
    T& Get(int index) {
        if (index >= size)
            throw std::out_of_range("LinkedList::_M_range_check");
        Node<T>* tmp = before_head;
        for (int i = 0; i < index; ++i) {
            tmp = tmp->get_next();
        }
        return tmp->get_next()->get_val();
    }

    virtual // Retrieve the index of the specified element (-1 if it does not exist in the list
    int IndexOf(T& value) {
        Node<T>* tmp = before_head->get_next();
        int ans = 0;
        while (tmp) {
            if (tmp->get_val() == value) {
                return ans;
            }
            ans++;
            tmp = tmp->get_next();
        }
        return -1;
    }

    virtual // Remove the element at the specified index and return it
    T& Remove(int index) {
        if (index >= size)
            throw std::out_of_range("LinkedList::_M_range_check");
        Node<T>* tmp = before_head;
        for (int i = 0; i < index; ++i) {
            tmp = tmp->get_next();
        }
        T& ans = tmp->get_next()->get_val();
        Node<T>* next = tmp->get_next()->get_next();
        delete tmp->get_next();
        tmp->get_next() = next;
        size--;
        return ans;
    }

    virtual // Return an iterator on this list
    ListIterator<T> Iterator() {return ListIterator<T>(before_head);}

    virtual // Return the size of the list
    int Size() {return size;}
};

#endif //MTH9815_HW1_LINKEDLIST_HPP
