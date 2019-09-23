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

#include <nan.h>

#include "common.h"
#include "fp.h"
#include "msg.h"

using namespace v8;
using v8::FunctionTemplate;

typedef enum {
    OP_START,                   // start fingerprint acquisition
    OP_STOP,                    // stop fingerprint acquisition
    OP_NONE,                    // No operation
    OP_EXIT                     // Everything is done
} FP_WORK_OP;

typedef struct __FP_WORK_DATA__ {
    uv_thread_t thread;
    uv_rwlock_t lock;
    FP_WORK_OP state = OP_NONE;
    CMsgWin *msg = NULL;
    void* handle = NULL;
    Nan::Persistent<Function> callback;
} FP_WORK_DATA;

typedef enum {
    ACQ_VERIFY,                 // acquire for verification
    ACQ_ENROLL,                 // acquire for enrollment
    ACQ_STOP                    // stop operation
} FP_ACQUIRE_OP;

typedef struct __FP_ACQUIRE_DATA__ {
    FP_ACQUIRE_OP op;
    int result;
    IPictureDispPtr fp_image;
    IDPFPFeatureSetPtr fp_template;
    IDPFPSamplePtr fp_sample;
    bool enrolled;
    Nan::Persistent<Function> callback;
    Nan::Persistent<Function> close_callback;
} FP_ACQUIRE_DATA;

static int initialized = 0;
static int fp_work_started = 0;
static FP_WORK_DATA *fpdata = NULL;
static CFPHandler *fp = NULL;

void fp_handle_event(int status) {
    uv_async_t* async = static_cast<uv_async_t*>(fp->GetUserData());
    if (async) {
        FP_ACQUIRE_DATA *data = static_cast<FP_ACQUIRE_DATA*>(async->data);
        if (data) {
            if (status != CFPHandler::FP_STATUS_NONE) {
                data->result = status;
                if (status == CFPHandler::FP_STATUS_COMPLETE) {
                    data->fp_image = fp->GetFingerImage();
                    data->fp_template = fp->GetFingerTemplate();
                    data->fp_sample = fp->GetFingerSample();
                }
                uv_async_send(async);
            }
        }
    }
}

void fp_prep_acquisition() {
    if (!fp) {
        fp = new CFPHandler();
        fp->SetHandler(fp_handle_event);
    }
    if (!fpdata->msg) {
        fpdata->msg = new CMsgWin();
    }
}

void fp_work_handler(void *d) {
    FP_WORK_DATA *data = (FP_WORK_DATA*)d;
    if (!data) return;

    CoInitialize(NULL);

    fp_prep_acquisition();
    // notify work progress
    uv_async_t* async = static_cast<uv_async_t*>(data->handle);;
    if (async) {
        uv_async_send(async);
    }

    while (data->state != OP_EXIT) {
        if (data->state == OP_START) {
            uv_rwlock_wrlock(&data->lock);
            data->state = OP_NONE;
            uv_rwlock_wrunlock(&data->lock);

            ShowWindow(data->msg->GetHandle(), SW_SHOW);
            SetFocus(data->msg->GetHandle());
            fp->Start();
        }
        if (data->state == OP_STOP) {
            uv_rwlock_wrlock(&data->lock);
            data->state = OP_NONE;
            uv_rwlock_wrunlock(&data->lock);

            ShowWindow(data->msg->GetHandle(), SW_HIDE);
            fp->Stop();
            // notify acquire progress
            async = static_cast<uv_async_t*>(fp->GetUserData());
            if (async) {
                uv_async_send(async);
            }
        }
        data->msg->ProcessMessages();
    }

    CoUninitialize();

    delete data;
    data = NULL;
}

void work_close(uv_handle_t* handle)
{
    uv_async_t* async = reinterpret_cast<uv_async_t*>(handle);
    FP_WORK_DATA *data = static_cast<FP_WORK_DATA*>(async->data);

    data->callback.Reset();
    data->handle = NULL;

    delete async;
}

