#pragma once

#include "nan.h"
#include "string.h"

NAN_METHOD(getValue);
NAN_METHOD(putValue);
NAN_METHOD(createValue);
NAN_METHOD(deleteValue);

class ValueEntity {
  public: 
    ValueEntity() : hkey(NULL){};
    ~ValueEntity() {
    };

    HKEY hkey;
    std::wstring subkey;
    std::wstring type;
    std::wstring key;

    std::wstring value;
    int32_t value32;
    int64_t value64;
};
