//
// Created by XingyuZhu on 11/11/22
//

#include <iostream>
#include <vector>

using namespace std;

template<typename T>
class maxHeap {
protected:
    vector<T> heap;
public:
    void percolate_up(size_t id) {
        while(id > 0 && heap[(id-1)/2] > heap[id]) {
            swap(heap[id], heap[(id-1)/2]);
            id = (id-1)/2;
        }
    }

    void Add(T item) {
        heap.emplace_back(-item);
        percolate_up(heap.size() - 1);
    }

    void percolate_down(size_t id) {
        for(size_t j = 2*id+1; j < heap.size(); j = 2*id+1) {
            if (j < heap.size()-1 && (heap[j+1] < heap[j]))
                j++;
            if (heap[id] <= heap[j])
                break;

            std::swap(heap[id], heap[j]);
            id = j;
        }
    }

    T Pop() {
        T result = -heap[0];
        swap(heap[0], heap[heap.size()-1]);
        heap.pop_back();
        percolate_down(0);
        return result;
    }
};

int main() {
    maxHeap<int> heap{};
    for (int i = 0; i < 5; ++i) {
        heap.Add(i);
    }
    for (int i = 0; i < 5; ++i) {
        cout << heap.Pop() << ' ';
    }
}