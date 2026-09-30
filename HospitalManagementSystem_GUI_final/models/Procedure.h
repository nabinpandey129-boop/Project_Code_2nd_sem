#pragma once
#include "Test.h"

class Procedure : public Service {
public:
    using Service::Service;
    string type() const override { return "Procedure"; }
};
