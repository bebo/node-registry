#include "value_functions.h"
#include "win_async_worker.h"
#include "registry.h"

#include <algorithm> 
#include <iostream>
#include <functional>
#include <cctype>
#include <vector>
#include <string>
#include <string.h>
#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <windows.h>

using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::Callback;
using v8::Function;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Value;
using v8::Array;
using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::HandleScope;
using Nan::New;
using Nan::Get;
using Nan::Set;
using Nan::Null;
using Nan::To;

#define REG_SZ_A    "REG_SZ"
#define REG_DWORD_A "REG_DWORD"
#define REG_QWORD_A "REG_QWORD"
#define REG_SZ_W    L"REG_SZ"
#define REG_DWORD_W L"REG_DWORD"
#define REG_QWORD_W L"REG_QWORD"
#define MAX_RETRY   3

std::wstring utf8_decode(const std::string& str) {
  if (str.empty())
    return std::wstring();

  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo( size_needed, 0 );
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
  return wstrTo;
}

std::string utf8_encode(const std::wstring &wstr)
{
  if (wstr.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}

std::string hkey_to_string(HKEY hkey) {
  if (hkey == HKEY_CLASSES_ROOT) return "HKEY_CLASSES_ROOT";
  if (hkey == HKEY_CURRENT_CONFIG) return "HKEY_CURRENT_CONFIG";
  if (hkey == HKEY_CURRENT_USER) return "HKEY_CURRENT_USER";
  if (hkey == HKEY_LOCAL_MACHINE) return "HKEY_LOCAL_MACHINE";
  if (hkey == HKEY_USERS) return "HKEY_USERS";
  return "HKEY_INVALID";
}

HKEY hkey_from_string(std::string hkey) {
  const char *hkey_c = hkey.c_str();
  if (_stricmp(hkey_c, "HKEY_CLASSES_ROOT") == 0) return HKEY_CLASSES_ROOT;
  if (_stricmp(hkey_c, "HKCR") == 0) return HKEY_CLASSES_ROOT;
  if (_stricmp(hkey_c, "HKEY_CURRENT_CONFIG") == 0) return HKEY_CURRENT_CONFIG;
  if (_stricmp(hkey_c, "HKCC") == 0) return HKEY_CURRENT_CONFIG;
  if (_stricmp(hkey_c, "HKEY_CURRENT_USER") == 0) return HKEY_CURRENT_USER;
  if (_stricmp(hkey_c, "HKCU") == 0) return HKEY_CURRENT_USER;
  if (_stricmp(hkey_c, "HKEY_LOCAL_MACHINE") == 0) return HKEY_LOCAL_MACHINE;
  if (_stricmp(hkey_c, "HKLM") == 0) return HKEY_LOCAL_MACHINE;
  if (_stricmp(hkey_c, "HKEY_USERS") == 0) return HKEY_USERS;
  if (_stricmp(hkey_c, "HKU") == 0) return HKEY_USERS;
  throw "HKEY INVALID";
}

bool validate_type(std::string &type) {
  const char *type_c = type.c_str();

  if (_stricmp(type_c, REG_SZ_A) != 0 && 
      _stricmp(type_c, REG_DWORD_A) != 0 &&
      _stricmp(type_c, REG_QWORD_A) != 0
      ) {
    return false;
  }

  transform(type.begin(), type.end(), type.begin(), std::toupper);
  return true;
}

v8::Local<v8::Value> getValueFromObject(v8::Local<v8::Object> options, std::string key) {
  v8::Local<v8::String> v8str = Nan::New<v8::String>(key).ToLocalChecked();
  return Nan::Get(options, v8str).ToLocalChecked();
}

int getIntFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Int32>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

bool getBoolFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Boolean>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

v8::Local<v8::String> getStringFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::String>(getValueFromObject(options, key)).ToLocalChecked();
}

double getDoubleFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<double>(getValueFromObject(options, key)).FromMaybe(0);
}

bool isKeyExists(v8::Local<v8::Object> options, std::string key) {
  v8::Local<v8::String> v8str = Nan::New<v8::String>(key).ToLocalChecked();
  return Nan::Has(options, v8str).FromJust();
}

