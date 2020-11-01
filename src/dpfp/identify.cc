/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2020 Toha <tohenk@yahoo.com>
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

#include "identify.h"

static void fp_do_identify(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    api_data->identify.matched = api_data->reader->identify(
        api_data->identify.feature,
        api_data->identify.featurelen,
        api_data->identify.fmds,
        api_data->identify.fmdlens,
        api_data->identify.fmdcnt);
}

static void fp_identify_done(napi_env env, napi_status status, void* data) {
    napi_value res;
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    assert(napi_ok == napi_create_int32(env, api_data->identify.matched, &res));
    assert(napi_ok == napi_resolve_deferred(env, api_data->identify.deferred, res));
    assert(napi_ok == napi_delete_async_work(env, api_data->identify.work));
    api_data->identify.deferred = NULL;
    api_data->identify.work = NULL;
}

napi_value fp_start_identify(napi_env env, void* data, napi_value feature, napi_value fmds) {
    napi_value res = NULL;
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    napi_value work_name, err;
    bool okay, value_is_array;
    unsigned int len = 1;
    unsigned char* buff;
    unsigned int bufflen;
    if (api_data->identify.work == NULL) {
        okay = true;
        assert(napi_ok == napi_create_promise(env, &(api_data->identify.deferred), &res));
        if (!fp_get_buffer(env, feature, &api_data->identify.feature, &api_data->identify.featurelen)) {
            assert(napi_ok == napi_create_string_utf8(env, "Feature must be an instance of ArrayBuffer!",
                NAPI_AUTO_LENGTH, &err));
            assert(napi_ok == napi_reject_deferred(env, api_data->identify.deferred, err));
            okay = false;
        }
        if (okay) {
            assert(napi_ok == napi_is_array(env, fmds, &value_is_array));
            if (value_is_array) {
                assert(napi_ok == napi_get_array_length(env, fmds, &len));
            } else if (!fp_get_buffer(env, fmds, &buff, &bufflen)) {
                assert(napi_ok == napi_create_string_utf8(env, "FMD must be an instance of ArrayBuffer!",
                    NAPI_AUTO_LENGTH, &err));
                assert(napi_ok == napi_reject_deferred(env, api_data->identify.deferred, err));
                okay = false;
            }
        }
        if (okay) {
            api_data->identify.fmdcnt = len;
            api_data->identify.fmds = new unsigned char*[len];
            api_data->identify.fmdlens = new unsigned int[len];
            if (!value_is_array) {
                api_data->identify.fmds[0] = buff;
                api_data->identify.fmdlens[0] = bufflen;
            } else {
                for (unsigned int i = 0; i < len; i++) {
                    napi_value fmd;
                    assert(napi_ok == napi_get_element(env, fmds, i, &fmd));
                    if (!fp_get_buffer(env, fmd, &buff, &bufflen)) {
                        assert(napi_ok == napi_create_string_utf8(env, "Array element must be an instance of ArrayBuffer!",
                            NAPI_AUTO_LENGTH, &err));
                        assert(napi_ok == napi_reject_deferred(env, api_data->identify.deferred, err));
                        okay = false;
                        break;
                    }                    
                    api_data->identify.fmds[i] = buff;
                    api_data->identify.fmdlens[i] = bufflen;
                }
            }
        }
        if (okay) {
            assert(napi_ok == napi_create_string_utf8(env, "DPFP N-API Identify Work",
                NAPI_AUTO_LENGTH, &work_name));
            assert(napi_ok == napi_create_async_work(env, NULL, work_name, fp_do_identify, fp_identify_done,
                api_data, &(api_data->identify.work)));
            assert(napi_ok == napi_queue_async_work(env, api_data->identify.work));
        }
    }
    return res;
}
