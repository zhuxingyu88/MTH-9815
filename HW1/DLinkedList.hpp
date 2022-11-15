//
// Created by Xingyu Zhu on 11/13/22.
//

#ifndef MTH9815_HW1_DLINKEDLIST_HPP
#define MTH9815_HW1_DLINKEDLIST_HPP
//#include "LinkedList.hpp"
using namespace std;

template <typename T>
class DNode{
protected:
    T val;
    DNode<T>* next;
    DNode<T>* prev;

public:
    DNode() = default;
    DNode(T v, DNode<T>* n, DNode<T>* p):val(v), next(n), prev(p) {}
    ~DNode() = default;
    T& get_val() {return val;}
    DNode<T>*& get_next() {return next;}
    DNode<T>*& get_prev() {return prev;}
};

template<typename T>
class ListIterator {
protected:
    DNode<T>* cur;

public:
    ListIterator() = default;
    explicit ListIterator(DNode<T>* node): cur(node) {}
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
class DoublyLinkedList{
protected:
    DNode<T>* after_end;
    DNode<T>* before_head;
    size_t size;
public:
    DoublyLinkedList():size(0) {
        before_head = new DNode<T>();
        after_end = new DNode<T>();
        before_head->get_next() = after_end;
        after_end->get_prev() = before_head;
    }

    ~DoublyLinkedList() {
        DNode<T>* tmp = before_head;
        while (tmp) {
            before_head = before_head->get_next();
            delete tmp;
            tmp = before_head;
        }
    }

    // Add the specified element at the end of the list
    void Add(T& value){
        DNode<T>* tmp = after_end->get_prev();
        after_end->get_prev() = new DNode<T>(value, after_end, tmp);
        tmp->get_next() = after_end->get_prev();
        size++;
    }

    // Add the specified element at the specified index
    void Insert(T& value, int index) {
        if (index > size)
            throw std::out_of_range("LinkedList::_M_range_check");
        if (index + 1 <= size / 2) {
            DNode<T>* tmp = before_head;
            for (int i = 0; i < index; ++i) {
                tmp = tmp->get_next();
            }
            DNode<T>* next = tmp->get_next();
            tmp->get_next() = new DNode<T>(value, next, tmp);
            next->get_prev() = tmp->get_next();
        } else {
            DNode<T>* tmp = after_end;
            for (int i = size; i != index; --i) {
                tmp = tmp->get_prev();
            }
            DNode<T>* prev = tmp->get_prev();
            tmp->get_prev() = new DNode<T>(value, tmp, prev);
            prev->get_next() = tmp->get_prev();
        }
        size++;
    }

    // Get the element at the specified index
    T& Get(int index) {
        if (index >= size)
            throw std::out_of_range("LinkedList::_M_range_check");
        if (index + 1 <= size / 2) {
            DNode<T>* tmp = before_head;
            for (int i = 0; i < index; ++i) {
                tmp = tmp->get_next();
            }
            return tmp->get_next()->get_val();
        } else {
            DNode<T>* tmp = after_end;
            for (int i = size; i != index; --i) {
                tmp = tmp->get_prev();
            }
            return tmp->get_val();
        }
    }

    // Retrieve the index of the specified element (-1 if it does not exist in the list)
    int IndexOf(T& value) {
        DNode<T>* l = before_head->get_next();
        DNode<T>* r = after_end->get_prev();
        int left = 0;
        int right = size - 1;
        while (left <= right) {
            if (l->get_val() == value) {
                return left;
            }
            if (r->get_val() == value) {
                return right;
            }
            left++;
            right--;
            l = l->get_next();
            r = r->get_prev();
        }
        return -1;
    }

    // Remove the element at the specified index and return it
    T& Remove(int index) {
        if (index >= size)
            throw std::out_of_range("LinkedList::_M_range_check");
        if (index + 1 <= size / 2) {
            DNode<T>* tmp = before_head;
            for (int i = 0; i < index; ++i) {
                tmp = tmp->get_next();
            }
            T& ans = tmp->get_next()->get_val();
            DNode<T>* next = tmp->get_next()->get_next();
            delete tmp->get_next();
            tmp->get_next() = next;
            next->get_prev() = tmp;
            size--;
            return ans;
        } else {
            DNode<T>* tmp = after_end;
            for (int i = size-1; i != index; --i) {
                tmp = tmp->get_prev();
            }
            T& ans = tmp->get_prev()->get_val();
            DNode<T>* prev = tmp->get_prev()->get_prev();
            delete tmp->get_prev();
            tmp->get_prev() = prev;
            prev->get_next() = tmp;
            size--;
            return ans;
        }
    }

    // Return an iterator on this list
    ListIterator<T> Iterator() {return ListIterator<T>(before_head);}

//     Return the size of the list
     int Size() {return size;}
};


#endif //MTH9815_HW1_DLINKEDLIST_HPP
