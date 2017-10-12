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

    if (entity->type.compare(L"REG_DWORD") == 0) {
      reg_key.ReadValueDW(key, reinterpret_cast<DWORD*>(&entity->value32));
    } else if (entity->type.compare(L"REG_QWORD") == 0) {
      reg_key.ReadInt64(key, &entity->value64);
    } else if (entity->type.compare(L"REG_SZ") == 0) {
      reg_key.ReadValue(key, &entity->value);
    } else if (entity->type.compare(L"REG_EXPAND_SZ") == 0) {
      // UNSUPPORTED FOR NOW
    } else if (entity->type.compare(L"REG_BINARY") == 0) {
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
      : entity(e), WinAsyncWorker(callback), replaceIfKeyExists(replaceIfKeyExists){};
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

    if (entity->type.compare(L"REG_DWORD") == 0) {
      reg_key.WriteValue(key, static_cast<DWORD>(entity->value32));
      reg_key.ReadValueDW(key, reinterpret_cast<DWORD*>(&entity->value32));
    } else if (entity->type.compare(L"REG_QWORD") == 0) {
      reg_key.WriteValue(key, entity->value64);
      reg_key.ReadInt64(key, &entity->value64);
    } else if (entity->type.compare(L"REG_SZ") == 0) {
      reg_key.WriteValue(key, entity->value.c_str());
      reg_key.ReadValue(key, &entity->value);
    } else if (entity->type.compare(L"REG_EXPAND_SZ") == 0) {
      // UNSUPPORTED FOR NOW
    } else if (entity->type.compare(L"REG_BINARY") == 0) {
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

  if (!Nan::Has(object, New("hkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("subkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("type").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("type is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("key").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(Get(object, New("hkey").ToLocalChecked()).ToLocalChecked()->ToString());
  try {
  entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(Get(object, New("subkey").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value utype(Get(object, New("type").ToLocalChecked()).ToLocalChecked()->ToString());
  std::string type(*utype);
  if (!validate_type(type)) {
    Local<Value> argv[] = {New("invalid type. [REG_SZ, REG_DWORD, REG_QWORD]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }
  entity->type = utf8_decode(type);

  v8::String::Utf8Value key(Get(object, New("key").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->key = utf8_decode(*key);

  AsyncQueueWorker(new GetValueWorker(entity, callback));
}

NAN_METHOD(putValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!Nan::Has(object, New("hkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("subkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("type").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("type is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("key").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("value").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("value is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(Get(object, New("hkey").ToLocalChecked()).ToLocalChecked()->ToString());
  try {
  entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(Get(object, New("subkey").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value utype(Get(object, New("type").ToLocalChecked()).ToLocalChecked()->ToString());
  std::string type(*utype);
  if (!validate_type(type)) {
    Local<Value> argv[] = {New("invalid type. [REG_SZ, REG_DWORD, REG_QWORD]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }
  entity->type = utf8_decode(type);

  v8::String::Utf8Value key(Get(object, New("key").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->key = utf8_decode(*key);

  if (type.compare("REG_DWORD") == 0) {
    entity->value32 = Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToInt32()->Value();
  } else if (type.compare("REG_QWORD") == 0) {
    v8::String::Utf8Value value(Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToString());
    std::string::size_type sz = 0;
    entity->value64 = std::stoll(*value, &sz, 0);
  } else if (type.compare("REG_SZ") == 0) {
    v8::String::Utf8Value value(Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToString());
    entity->value = utf8_decode(*value);
  }

  AsyncQueueWorker(new PutValueWorker(entity, callback, true));
}

NAN_METHOD(createValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!Nan::Has(object, New("hkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("subkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("type").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("type is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("key").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("value").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("value is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(Get(object, New("hkey").ToLocalChecked()).ToLocalChecked()->ToString());
  try {
  entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(Get(object, New("subkey").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value utype(Get(object, New("type").ToLocalChecked()).ToLocalChecked()->ToString());
  std::string type(*utype);
  if (!validate_type(type)) {
    Local<Value> argv[] = {New("invalid type. [REG_SZ, REG_DWORD, REG_QWORD]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }
  entity->type = utf8_decode(type);

  v8::String::Utf8Value key(Get(object, New("key").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->key = utf8_decode(*key);

  if (type.compare("REG_DWORD") == 0) {
    entity->value32 = Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToInt32()->Value();
  } else if (type.compare("REG_QWORD") == 0) {
    v8::String::Utf8Value value(Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToString());
    std::string::size_type sz = 0;
    entity->value64 = std::stoll(*value, &sz, 0);
  } else if (type.compare("REG_SZ") == 0) {
    v8::String::Utf8Value value(Get(object, New("value").ToLocalChecked()).ToLocalChecked()->ToString());
    entity->value = utf8_decode(*value);
  }

  AsyncQueueWorker(new PutValueWorker(entity, callback, false));
}

NAN_METHOD(deleteValue)
{
  Local<Object> object = info[0].As<Object>();
  Callback *callback = new Callback(info[1].As<Function>());
  ValueEntity *entity = new ValueEntity;

  if (!Nan::Has(object, New("hkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("hkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("subkey").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("subkey is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  if (!Nan::Has(object, New("key").ToLocalChecked()).FromJust()) {
    Local<Value> argv[] = {New("key is missing").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value hkey(Get(object, New("hkey").ToLocalChecked()).ToLocalChecked()->ToString());
  try {
    entity->hkey = hkey_from_string(*hkey);
  } catch (const char*) {
    Local<Value> argv[] = {New("invalid hkey. [HKLM, HKCU, HKCC, HKCR, HKU]").ToLocalChecked(), Null()};
    callback->Call(2, argv);
    return;
  }

  v8::String::Utf8Value subkey(Get(object, New("subkey").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->subkey = utf8_decode(*subkey);

  v8::String::Utf8Value key(Get(object, New("key").ToLocalChecked()).ToLocalChecked()->ToString());
  entity->key = utf8_decode(*key);

  AsyncQueueWorker(new DeleteValueWorker(entity, callback));
}
