//
// Created by XingyuZhu on 11/11/22
//

#include <iostream>
#include "LinkedList.hpp"

using namespace std;

int main() {
    LinkedList<int> linkedList1;
    for (int i = 0; i < 10; ++i) {
        linkedList1.Add(i);
    }
    auto iter = linkedList1.Iterator();
    // 0 1 2 3 4 5 6 7 8 9
    while (iter.HasNext()) {
        cout << iter.Next() << ' ';
    }
    cout << endl;
    // 5
    cout << linkedList1.Get(5) << endl;
    // 6
    int tmp = 6;
    cout << linkedList1.IndexOf(tmp) << endl;

    linkedList1.Remove(3);
    iter = linkedList1.Iterator();
    // 0 1 2 4 5 6 7 8 9
    while (iter.HasNext()) {
        cout << iter.Next() << ' ';
    }
    cout << endl;

    tmp = 3;
    linkedList1.Insert(tmp, 3);
    iter = linkedList1.Iterator();
    // 0 1 2 3 4 5 6 7 8 9
    while (iter.HasNext()) {
        cout << iter.Next() << ' ';
    }
    cout << endl;
    // 10
    cout << linkedList1.Size() << endl;
}