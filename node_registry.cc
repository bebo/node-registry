#include "value_functions.h"

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll)
{
  Nan::Set(target, Nan::New("getValue").ToLocalChecked(),
           Nan::GetFunction(Nan::New<FunctionTemplate>(getValue)).ToLocalChecked());
  Nan::Set(target, Nan::New("putValue").ToLocalChecked(),
           Nan::GetFunction(Nan::New<FunctionTemplate>(putValue)).ToLocalChecked());
  Nan::Set(target, Nan::New("deleteValue").ToLocalChecked(),
           Nan::GetFunction(Nan::New<FunctionTemplate>(deleteValue)).ToLocalChecked());
}

NODE_MODULE(NativeExtension, InitAll)
