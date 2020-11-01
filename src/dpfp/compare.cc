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

#include "compare.h"

static void fp_do_compare(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    api_data->compare.matched = api_data->reader->compare(
        api_data->compare.feature,
        api_data->compare.featurelen,
        api_data->compare.fmd,
        api_data->compare.fmdlen);
}

static void fp_compare_done(napi_env env, napi_status status, void* data) {
    napi_value res;
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    assert(napi_ok == napi_get_boolean(env, api_data->compare.matched, &res));
    assert(napi_ok == napi_resolve_deferred(env, api_data->compare.deferred, res));
    assert(napi_ok == napi_delete_async_work(env, api_data->compare.work));
    api_data->compare.deferred = NULL;
    api_data->compare.work = NULL;
}

napi_value fp_start_compare(napi_env env, void* data, napi_value feature, napi_value fmd) {
    napi_value res = NULL;
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    napi_value work_name, err;
    bool okay;

    if (api_data->compare.work == NULL) {
        okay = true;
        assert(napi_ok == napi_create_promise(env, &(api_data->compare.deferred), &res));
        if (!fp_get_buffer(env, feature, &api_data->compare.feature, &api_data->compare.featurelen)) {
            assert(napi_ok == napi_create_string_utf8(env, "Feature must be an instance of ArrayBuffer!",
                NAPI_AUTO_LENGTH, &err));
            assert(napi_ok == napi_reject_deferred(env, api_data->compare.deferred, err));
            okay = false;
        }
        if (okay && !fp_get_buffer(env, fmd, &api_data->compare.fmd, &api_data->compare.fmdlen)) {
            assert(napi_ok == napi_create_string_utf8(env, "Fmd must be an instance of ArrayBuffer!",
                NAPI_AUTO_LENGTH, &err));
            assert(napi_ok == napi_reject_deferred(env, api_data->compare.deferred, err));
            okay = false;
        }
        if (okay) {
            assert(napi_ok == napi_create_string_utf8(env, "DPFP N-API Compare Work",
                NAPI_AUTO_LENGTH, &work_name));
            assert(napi_ok == napi_create_async_work(env, NULL, work_name, fp_do_compare, fp_compare_done,
                api_data, &(api_data->compare.work)));
            assert(napi_ok == napi_queue_async_work(env, api_data->compare.work));
        }
    }
    return res;
}