void work_progress(uv_async_t *handle) {
    FP_WORK_DATA *data = static_cast<FP_WORK_DATA*>(handle->data);
    Nan::HandleScope scope;
    if (!data)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Nan::AsyncResource asyncResource("workProgress");
    Local<Value> argv[1] = {Nan::Null()};

    callback.Call(1, argv, &asyncResource);

    uv_close(reinterpret_cast<uv_handle_t*>(handle), work_close);
}

void start_fp_work_thread(Local<Function> cb) {
    fpdata = new FP_WORK_DATA;
    fpdata->state = OP_NONE;
    fpdata->callback.Reset(cb);

    uv_async_t* async = new uv_async_t;
    uv_async_init(uv_default_loop(), async, work_progress);
    async->data = fpdata;
    fpdata->handle = async;

    uv_rwlock_init(&fpdata->lock);
    uv_thread_create(&fpdata->thread, fp_work_handler, (void*)fpdata);
}

void start_fp_work_handler(Local<Function> cb) {
    if (fp_work_started == 0) {
        start_fp_work_thread(cb);
        fp_work_started = 1;
    }
}

static const char *fp_result_name(int result)
{
    switch (result) {
    case CFPHandler::FP_STATUS_COMPLETE:
        return "complete";
    case CFPHandler::FP_STATUS_ERROR:
        return "error";
    case CFPHandler::FP_STATUS_CONNECTED:
        return "connected";
    case CFPHandler::FP_STATUS_DISCONNECTED:
        return "disconnected";
    default:
        return NULL;
    }
}

Local<Object> fp_sa2buff(SAFEARRAY *sa)
{
    char *pData = NULL;
    LONG lBound = 0;
    LONG uBound = 0;

    SafeArrayGetLBound(sa, 1, &lBound);
    SafeArrayGetUBound(sa, 1, &uBound);
    SafeArrayAccessData(sa, (LPVOID*)&pData);
    size_t sz = uBound - lBound + 1;
    Local<Object> buffer = Nan::CopyBuffer(pData, sz).ToLocalChecked();
    SafeArrayUnaccessData(sa);

    return buffer;
}

VARIANT fp_buff2sa(Local<Object> buff)
{
    VARIANT v = {};
    SAFEARRAY *sa = NULL;
    size_t sz = node::Buffer::Length(buff);
    char *pData = new char[sz];

    sa = SafeArrayCreateVector(VT_UI1, 0, sz - 1);
    SafeArrayAccessData(sa, (LPVOID*)&pData);
    CopyMemory(pData, node::Buffer::Data(buff), sz);
    SafeArrayUnaccessData(sa);
    v.vt = VT_UI1 | VT_ARRAY;
    v.parray = sa;

    return v;
}

void acquire_close(uv_handle_t* handle)
{
    fp->SetUserData(NULL);

    uv_async_t* async = reinterpret_cast<uv_async_t*>(handle);
    FP_ACQUIRE_DATA *data = static_cast<FP_ACQUIRE_DATA*>(async->data);

    if (!data->close_callback.IsEmpty()) {
        Nan::HandleScope scope;
        Nan::Callback callback(Nan::New<Function>(data->close_callback));
        Nan::AsyncResource asyncResource("acquireClose");
        Local<Value> argv[1] = {Nan::Null()};
        callback.Call(1, argv, &asyncResource);
    }

    delete async;
}

