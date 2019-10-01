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

#include <assert.h>
#include <stdlib.h>
#include <node_api.h>
#include "common.h"

typedef struct {
    char* message;
} VerifierData;

VARIANT fp_buff2sa(void* buff, size_t len)
{
    VARIANT v = {};
    SAFEARRAY *sa = NULL;
    char *pData = new char[len];

    sa = SafeArrayCreateVector(VT_UI1, 0, len);
    SafeArrayAccessData(sa, (LPVOID*)&pData);
    CopyMemory(pData, buff, len);
    SafeArrayUnaccessData(sa);
    v.vt = VT_UI1 | VT_ARRAY;
    v.parray = sa;

    return v;
}

static int DoVerify(VerifierData* data, variant_t ftdata, variant_t tmpldata) {
    int ret = -1;

    IDPFPVerificationPtr verifier = NULL;
    IDPFPFeatureSetPtr ftr = NULL;
    IDPFPTemplatePtr tmpl = NULL;

    try {
        verifier.CreateInstance(__uuidof(DPFPVerification));
        ftr.CreateInstance(__uuidof(DPFPFeatureSet));
        tmpl.CreateInstance(__uuidof(DPFPTemplate));

        data->message = NULL;

        if (SUCCEEDED(ftr->Deserialize(ftdata))) {
            if (SUCCEEDED(tmpl->Deserialize(tmpldata))) {
                IDPFPVerificationResultPtr res = verifier->Verify(ftr, tmpl);
                ret = res->Verified ? 0 : 1;
            }
        }
    }
    catch (_com_error& e) {
        _bstr_t err(e.ErrorMessage());
        const char* msg = err;
        data->message = (char*) msg;
    }
    ftr = NULL;
    tmpl = NULL;
    verifier = NULL;

    return ret;
}

static napi_value Verify(napi_env env, napi_callback_info info) {
    VerifierData* data = NULL;
    size_t argc = 2;
    napi_value args[2];

    assert(napi_get_cb_info(env, info, &argc, args, nullptr, ((void**)&data)) == napi_ok);
    if (argc < 2) {
        napi_throw_type_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    bool isBuff0, isBuff1;
    assert(napi_is_arraybuffer(env, args[0], &isBuff0) == napi_ok);
    assert(napi_is_arraybuffer(env, args[1], &isBuff1) == napi_ok);
    if (!isBuff0 || !isBuff1) {
        napi_throw_type_error(env, nullptr, "Argument must both an ArrayBuffer");
        return nullptr;
    }

    void *featBuff, *tmplBuff;
    size_t featLen, tmplLen;
    assert(napi_get_arraybuffer_info(env, args[0], &featBuff, &featLen) == napi_ok);
    assert(napi_get_arraybuffer_info(env, args[1], &tmplBuff, &tmplLen) == napi_ok);

    variant_t vFeature, vTmpl;
    vFeature = variant_t(fp_buff2sa(featBuff, featLen));
    vTmpl = variant_t(fp_buff2sa(tmplBuff, tmplLen));

    int res = DoVerify(data, vFeature, vTmpl);
    if (data->message != NULL) {
        napi_throw_type_error(env, nullptr, data->message);
        return nullptr;
    }

    napi_value result;
    assert(napi_create_int32(env, res, &result) == napi_ok);
    return result;
}

static void DeleteVerifierData(napi_env env, void* data, void* hint) {
    VerifierData* _data = (VerifierData*) data;

    CoUninitialize();

    (void) env;
    (void) hint;
    free(_data);
}

static VerifierData* CreateVerifierData(napi_env env, napi_value exports) {
    VerifierData* result = (VerifierData*) malloc(sizeof(*result));
    assert(napi_wrap(env, exports, result, DeleteVerifierData, NULL, NULL) == napi_ok);
    return result;
}

NAPI_MODULE_INIT(/*env, exports*/) {
    CoInitialize(NULL);
    VerifierData* data = CreateVerifierData(env, exports);
    napi_property_descriptor bindings[] = {
        {"verify", NULL, Verify, NULL, NULL, NULL, napi_enumerable, data}
    };
    assert(napi_define_properties(env, exports, sizeof(bindings) / sizeof(bindings[0]),
        bindings) == napi_ok);

    return exports;
}

//----------------------------------------------------------------------------------

bool APIENTRY DllMain(HMODULE hModule, DWORD  ulReason, LPVOID lpReserved)
{
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

//----------------------------------------------------------------------------------