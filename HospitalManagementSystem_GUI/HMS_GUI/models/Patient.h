#pragma once
#include "Util.h"

class Patient {
    string id, name, dob, gender, phone, address, bloodGroup, emergency;
public:
    Patient() {}
    Patient(string i, string n, string d, string g, string ph, string a, string b, string e)
        : id(i), name(n), dob(d), gender(g), phone(ph), address(a), bloodGroup(b), emergency(e) {}
    ~Patient() {}

    string getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }
    string getDob() const { return dob; }
    string getGender() const { return gender; }
    string getAddress() const { return address; }
    string getBloodGroup() const { return bloodGroup; }
    string getEmergency() const { return emergency; }
    int age() const { return calcAge(dob); }
    void setId(const string& v) { id = v; }
    void setName(const string& v) { name = v; }
    void setDob(const string& v) { dob = v; }
    void setGender(const string& v) { gender = v; }
    void setPhone(const string& v) { phone = v; }
    void setAddress(const string& v) { address = v; }
    void setBloodGroup(const string& v) { bloodGroup = v; }
    void setEmergency(const string& v) { emergency = v; }

    string toRecord() const {
        return id + "|" + name + "|" + dob + "|" + gender + "|" + phone + "|" + address + "|" + bloodGroup + "|" + emergency;
    }
    static Patient fromRecord(const string& line) {
        auto f = split(line, '|', 8);
        return Patient(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
    }
    void display() const {
        cout << "  Patient ID       : " << id << "\n  Name             : " << name
             << "\n  Date of Birth    : " << dob << " (Age " << age() << ")"
             << "\n  Gender           : " << gender << "\n  Phone            : " << phone
             << "\n  Address          : " << address << "\n  Blood Group      : " << bloodGroup
             << "\n  Emergency Contact: " << emergency << "\n";
    }
};
