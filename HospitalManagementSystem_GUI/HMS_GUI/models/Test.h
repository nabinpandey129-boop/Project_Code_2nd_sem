#pragma once
#include "Util.h"

// Base class for hospital services (price is stored here, doctor never types it)
class Service {
protected:
    string id, name;
    int price;
public:
    Service(string i, string n, int p) : id(i), name(n), price(p) {}
    virtual ~Service() {}
    virtual string type() const = 0;
    string getId() const { return id; }
    string getName() const { return name; }
    int getPrice() const { return price; }
    string toRecord() const { return type() + "|" + id + "|" + name + "|" + to_string(price); }
};

class Test : public Service {
public:
    using Service::Service;
    string type() const override { return "Test"; }
};
