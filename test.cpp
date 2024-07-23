#include <iostream>
using namespace std;

int main() {
    string s1, s2;
    cin >> s1;
    cin >> s2;
    int arr[256] = { 0 };
    for (char ch : s2) {
        if (ch != ' ')
        {
            arr[ch] += 1;
        }

    }
    for (int i = 0; i < s1.size(); i++) {
        if (arr[s1[i]] > 0) {
            s1.erase(i, 1);
        }
    }
    
    cout << s1 << endl;
}
// 64 Î»Êä³öÇëÓÃ printf("%lld")