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

#include "acquire.h"

void fp_reader_handler(void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->acquire.q != NULL) {
        FP_ACQUIRE_QUEUE q;
        q.op = FP_ACQUIRE_READER;
        q.data = NULL;
        q.len = 0;
        api_data->acquire.q->push(q);
    }
}

void fp_capture_handler(void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->acquire.q != NULL) {
        FP_ACQUIRE_QUEUE q;
        q.op = FP_ACQUIRE_CAPTURE;
        q.data = api_data->reader->getFeature();
        q.len = api_data->reader->getFeatureSize();
        api_data->acquire.q->push(q);
    }
}

void fp_enroll_handler(void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->acquire.q != NULL) {
        FP_ACQUIRE_QUEUE q;
        q.op = FP_ACQUIRE_ENROLL;
        q.data = api_data->reader->getFmd();
        q.len = api_data->reader->getFmdSize();
        api_data->acquire.q->push(q);
    }
}

static void fp_start_acquire_call(napi_env env, napi_value cb, void* context, void* data) {
    (void) context;
    FP_ACQUIRE_DATA info = *(FP_ACQUIRE_DATA*) data;

    if (env != NULL) {
        napi_value undefined, argv[2];
        assert(napi_ok == napi_create_string_utf8(env, info.status.c_str(), NAPI_AUTO_LENGTH, &argv[0]));
        if (info.data != NULL) {
            void* buff;
            assert(napi_ok == napi_create_arraybuffer(env, info.len, &buff, &argv[1]));
            memcpy_s(buff, info.len, info.data, info.len);
        } else {
            assert(napi_ok == napi_get_undefined(env, &argv[1]));
        }
        assert(napi_ok == napi_get_undefined(env, &undefined));
        assert(napi_ok == napi_call_function(env, undefined, cb, 2, argv, NULL));
    }
    free(data);
}

static void fp_do_start_acquire(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->reader != NULL) {
        api_data->acquire.q = new queue<FP_ACQUIRE_QUEUE>;
        api_data->reader->setContext(data);
        api_data->reader->setReaderHandler(fp_reader_handler);
        api_data->reader->setCaptureHandler(fp_capture_handler);
        api_data->reader->setEnrollHandler(fp_enroll_handler);
        while (true) {
            if (api_data->exit || api_data->acquire.stopped) {
                break;
            }
            api_data->reader->startCapture();
            if (!api_data->acquire.q->empty()) {
                FP_ACQUIRE_QUEUE q = api_data->acquire.q->front();
                FP_ACQUIRE_DATA* d = new FP_ACQUIRE_DATA;
                d->data = NULL;
                switch (q.op)
                {
                    case FP_ACQUIRE_READER:
                        d->status = api_data->reader->getReaders().size() > 0 ? "connected" :
                            "disconnected";
                        break;
                    case FP_ACQUIRE_CAPTURE:
                        d->status = "complete";
                        d->data = q.data;
                        d->len = q.len;
                        break;
                    case FP_ACQUIRE_ENROLL:
                        d->status = "enrolled";
                        d->data = q.data;
                        d->len = q.len;
                        break;
                    default:
                        break;
                }
                if (d->status.length() && api_data->acquire.fn != NULL) {
                    assert(napi_ok == napi_acquire_threadsafe_function(api_data->acquire.fn));
                    assert(napi_ok == napi_call_threadsafe_function(api_data->acquire.fn, d,
                        napi_tsfn_blocking));
                    assert(napi_ok == napi_release_threadsafe_function(api_data->acquire.fn,
                        napi_tsfn_release));
                }
                api_data->acquire.q->pop();
            }
            Sleep(10);
        }
        api_data->reader->setContext(NULL);
        api_data->reader->setReaderHandler(NULL);
        api_data->reader->setCaptureHandler(NULL);
        api_data->reader->setEnrollHandler(NULL);
        free(api_data->acquire.q);
        api_data->acquire.q = NULL;
    }
}

static void fp_start_acquire_done(napi_env env, napi_status status, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    assert(napi_ok == napi_release_threadsafe_function(api_data->acquire.fn, napi_tsfn_release));
    assert(napi_ok == napi_delete_async_work(env, api_data->acquire.work));
    api_data->acquire.work = NULL;
    api_data->acquire.fn = NULL;
}

bool fp_start_acquire(napi_env env, void* data, bool enroll, napi_value callback) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    napi_value work_name;

    if (api_data->acquire.work == NULL) {
        assert(napi_ok == napi_create_string_utf8(env, "DPFP N-API Acquire Work",
            NAPI_AUTO_LENGTH, &work_name));
        assert(napi_ok == napi_create_threadsafe_function(env, callback, NULL, work_name, 0, 1,
            NULL, NULL, NULL, fp_start_acquire_call, &(api_data->acquire.fn)));
        assert(napi_ok == napi_create_async_work(env, NULL, work_name, fp_do_start_acquire, fp_start_acquire_done,
            api_data, &(api_data->acquire.work)));
        api_data->acquire.stopped = false;
        api_data->reader->setMode(enroll ? DPFPReader::MODE_ENROLL : DPFPReader::MODE_CONTINOUS_CAPTURE);
        assert(napi_ok == napi_queue_async_work(env, api_data->acquire.work));
        return true;
    }
    return false;
}

static void fp_stop_acquire_call(napi_env env, napi_value cb, void* context, void* data) {
    (void) context;

    if (env != NULL) {
        napi_value undefined;
        assert(napi_ok == napi_get_undefined(env, &undefined));
        assert(napi_ok == napi_call_function(env, undefined, cb, 0, NULL, NULL));
    }
    free(data);
}

static void fp_do_stop_acquire(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->reader != NULL) {
        assert(napi_ok == napi_acquire_threadsafe_function(api_data->acquire.sfn));
        api_data->reader->stopCapture();
        api_data->acquire.stopped = true;
        while (true) {
            if (api_data->exit || api_data->acquire.work == NULL) {
                break;
            }
            Sleep(10);
        }
        assert(napi_ok == napi_call_threadsafe_function(api_data->acquire.sfn, NULL,
            napi_tsfn_blocking));
        assert(napi_ok == napi_release_threadsafe_function(api_data->acquire.sfn, napi_tsfn_release));
    }
}

static void fp_stop_acquire_done(napi_env env, napi_status status, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    assert(napi_ok == napi_release_threadsafe_function(api_data->acquire.sfn, napi_tsfn_release));
    assert(napi_ok == napi_delete_async_work(env, api_data->acquire.swork));
    api_data->acquire.swork = NULL;
    api_data->acquire.sfn = NULL;
}

bool fp_stop_acquire(napi_env env, void* data, napi_value callback)
{
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    napi_value work_name;

    if (api_data->acquire.swork == NULL && api_data->acquire.work != NULL && !api_data->acquire.stopped) {
        assert(napi_ok == napi_create_string_utf8(env, "DPFP N-API Acquire Stop Work",
            NAPI_AUTO_LENGTH, &work_name));
        assert(napi_ok == napi_create_threadsafe_function(env, callback, NULL, work_name, 0, 1,
            NULL, NULL, NULL, fp_stop_acquire_call, &(api_data->acquire.sfn)));
        assert(napi_ok == napi_create_async_work(env, NULL, work_name, fp_do_stop_acquire, fp_stop_acquire_done,
            api_data, &(api_data->acquire.swork)));
        assert(napi_ok == napi_queue_async_work(env, api_data->acquire.swork));
        return true;
    }
    return false;
}