class GetValueWorker : public WinAsyncWorker
{
protected:
  ValueEntity *entity;

public:
  GetValueWorker(ValueEntity *e, Callback *callback)
      : entity(e), WinAsyncWorker(callback){};
  ~GetValueWorker(){};

  void Execute()
  {
    const wchar_t *subkey = entity->subkey.c_str();
    RegKey reg_key(entity->hkey, subkey, KEY_READ);
    if (!reg_key.Valid()) {
      SetErrorMessage("Failed to open registry");
      return;
    }

    const wchar_t *key = entity->key.c_str();
    if (!reg_key.HasValue(key)) {
      SetErrorMessage("Unable to find key");
      return;
    }

    BYTE data[1024];
    DWORD size = 1024;
    DWORD type;
    LONG result = reg_key.ReadValue(key, data, &size, &type);

    if (type == REG_DWORD) {
      entity->type = L"REG_DWORD";
      entity->value32 = *reinterpret_cast<DWORD*>(data);
    } else if (type == REG_QWORD) {
      entity->type = L"REG_QWORD";
      entity->value64 = *reinterpret_cast<int64_t*>(data);
    } else if (type == REG_SZ) {
      entity->type = L"REG_SZ";
      entity->value = reinterpret_cast<wchar_t*>(data);
    } else if (type == REG_EXPAND_SZ) {
      // UNSUPPORTED FOR NOW
    } else if (type == REG_BINARY) {
      // UNSUPPORTED FOR NOW
    }
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback()
  {
    HandleScope scope;

    Local<Object> obj = Nan::New<Object>();

    std::string hkey = hkey_to_string(entity->hkey);
    Set(obj, New("hkey").ToLocalChecked(), New(hkey).ToLocalChecked());

    std::string subkey = utf8_encode(entity->subkey);
    Set(obj, New("subkey").ToLocalChecked(), New(subkey).ToLocalChecked());

    std::string key = utf8_encode(entity->key);
    Set(obj, New("key").ToLocalChecked(), New(key).ToLocalChecked());

    std::string type = utf8_encode(entity->type);
    Set(obj, New("type").ToLocalChecked(), New(type).ToLocalChecked());

    if (type.compare("REG_DWORD") == 0) {
      Set(obj, New("value").ToLocalChecked(), New(entity->value32));
    } else if (type.compare("REG_QWORD") == 0) {
      std::ostringstream oss;
      oss << entity->value64;
      Set(obj, New("value").ToLocalChecked(), New(oss.str()).ToLocalChecked());
    } else if (type.compare("REG_SZ") == 0) {
      std::string value = utf8_encode(entity->value);
      Set(obj, New("value").ToLocalChecked(), New(value).ToLocalChecked());
    }


    Local<Value> argv[] = {Null(), obj};

    callback->Call(2, argv);
  }
};

class PutValueWorker : public WinAsyncWorker
{
protected:
  ValueEntity *entity;
  bool replaceIfKeyExists;

public:
  PutValueWorker(ValueEntity *e, Callback *callback, bool _replaceIfKeyExists)
      : entity(e), WinAsyncWorker(callback), replaceIfKeyExists(_replaceIfKeyExists){};
  ~PutValueWorker(){};