void acquire_progress(uv_async_t *handle) {
    FP_ACQUIRE_DATA *data = static_cast<FP_ACQUIRE_DATA*>(handle->data);
    Nan::HandleScope scope;

    if (!data || data->result == CFPHandler::FP_STATUS_NONE)
        return;

    Nan::Callback callback(Nan::New<Function>(data->callback));
    Nan::AsyncResource asyncResource("acquireProgress");
    Local<Value> argv[3];
    argv[0] = Nan::New(fp_result_name(data->result)).ToLocalChecked();
    argv[1] = Nan::Null();
    argv[2] = Nan::Null();

    if (data->result == CFPHandler::FP_STATUS_COMPLETE) {
        if (data->op != ACQ_STOP) {
            if (data->enrolled) {
                data->enrolled = false;
                argv[0] = Nan::New("enrolled").ToLocalChecked();
                IDPFPTemplatePtr enrollTmpl = fp->GetRegTemplate();
                argv[2] = fp_sa2buff(V_ARRAY(&enrollTmpl->Serialize()));
            } else {
                if (data->fp_image) {
                    // @todo: convert bitmap to data
                }
                if (data->op == ACQ_VERIFY) {
                    if (data->fp_template) {
                        argv[2] = fp_sa2buff(V_ARRAY(&data->fp_template->Serialize()));
                    }
                }
                if (data->op == ACQ_ENROLL) {
                    if (data->fp_sample) {
                        argv[2] = fp_sa2buff(V_ARRAY(&data->fp_sample->Serialize()));
                    }
                    if (data->fp_template) {
                        fp->AddTemplate(data->fp_template);
                    }
                }
            }
        }
    }

    callback.Call(3, argv, &asyncResource);

    switch (data->op) {
    case ACQ_ENROLL:
        if (fp->GenerateRegTemplate()) {
            data->enrolled = true;
            uv_async_send(handle);
        }
        break;

    case ACQ_STOP:
        uv_close(reinterpret_cast<uv_handle_t*>(handle), acquire_close);
        data->fp_image = NULL;
        data->fp_template = NULL;
        data->fp_sample = NULL;
        delete data;
        break;

    default:
        break;
    }
}

bool acquire_start(Local<Function> cb, bool enroll = false)
{
    bool ret = false;
    FP_ACQUIRE_DATA *data;
    if (fp && NULL == fp->GetUserData()) {
        data = new FP_ACQUIRE_DATA;
        data->op = enroll ? ACQ_ENROLL : ACQ_VERIFY;
        data->result = CFPHandler::FP_STATUS_NONE;
        data->fp_image = NULL;
        data->fp_template = NULL;
        data->fp_sample = NULL;
        data->enrolled = false;
        data->callback.Reset(cb);
        data->close_callback.Reset();

        uv_async_t* async = new uv_async_t;
        uv_async_init(uv_default_loop(), async, acquire_progress);
        async->data = data;

        fp->SetUserData(async);
        fp->SetOperationMode(enroll ? CFPHandler::FP_ACQUIRE_ENROLL :
            CFPHandler::FP_ACQUIRE_VERIFY);
        if (enroll) {
            fp->ResetTemplate();
        }

        uv_rwlock_wrlock(&fpdata->lock);
        fpdata->state = OP_START;
        uv_rwlock_wrunlock(&fpdata->lock);

        ret = true;
    }
    return ret;
}

bool acquire_stop(Local<Function> cb)
{
    bool ret = false;
    if (fp && NULL != fp->GetUserData()) {
        uv_async_t* async = static_cast<uv_async_t*>(fp->GetUserData());
        if (async) {
            FP_ACQUIRE_DATA *data = static_cast<FP_ACQUIRE_DATA*>(async->data);
            if (data) {
                data->op = ACQ_STOP;
                data->close_callback.Reset(cb);

                uv_rwlock_wrlock(&fpdata->lock);
                fpdata->state = OP_STOP;
                uv_rwlock_wrunlock(&fpdata->lock);

                ret = true;
            }
        }
    }
    return ret;
}

