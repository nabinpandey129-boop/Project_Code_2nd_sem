#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <cctype>
using namespace std;

// split a record line on '|' (keeps empty fields, pads to minFields)
inline vector<string> split(const string& s, char d = '|', size_t minFields = 0) {
    vector<string> r; string cur;
    for (char c : s) { if (c == d) { r.push_back(cur); cur.clear(); } else cur += c; }
    r.push_back(cur);
    while (r.size() < minFields) r.push_back("");
    return r;
}
// remove characters that would break the file format
inline string clean(string s) {
    for (char& c : s) if (c == '|' || c == '\n' || c == '\r' || c == '~' || c == ';') c = ' ';
    return s;
}
inline string lower(string s) { for (char& c : s) c = tolower(c); return s; }
inline string pad(int n, int w) { string s = to_string(n); while ((int)s.size() < w) s = "0" + s; return s; }

inline string readLine(const string& prompt) {
    cout << prompt; string s;
    if (!getline(cin, s)) exit(0);
    return s;
}
inline int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt); char* end;
        long v = strtol(s.c_str(), &end, 10);
        if (!s.empty() && *end == '\0') return (int)v;
        cout << "  Enter a valid number.\n";
    }
}

inline string today() {
    time_t t = time(nullptr); tm* l = localtime(&t); char b[11];
    strftime(b, sizeof b, "%Y-%m-%d", l); return b;
}
inline bool isDate(const string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i : {0,1,2,3,5,6,8,9}) if (!isdigit(s[i])) return false;
    int m = stoi(s.substr(5, 2)), d = stoi(s.substr(8, 2));
    return m >= 1 && m <= 12 && d >= 1 && d <= 31;
}
inline bool isTime(const string& s) {
    if (s.size() != 5 || s[2] != ':' || !isdigit(s[0]) || !isdigit(s[1]) || !isdigit(s[3]) || !isdigit(s[4])) return false;
    return stoi(s.substr(0, 2)) < 24 && stoi(s.substr(3, 2)) < 60;
}
inline string readDate(const string& p) { while (true) { string s = readLine(p); if (isDate(s)) return s; cout << "  Use format YYYY-MM-DD.\n"; } }
inline string readTime(const string& p) { while (true) { string s = readLine(p); if (isTime(s)) return s; cout << "  Use format HH:MM (24h).\n"; } }
inline string readNonEmpty(const string& p) { while (true) { string s = readLine(p); if (!s.empty()) return clean(s); cout << "  Cannot be empty.\n"; } }

inline int calcAge(const string& dob) {
    if (!isDate(dob)) return 0;
    int y = stoi(dob.substr(0, 4)), m = stoi(dob.substr(5, 2)), d = stoi(dob.substr(8, 2));
    time_t t = time(nullptr); tm* l = localtime(&t);
    int age = l->tm_year + 1900 - y;
    if (l->tm_mon + 1 < m || (l->tm_mon + 1 == m && l->tm_mday < d)) age--;
    return age < 0 ? 0 : age;
}