  void Execute()
  {
    const wchar_t *subkey = entity->subkey.c_str();

    RegKey reg_key(entity->hkey, subkey, KEY_WRITE | KEY_READ);
    if (!reg_key.Valid()) {
      SetErrorMessage("Failed to open registry");
      return;
    }

    const wchar_t *key = entity->key.c_str();
    if (!replaceIfKeyExists && reg_key.HasValue(key)) {
      SetErrorMessage("Key already exists");
      return;
    }

    LONG result = -1;
    for (int i = 0; i < MAX_RETRY && result != ERROR_SUCCESS; i++) {
      if (entity->type.compare(L"REG_DWORD") == 0) {
        result = reg_key.WriteValue(key, static_cast<DWORD>(entity->value32));
      } else if (entity->type.compare(L"REG_QWORD") == 0) {
        result = reg_key.WriteValue(key, entity->value64);
      } else if (entity->type.compare(L"REG_SZ") == 0) {
        result = reg_key.WriteValue(key, entity->value.c_str());
      }

      if (result != ERROR_SUCCESS) {
        Sleep((i + 1) * 10);
        continue;
      }

      BYTE data[1024];
      DWORD size = 1024;
      DWORD type;
      result = reg_key.ReadValue(key, data, &size, &type);

      if (type == REG_DWORD) {
        DWORD reg_data = *reinterpret_cast<DWORD*>(data);
        result = (entity->value32 == reg_data) ? ERROR_SUCCESS : -1;
        entity->type = L"REG_DWORD";
        entity->value32 = reg_data;
      } else if (type == REG_QWORD) {
        int64_t reg_data = *reinterpret_cast<int64_t*>(data);
        result = (entity->value64 == reg_data) ? ERROR_SUCCESS : -1;
        entity->type = L"REG_QWORD";
        entity->value64 = reg_data;
      } else if (type == REG_SZ) {
        wchar_t *reg_data = reinterpret_cast<wchar_t*>(data);
        result = (entity->value.compare(reg_data) == 0) ? ERROR_SUCCESS : -1;
        entity->type = L"REG_SZ";
        entity->value = reg_data;
      }

      if (result != ERROR_SUCCESS) {
        Sleep((i + 1) * 10);
      }
    }

    if (result != ERROR_SUCCESS) {
      if (result == -1){
        SetErrorMessage("Value verification failed.");
      } else {
        char buffer[512];
        size_t size;
        const wchar_t *wbuffer = errno_to_text(result);
        wcstombs_s(&size, buffer, 512, wbuffer, 512);
        SetErrorMessage(buffer);
        delete[] wbuffer;
      }
      return;
    }
  }


  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback()
  {
    HandleScope scope;

    Local<Object> obj = Nan::New<Object>();

    std::string hkey = hkey_to_string(entity->hkey);
    Set(obj, New("hkey").ToLocalChecked(), New(hkey).ToLocalChecked());

    std::string subkey = utf8_encode(entity->subkey);
    Set(obj, New("subkey").ToLocalChecked(), New(subkey).ToLocalChecked());

    std::string key = utf8_encode(entity->key);
    Set(obj, New("key").ToLocalChecked(), New(key).ToLocalChecked());

    std::string type = utf8_encode(entity->type);
    Set(obj, New("type").ToLocalChecked(), New(type).ToLocalChecked());

    if (type.compare("REG_DWORD") == 0) {
      Set(obj, New("value").ToLocalChecked(), New(entity->value32));
    } else if (type.compare("REG_QWORD") == 0) {
      std::ostringstream oss;
      oss << entity->value64;
      Set(obj, New("value").ToLocalChecked(), New(oss.str()).ToLocalChecked());
    } else if (type.compare("REG_SZ") == 0) {
      std::string value = utf8_encode(entity->value);
      Set(obj, New("value").ToLocalChecked(), New(value).ToLocalChecked());
    }

    Local<Value> argv[] = {Null(), obj};

    callback->Call(2, argv);
  }
};

class DeleteValueWorker : public WinAsyncWorker
{
protected:
  ValueEntity *entity;

public:
  DeleteValueWorker(ValueEntity *e, Callback *callback)
      : entity(e), WinAsyncWorker(callback){};
  ~DeleteValueWorker(){};

  void Execute()
  {
    const wchar_t *subkey = entity->subkey.c_str();
    RegKey reg_key(entity->hkey, subkey, KEY_READ | KEY_WRITE);
    if (!reg_key.Valid()) {
      SetErrorMessage("Failed to open registry");
      return;
    }

    const wchar_t *key = entity->key.c_str();
    if (!reg_key.HasValue(key)) {
      SetErrorMessage("Unable to find key");
      return;
    }

    reg_key.DeleteValue(key);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback()
  {
    HandleScope scope;

    Local<Object> obj = Nan::New<Object>();

    std::string hkey = hkey_to_string(entity->hkey);
    Set(obj, New("hkey").ToLocalChecked(), New(hkey).ToLocalChecked());

    std::string subkey = utf8_encode(entity->subkey);
    Set(obj, New("subkey").ToLocalChecked(), New(subkey).ToLocalChecked());

    std::string key = utf8_encode(entity->key);
    Set(obj, New("key").ToLocalChecked(), New(key).ToLocalChecked());

    Local<Value> argv[] = {Null(), obj};

    callback->Call(2, argv);
  }
};


NAN_METHOD(getValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!isKeyExists(object, "hkey")) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "subkey")) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "key")) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(getStringFromObject(object, "hkey"));
  try {
  entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(getStringFromObject(object, "subkey"));
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value key(getStringFromObject(object, "key"));
  entity->key = utf8_decode(*key);

  AsyncQueueWorker(new GetValueWorker(entity, callback));
}

