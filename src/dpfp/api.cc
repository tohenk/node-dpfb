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

#include "api.h"
#include "worker.h"
#include "acquire.h"
#include "compare.h"
#include "identify.h"

bool fp_get_buffer(napi_env env, napi_value buff, unsigned char** data, unsigned int* len) {
    bool isbuff;
    void* buffdata;
    size_t bufflen;
    assert(napi_ok == napi_is_arraybuffer(env, buff, &isbuff));
    if (isbuff) {
        assert(napi_ok == napi_get_arraybuffer_info(env, buff, &buffdata, &bufflen));
        *data = (unsigned char*) buffdata;
        *len = bufflen;
        return true;
    }
    return false;
}

static napi_value fp_get_features_len(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    if (argc == 0) {
        assert(napi_ok == napi_create_int32(env, api_data->reader->getFeaturesLen(), &res));
    }
    return res;
}

static napi_value fp_get_identification_len(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    if (argc == 0) {
        assert(napi_ok == napi_create_int32(env, api_data->reader->getIdentificationLen(), &res));
    }
    return res;
}

static napi_value fp_get_readers(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    vector<string> readers = api_data->reader->getReaders(true);
    unsigned int cnt = readers.size();
    if (cnt > 0) {
        assert(napi_ok == napi_create_array_with_length(env, cnt, &res));
        for (unsigned int i = 0; i < cnt; i++) {
            napi_value name;
            assert(napi_ok == napi_create_string_utf8(env, readers[i].c_str(),
                NAPI_AUTO_LENGTH, &name));
            assert(napi_ok == napi_set_element(env, res, i, name));
        }
    }
    return res;
}

static napi_value fp_select_reader(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 1;
    napi_value argv[1];
    FP_API_DATA* api_data = NULL;
#ifdef _WIN32
    char reader[MAX_PATH];
#endif
#ifdef __linux__
    char reader[PATH_MAX];
#endif
    size_t len;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, NULL, ((void**)&api_data)));
    if (argc == 1 &&
        napi_ok == napi_get_value_string_utf8(env, argv[0], reader, sizeof(reader), &len)) {
        assert(napi_ok == napi_get_boolean(env, api_data->reader->setReader(reader), &res));
    }
    return res;
}

static napi_value fp_acquire_start(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 2;
    napi_value argv[2];
    napi_valuetype t;
    bool enroll = false;
    FP_API_DATA* api_data = NULL;

    // Accepted signatures:
    // startAcquire(() => {});
    // startAcquire(true, () => {});
    // startAcquire({enroll: true}, () => {});

    assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, NULL, ((void**)&api_data)));
    int idx = 0;
    if (argc == 2) {
        idx = 1;
        assert(napi_ok == napi_typeof(env, argv[0], &t));
        switch (t)
        {
            case napi_object:
                napi_value key, value;
                bool hasEnrollProp;
                assert(napi_ok == napi_create_string_utf8(env, "enroll", NAPI_AUTO_LENGTH, &key));
                assert(napi_ok == napi_has_own_property(env, argv[0], key, &hasEnrollProp));
                if (hasEnrollProp) {
                    assert(napi_ok == napi_get_property(env, argv[0], key, &value));
                    assert(napi_ok == napi_get_value_bool(env, value, &enroll));
                }
                break;
            case napi_boolean:
                assert(napi_ok == napi_get_value_bool(env, argv[0], &enroll));
                break;
            default:
                napi_throw_type_error(env, NULL, "Object required!");
                idx = -1;
                break;
        }
    }
    if (idx >= 0) {
        assert(napi_ok == napi_typeof(env, argv[idx], &t));
        if (t == napi_function) {
            fp_start_acquire(env, api_data, enroll, argv[idx]);
        } else {
            napi_throw_type_error(env, NULL, "Callback is not a function!");
        }
    }
    return res;
}

static napi_value fp_acquire_stop(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 1;
    napi_value callback;
    napi_valuetype t;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &callback, NULL, ((void**)&api_data)));
    assert(napi_ok == napi_typeof(env, callback, &t));
    if (t == napi_function) {
        fp_stop_acquire(env, api_data, callback);
    } else {
        napi_throw_type_error(env, NULL, "Callback is not a function!");
    }
    return res;
}

