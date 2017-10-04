#pragma once

#include "nan.h"

#include <string>
#include <cstdint>
#include <cinttypes>
#include <iostream>

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
using Nan::Set;
using Nan::Null;
using Nan::To;

class WinAsyncWorker : public Nan::AsyncWorker
{
public:
  WinAsyncWorker(Nan::Callback *callback) : Nan::AsyncWorker(callback){};
  ~WinAsyncWorker(){};

protected:
  bool chk(HRESULT hresult, std::string &msg);
  bool chk(HRESULT hresult, const char *msg);
};

const wchar_t *errno_to_text(HRESULT errorNumber);