NAN_METHOD(putValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!isKeyExists(object, "hkey")) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "subkey")) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "type")) {
    Local<Value> argv[] = {New("type is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "key")) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "value")) {
    Local<Value> argv[] = {New("value is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(getStringFromObject(object, "hkey"));
  try {
  entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(getStringFromObject(object, "subkey"));
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value utype(getStringFromObject(object, "type"));
  std::string type(*utype);
  if (!validate_type(type)) {
    Local<Value> argv[] = {New("invalid type. [REG_SZ, REG_DWORD, REG_QWORD]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }
  entity->type = utf8_decode(type);

  v8::String::Utf8Value key(getStringFromObject(object, "key"));
  entity->key = utf8_decode(*key);

  if (type.compare("REG_DWORD") == 0) {
    if (!Get(object, New("value").ToLocalChecked()).ToLocalChecked()->IsInt32()) {
      Local<Value> argv[] = {New("value - invalid argument").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    }
    entity->value32 = getIntFromObject(object, "value");
  } else if (type.compare("REG_QWORD") == 0) {
    v8::String::Utf8Value value(getStringFromObject(object, "value"));
    std::string::size_type sz = 0;
    try {
      entity->value64 = std::stoll(*value, &sz, 0);
    } catch (std::invalid_argument) {
      Local<Value> argv[] = {New("value - invalid argument").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    } catch (std::out_of_range) {
      Local<Value> argv[] = {New("value - out of range").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    }
  } else if (type.compare("REG_SZ") == 0) {
    v8::String::Utf8Value value(getStringFromObject(object, "value"));
    entity->value = utf8_decode(*value);
  }

  AsyncQueueWorker(new PutValueWorker(entity, callback, true));
}

NAN_METHOD(createValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!isKeyExists(object, "hkey")) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "subkey")) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "type")) {
    Local<Value> argv[] = {New("type is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "key")) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "value")) {
    Local<Value> argv[] = {New("value is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(getStringFromObject(object, "hkey"));
  try {
    entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(getStringFromObject(object, "subkey"));
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value utype(getStringFromObject(object, "type"));
  std::string type(*utype);
  if (!validate_type(type)) {
    Local<Value> argv[] = {New("invalid type. [REG_SZ, REG_DWORD, REG_QWORD]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }
  entity->type = utf8_decode(type);

  v8::String::Utf8Value key(getStringFromObject(object, "key"));
  entity->key = utf8_decode(*key);

  if (type.compare("REG_DWORD") == 0) {
    if (!Get(object, New("value").ToLocalChecked()).ToLocalChecked()->IsInt32()) {
      Local<Value> argv[] = {New("value - invalid argument").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    }
    entity->value32 = getIntFromObject(object, "value");
  } else if (type.compare("REG_QWORD") == 0) {
    v8::String::Utf8Value value(getStringFromObject(object, "value"));
    std::string::size_type sz = 0;
    try {
      entity->value64 = std::stoll(*value, &sz, 0);
    } catch (std::invalid_argument) {
      Local<Value> argv[] = {New("value - invalid argument").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    } catch (std::out_of_range) {
      Local<Value> argv[] = {New("value - out of range").ToLocalChecked(), Null()};
      callback->Call(2, argv);
      return;
    }
  } else if (type.compare("REG_SZ") == 0) {
    v8::String::Utf8Value value(getStringFromObject(object, "value"));
    entity->value = utf8_decode(*value);
  }

  AsyncQueueWorker(new PutValueWorker(entity, callback, false));
}

NAN_METHOD(deleteValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!isKeyExists(object, "hkey")) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "subkey")) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!isKeyExists(object, "key")) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(getStringFromObject(object, "hkey"));
  try {
    entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(getStringFromObject(object, "subkey"));
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value key(getStringFromObject(object, "key"));
  entity->key = utf8_decode(*key);

  AsyncQueueWorker(new DeleteValueWorker(entity, callback));
}
