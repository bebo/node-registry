#pragma once

#include "nan.h"
#include "string.h"

NAN_METHOD(getValue);
NAN_METHOD(putValue);
NAN_METHOD(createValue);
NAN_METHOD(deleteValue);

class ValueEntity {
  public:
    ValueEntity() : hkey(NULL), value(NULL), size(0) {};
    ~ValueEntity() {
      if (value) {
        free(value);
      }
    };

    HKEY hkey;
    std::wstring subkey;
    std::wstring type;
    std::wstring key;

    void* value;
    DWORD size;
};