NAN_METHOD(init) {
    bool ret = false;
    if (initialized == 0 && info.Length() > 0 && info[0]->IsFunction()) {
        initialized = 1;
        start_fp_work_handler(Local<Function>::Cast(info[0]));

        ret = true;
    }
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_METHOD(exit) {
    bool ret = false;
    if (initialized == 1) {
        initialized = 0;
        if (fpdata) {
            uv_rwlock_wrlock(&fpdata->lock);
            fpdata->state = OP_EXIT;
            uv_rwlock_wrunlock(&fpdata->lock);

            uv_rwlock_destroy(&fpdata->lock);
        }
        delete fp;
        fp = NULL;

        ret = true;
    }
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_METHOD(getFeaturesLen) {
    if (initialized == 0) {
        Nan::ThrowError("Not initialized!");
    }
    info.GetReturnValue().Set(fp->GetFeaturesLen());
}

NAN_METHOD(startAcquire) {
    bool ret = false;
    bool enroll = false;
    Local<Function> cb;

    if (info.Length() < 1 || initialized == 0 || !fp->Initialize()) {
        goto error;
    }
    // startAcquire(() => {});
    if (info[0]->IsFunction()) {
        cb = Local<Function>::Cast(info[0]);
    } else if (info.Length() > 1) {
        // startAcquire(true, () => {});
        // startAcquire({enroll: true}, () => {});
        if (!info[1]->IsFunction()) {
            goto error;
        }
        Local<Context> context = Nan::GetCurrentContext();
        Isolate* isolate = context->GetIsolate();
        cb = Local<Function>::Cast(info[1]);
        if (info[0]->IsBoolean()) {
            enroll = info[0]->BooleanValue(isolate);
        } else if (info[0]->IsObject()) {
            Local<Object> options = info[0]->ToObject(context).ToLocalChecked();
            Local<String> enrollProp = Nan::New("enroll").ToLocalChecked();
            if (Nan::HasOwnProperty(options, enrollProp).FromJust()) {
                enroll = Nan::Get(options, enrollProp).ToLocalChecked()->BooleanValue(isolate);
            }
        } else {
            goto error;
        }
    } else {
        goto error;
    }

    ret = acquire_start(cb, enroll);

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_METHOD(stopAcquire) {
    bool ret = false;

    if (info.Length() < 1 || !info[0]->IsFunction() || initialized == 0 || !fp->Initialize()) {
        goto error;
    }
    ret = acquire_stop(Local<Function>::Cast(info[0]));

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_METHOD(verify) {
    bool ret = false;
    Local<Context> context = Nan::GetCurrentContext();
    IDPFPTemplatePtr tmpl = NULL;
    variant_t vTmpl = NULL;

    if (info.Length() < 2 || initialized == 0 || !fp->Initialize()) {
        goto error;
    }
    if (!info[0]->IsObject() || !info[1]->IsFunction()) {
        goto error;
    }

    tmpl.CreateInstance(__uuidof(DPFPTemplate));
    vTmpl = _variant_t(fp_buff2sa(info[0]->ToObject(context).ToLocalChecked()));
    try {
        if (SUCCEEDED(tmpl->Deserialize(vTmpl))) {
            int res = fp->Verify(tmpl);
            if (res >= CFPHandler::FP_VERIFY_NO_MATCH) {
                Nan::HandleScope scope;
                Nan::Persistent<Function> cb;
                cb.Reset(Local<Function>::Cast(info[1]));
                Nan::Callback callback(Nan::New<Function>(cb));
                Nan::AsyncResource asyncResource("verify");
                Local<Value> argv[1] = {Nan::New(res == CFPHandler::FP_VERIFY_MATCHED ? true : false)};
                callback.Call(1, argv, &asyncResource);

                ret = true;
            }
        }
    }
    catch (...) {
        // nothing
    }
    tmpl = NULL;

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_METHOD(isAcquiring) {
    bool ret = false;

    if (initialized == 0 || !fp->Initialize()) {
        goto error;
    }
    ret = fp->IsAcquiring();

error:
    info.GetReturnValue().Set(Nan::New(ret));
}

NAN_MODULE_INIT(module_init) {
    Nan::HandleScope scope;
    Nan::SetMethod(target, "init", init);
    Nan::SetMethod(target, "exit", exit);
    Nan::SetMethod(target, "getFeaturesLen", getFeaturesLen);
    Nan::SetMethod(target, "startAcquire", startAcquire);
    Nan::SetMethod(target, "stopAcquire", stopAcquire);
    Nan::SetMethod(target, "verify", verify);
    Nan::SetMethod(target, "isAcquiring", isAcquiring);
}

NODE_MODULE(dpax, module_init)

//----------------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ulReason, LPVOID lpReserved)
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