static napi_value fp_is_acquiring(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    assert(napi_ok == napi_get_boolean(env, api_data->acquire.work != NULL &&
        !api_data->acquire.stopped, &res));
    return res;
}

static napi_value fp_compare(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    FP_API_DATA* api_data = NULL;

    // Accepted signatures:
    // compare(feature, fmd);

    assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, NULL, ((void**)&api_data)));
    return fp_start_compare(env, api_data, argv[0], argv[1]);
}

static napi_value fp_identify(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    FP_API_DATA* api_data = NULL;

    // Accepted signatures:
    // identify(feature, fmds);

    assert(napi_ok == napi_get_cb_info(env, info, &argc, argv, NULL, ((void**)&api_data)));
    return fp_start_identify(env, api_data, argv[0], argv[1]);
}

static napi_value fp_init(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    if (argc == 0) {
        if (!api_data->init) {
            api_data->init = true;
            start_fp_worker(env, api_data);
        }
        assert(napi_ok == napi_get_boolean(env, api_data->init, &res));
    }
    return res;
}

static napi_value fp_exit(napi_env env, napi_callback_info info) {
    napi_value res = NULL;
    size_t argc = 0;
    napi_value argv;
    FP_API_DATA* api_data = NULL;

    assert(napi_ok == napi_get_cb_info(env, info, &argc, &argv, NULL, ((void**)&api_data)));
    if (argc == 0) {
        if (!api_data->exit) {
            api_data->exit = true;
        }
        assert(napi_ok == napi_get_boolean(env, api_data->exit, &res));
    }
    return res;
}

static void free_fp_api_data(napi_env env, void* data, void* hint) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    api_data->exit = true;
    if (api_data->reader != NULL) {
        free(api_data->reader);
    }
    (void) env;
    (void) hint;
    free(api_data);
}

static FP_API_DATA* create_fp_api_data(napi_env env, napi_value exports) {
    FP_API_DATA* api_data = (FP_API_DATA*) malloc(sizeof(*api_data));
    api_data->reader = new DPFPReader();
    api_data->init = false;
    api_data->exit = false;
#ifdef _WIN32
    api_data->msg = NULL;
#endif
    api_data->worker = {NULL};
    api_data->acquire = {true, NULL, NULL, NULL, NULL, NULL};
    api_data->compare = {NULL, NULL, false, NULL, 0, NULL, 0};
    api_data->identify = {NULL, NULL, -1, NULL, NULL, NULL, NULL, 0};
    assert(napi_ok == napi_wrap(env, exports, api_data, free_fp_api_data, NULL, NULL));
    return api_data;
}

NAPI_MODULE_INIT(/*env, exports*/) {
    FP_API_DATA* api_data = create_fp_api_data(env, exports);
    napi_property_descriptor bindings[] = {
        {"init", NULL, fp_init, NULL, NULL, NULL, napi_enumerable, api_data},
        {"exit", NULL, fp_exit, NULL, NULL, NULL, napi_enumerable, api_data},
        {"getFeaturesLen", NULL, fp_get_features_len, NULL, NULL, NULL, napi_enumerable, api_data},
        {"getIdentificationLen", NULL, fp_get_identification_len, NULL, NULL, NULL, napi_enumerable, api_data},
        {"getReaders", NULL, fp_get_readers, NULL, NULL, NULL, napi_enumerable, api_data},
        {"selectReader", NULL, fp_select_reader, NULL, NULL, NULL, napi_enumerable, api_data},
        {"startAcquire", NULL, fp_acquire_start, NULL, NULL, NULL, napi_enumerable, api_data},
        {"stopAcquire", NULL, fp_acquire_stop, NULL, NULL, NULL, napi_enumerable, api_data},
        {"isAcquiring", NULL, fp_is_acquiring, NULL, NULL, NULL, napi_enumerable, api_data},
        {"compare", NULL, fp_compare, NULL, NULL, NULL, napi_enumerable, api_data},
        {"identify", NULL, fp_identify, NULL, NULL, NULL, napi_enumerable, api_data},
    };
    assert(napi_ok == napi_define_properties(env, exports, sizeof(bindings) / sizeof(bindings[0]),
        bindings));

    return exports;
}
