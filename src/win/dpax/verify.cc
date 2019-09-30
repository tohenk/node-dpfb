/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Toha <tohenk@yahoo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <napi.h>
#include "common.h"
#include "fp.h"

static CFPHandler *fp = NULL;

VARIANT fp_buff2sa(Napi::ArrayBuffer buff)
{
    VARIANT v = {};
    SAFEARRAY *sa = NULL;
    size_t sz = buff.ByteLength();
    char *pData = new char[sz];

    sa = SafeArrayCreateVector(VT_UI1, 0, sz - 1);
    SafeArrayAccessData(sa, (LPVOID*)&pData);
    CopyMemory(pData, buff.Data(), sz);
    SafeArrayUnaccessData(sa);
    v.vt = VT_UI1 | VT_ARRAY;
    v.parray = sa;

    return v;
}

static Napi::Value Verify(const Napi::CallbackInfo& info) {
    if (info.Length() < 2) {
        Napi::Error::New(info.Env(), "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    if (!info[0].IsArrayBuffer() || !info[1].IsArrayBuffer()) {
        Napi::Error::New(info.Env(), "Argument must both an ArrayBuffer")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    int ret = -1;
    IDPFPTemplatePtr tmpl = NULL;
    IDPFPFeatureSetPtr ftr = NULL;
    variant_t vTmpl = NULL;
    variant_t vFtr = NULL;

    if (!fp) {
        fp = new CFPHandler();
        fp->Initialize();
    }

    vFtr = _variant_t(fp_buff2sa(info[0].As<Napi::ArrayBuffer>()));
    vTmpl = _variant_t(fp_buff2sa(info[1].As<Napi::ArrayBuffer>()));
    ftr.CreateInstance(__uuidof(DPFPFeatureSet));
    tmpl.CreateInstance(__uuidof(DPFPTemplate));
    try {
        if (SUCCEEDED(ftr->Deserialize(vFtr))) {
            if (SUCCEEDED(tmpl->Deserialize(vTmpl))) {
                int res = fp->Identify(ftr, tmpl);
                switch (res)
                {
                case CFPHandler::FP_VERIFY_MATCHED:
                    ret = 0;
                    break;
                case CFPHandler::FP_VERIFY_NO_MATCH:
                    ret = 1;
                    break;
                default:
                    break;
                }
            }
        }
    }
    catch (_com_error& e) {
        _bstr_t err(e.ErrorMessage());
        const char* msg = err;
        Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    ftr = NULL;
    tmpl = NULL;

    return Napi::Number::New(info.Env(), ret);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "verify"), Napi::Function::New(env, Verify));
  return exports;
}

NODE_API_MODULE(addon, Init)

//----------------------------------------------------------------------------------

bool APIENTRY DllMain(HMODULE hModule, DWORD  ulReason, LPVOID lpReserved)
{
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        CoInitialize(NULL);
        break;
    case DLL_PROCESS_DETACH:
        CoUninitialize();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

//----------------------------------------------------------------------------------