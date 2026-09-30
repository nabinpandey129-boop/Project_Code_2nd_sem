#pragma once
#include "Util.h"

// Abstract base class: every role decides which pages its dashboard shows (polymorphism)
class User {
protected:
    string id, username, password, name;
public:
    User(string i, string u, string p, string n) : id(i), username(u), password(p), name(n) {}
    virtual ~User() {}
    virtual string role() const = 0;
    virtual string pages() const = 0;   // JSON list of dashboard pages, overridden by each role
    virtual string extra1() const { return ""; }
    virtual string extra2() const { return ""; }

    string getId() const { return id; }
    string getUsername() const { return username; }
    string getName() const { return name; }
    bool checkPassword(const string& p) const { return password == p; }
    void setPassword(const string& p) { password = p; }
    string toRecord() const {
        return role() + "|" + id + "|" + username + "|" + password + "|" + name + "|" + extra1() + "|" + extra2();
    }
};

class Admin : public User {
public:
    using User::User;
    string role() const override { return "Admin"; }
    string pages() const override {
        return R"([{"id":"dashboard","label":"Dashboard"},{"id":"patients","label":"Patients"},{"id":"doctors","label":"Doctors"},{"id":"receptionists","label":"Receptionists"},{"id":"appointments","label":"Appointments"},{"id":"bills","label":"Bills"},{"id":"reports","label":"Reports"}])";
    }
};

class Receptionist : public User {
public:
    using User::User;
    string role() const override { return "Receptionist"; }
    string pages() const override {   // no delete, no reports, no staff management
        return R"([{"id":"dashboard","label":"Dashboard"},{"id":"patients","label":"Patients"},{"id":"appointments","label":"Appointments"},{"id":"history","label":"Patient History"},{"id":"bills","label":"Billing"}])";
    }
};

class Doctor : public User {
    string specialization;
    int fee;
public:
    Doctor(string i, string u, string p, string n, string spec, int f)
        : User(i, u, p, n), specialization(spec), fee(f) {}
    string role() const override { return "Doctor"; }
    string extra1() const override { return specialization; }
    string extra2() const override { return to_string(fee); }
    string getSpecialization() const { return specialization; }
    int getFee() const { return fee; }
    string pages() const override {   // medical side only
        return R"([{"id":"dashboard","label":"Dashboard"},{"id":"appointments","label":"My Appointments"},{"id":"patients","label":"Patients"},{"id":"history","label":"Patient History"},{"id":"visit","label":"Add Medical Visit"}])";
    }
};
