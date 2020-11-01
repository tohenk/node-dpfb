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

#include "worker.h"

static void fp_prep_message_receiver(void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->msg == NULL) {
        api_data->msg = new MsgWin();
        api_data->msg->SetHandler(fp_handle_message);
        api_data->reader->setHandle(api_data->msg->GetHandle());
    }
}

static void fp_focus_window(void* data, bool focus) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    if (api_data->msg != NULL) {
        if (focus) {
            if (!IsWindowVisible(api_data->msg->GetHandle())) {
                ShowWindow(api_data->msg->GetHandle(), SW_SHOW);
                if (IsWindowVisible(api_data->msg->GetHandle())) {
                    SetFocus(api_data->msg->GetHandle());
                }
            }
        } else {
            if (IsWindowVisible(api_data->msg->GetHandle())) {
                ShowWindow(api_data->msg->GetHandle(), SW_HIDE);
            }
        }
    }
}

static void fp_do_work(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    fp_prep_message_receiver(data);
    bool capturing;
    while (true) {
        if (api_data->reader == NULL || api_data->exit) {
            break;
        }
        capturing = api_data->reader->getState(DPFPReader::STATE_OP_CAPTURE);
        if (capturing) {
            fp_focus_window(data, true);
            api_data->reader->doCapture();
        } else {
            api_data->reader->doIdle();
            fp_focus_window(data, false);
        }
        if (api_data->msg != NULL) {
            api_data->msg->ProcessMessages();
        }
        Sleep(10);
    }
}

static void fp_work_done(napi_env env, napi_status status, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;

    assert(napi_ok == napi_delete_async_work(env, api_data->worker.work));
    api_data->worker.work = NULL;
}

bool start_fp_worker(napi_env env, void* data) {
    FP_API_DATA* api_data = (FP_API_DATA*) data;
    napi_value work_name;

    if (api_data->worker.work == NULL) {
        assert(napi_ok == napi_create_string_utf8(env, "DPFP N-API Monitor Work",
            NAPI_AUTO_LENGTH, &work_name));
        assert(napi_ok == napi_create_async_work(env, NULL, work_name, fp_do_work, fp_work_done,
            api_data, &(api_data->worker.work)));
        assert(napi_ok == napi_queue_async_work(env, api_data->worker.work));
        return true;
    }
    return false;
}
