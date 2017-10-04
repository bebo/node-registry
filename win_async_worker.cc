#include "win_async_worker.h"

#include <string>
#include <cstdint>
#include <cinttypes>

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

const wchar_t *errno_to_text(HRESULT errorNumber)
{
    const wchar_t *errorMessage;
    DWORD nLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                               NULL,
                               errorNumber,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPTSTR)&errorMessage,
                               0,
                               NULL);
    if (nLen == 0) return L"Unknown Error. ";
    return errorMessage;
}

bool WinAsyncWorker::chk(HRESULT hresult, std::string &msg)
{
    std::string mymsg(msg);
    if (hresult != NOERROR)
    {
      char buffer[512];
      sprintf_s(buffer, 512, "%S(0x%08x)", errno_to_text(hresult), hresult);

      mymsg.append(" - ");
      mymsg.append(buffer);

      SetErrorMessage(mymsg.c_str());
      return false;
    }
    return true;
}

bool WinAsyncWorker::chk(HRESULT hresult, const char *msg)
{
    return chk(hresult, std::string(msg));
}
