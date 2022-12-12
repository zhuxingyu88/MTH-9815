
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;

int main() {
    unordered_map<double, int> map1;
    map1[1.] += 1;
    map1[0] -=1;
    for (auto i: map1) {
        cout << i.first << endl;
    }
    return 0;
}