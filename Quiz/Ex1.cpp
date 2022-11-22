//
// Created by XingyuZhu on 11/11/22
//

#include <iostream>
#include <vector>

using namespace std;

void quick_sort_helper(std::vector<int> &v, int left, int right) {
    if (left >= right)
        return;
    int pivotat = (left + right) / 2;
    std::vector<int> left_part, right_part;
    for (int i = left; i <= right; ++i) {
        if (i == pivotat)
            continue;
        if (v[i] < v[pivotat])
            left_part.push_back(v[i]);
        else
            right_part.push_back(v[i]);
    }
    pivotat = (int)left_part.size() + left;
    v[pivotat] = v[(left + right) / 2];
    for (int i = left; i < pivotat; ++i) {
        v[i] = left_part[i - left];
    }
    for (int i = pivotat + 1; i <= right; ++i) {
        v[i] = right_part[i - pivotat - 1];
    }
    quick_sort_helper(v, left, pivotat - 1);
    quick_sort_helper(v, pivotat + 1, right);
}

void quick_sort(std::vector<int> &vector) {
    quick_sort_helper(vector, 0, (int)vector.size() - 1);
}

int main() {
    vector<int> a{2,1,4,3};
    quick_sort(a);
    for (int i: a) {
        cout << i << endl;
    }

}