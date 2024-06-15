#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;

bool isLeapYear(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

int Getmonthday(int month, int year) {
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    else if (month == 2) {
        return isLeapYear(year) ? 29 : 28;
    }
    else {
        return 31;
    }
}

int main() {
    int m;
    cout << "请输入次数" << endl;
    cin >> m;
    while (m--) {
        int year, month, day, addDays;
        cout << "请你依次输入 " << "年" << " -- " << "月" << "日" << "--"<<"增加的天数" <<":<" << endl;
        cin >> year >> month >> day >> addDays;

        while (addDays > 0) {
            int daysInMonth = Getmonthday(month, year);
            if (day + addDays > daysInMonth) {
                addDays -= (daysInMonth - day + 1);
                day = 1; // Reset day to 1
                if (month == 12) {
                    month = 1;
                    year++;
                }
                else {
                    month++;
                }
            }
            else {
                day += addDays;
                addDays = 0;
            }
        }
        printf("增加后的天数是\n");
        printf("%04d-%02d-%02d\n", year, month, day);
    }
    return 0;
}
