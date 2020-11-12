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

#include "fp.h"

DPFPReader::DPFPReader():
#ifdef _WIN32
    mHandle(NULL),
#endif
    mStates(STATE_OP_IDLE),
    mMode(MODE_CAPTURE),
    mDev(NULL),
    mReaderIdx(-1),
    mFeature(NULL),
    mFeatureSize(0),
    mFmd(NULL),
    mFmdSize(0),
    mCaptureFormat(DPFJ_FMD_DP_VER_FEATURES),
    mEnrollFormat(DPFJ_FMD_DP_REG_FEATURES),
    mFalseMatchRate(DPFJ_PROBABILITY_ONE / 100000),
    mContext(NULL),
    mReaderHandler(NULL),
    mCaptureHandler(NULL),
    mEnrollHandler(NULL)
{
    initialize();
    prepTicks();
}

DPFPReader::~DPFPReader()
{
    if (mInitialized) {
        finalize();
    }
}

void DPFPReader::initialize()
{
    mInitialized = DPFPDD_SUCCESS == dpfpdd_init();
    if (mInitialized) {
#ifdef _WIN32
        CoInitializeEx(NULL, 0);
#endif
    }
}

void DPFPReader::finalize()
{
    if (mInitialized) {
#ifdef _WIN32
        CoUninitialize();
#endif
        dpfpdd_exit();
    }
}

void DPFPReader::queryReader()
{
    if (isNextTick(TICK_READER)) {
        if (!inState(STATE_QUERY)) {
            DPFPDD_DEV_INFO* readers = NULL;
            unsigned int cnt = 0;
            while (DPFPDD_E_MORE_DATA == dpfpdd_query_devices(&cnt, readers)) {
                if (NULL != readers)
                    delete [] readers;
                readers = new DPFPDD_DEV_INFO[cnt];
                if (NULL == readers) {
                    cnt = 0;
                    break;
                } else {
                    readers[0].size = sizeof(DPFPDD_DEV_INFO);
                }
            }
            mReaders.clear();
            if (readers != NULL) {
                for (unsigned int i = 0; i < cnt; i++) {
                    string name = readers[i].name;
                    mReaders.push_back(name);
                }
            }
            notifyReader();
            if (mReaderIdx < 0 && !mReaders.empty()) {
                mReaderIdx = 0;
            }
            unsetState(STATE_QUERY);
        }
    }
}

void DPFPReader::queryStatus()
{
    if (isNextTick(TICK_STATUS)) {
        if (!inState(STATE_STATUS)) {
            if (mDev != NULL) {
                DPFPDD_DEV_STATUS ds = {0};
                ds.size = sizeof(ds);
                if (DPFPDD_E_INVALID_DEVICE == dpfpdd_get_device_status(mDev, &ds)) {
                    mDev = NULL;
                    mReaderIdx = -1;
                    mReaders.clear();
                    notifyReader();
                }
            }
            unsetState(STATE_STATUS);
        }
    }
}

void DPFPReader::openReader()
{
    if (mDev == NULL && mReaderIdx >= 0 && mReaderIdx < mReaders.size()) {
        int ret = dpfpdd_open_ext((char*)mReaders[mReaderIdx].c_str(), DPFPDD_PRIORITY_COOPERATIVE, &mDev);
        if (DPFPDD_SUCCESS == ret) {
            fp_dbg("Reader sucessfully opened\n");
        } else {
            fp_dbg("Unable to open reader, error: %d\n", ret);
        }
    }
}

void DPFPReader::closeReader()
{
    if (mDev != NULL) {
        fp_dbg("Closing reader\n");
        dpfpdd_close(mDev);
        mDev = NULL;
    }
}

bool DPFPReader::prepareCapture()
{
    if (mDev != NULL) {
        if (!inState(STATE_PREP_CAPTURE)) {
            DPFPDD_CAPTURE_PARAM cp = {0};
            cp.size = sizeof(cp);
            cp.image_fmt = DPFPDD_IMG_FMT_ISOIEC19794;
            cp.image_proc = DPFPDD_IMG_PROC_NONE;
            cp.image_res = 500;
            int ret = dpfpdd_capture_async(mDev, &cp, this, captureCallback);
            if (DPFPDD_SUCCESS == ret) {
                fp_dbg("Capture started\n");
            } else {
                fp_dbg("Unable to start capture, error: %d\n", ret);
            }
            setTickRes(TICK_CAPTURE, DPFPDD_SUCCESS == ret ? 1 : 0);
        } else {
            if (isNextTick(TICK_CAPTURE)) {
                fp_dbg("Capture already prepared\n");
            }
        }
        return getTickRes(TICK_CAPTURE) == 1 ? true : false;
    }
    return false;
}

bool DPFPReader::cancelCapture()
{
    if (mDev != NULL) {
        if (!inState(STATE_CANCEL)) {
            fp_dbg("Cancel capture\n");
            int ret = dpfpdd_cancel(mDev);
            setTickRes(TICK_CANCEL, DPFPDD_SUCCESS == ret ? 1 : 0);
        } else {
            if (isNextTick(TICK_CANCEL)) {
                fp_dbg("Capture already cancelled\n");
            }
        }
        return getTickRes(TICK_CANCEL) == 1 ? true : false;
    }
    return false;
}

bool DPFPReader::prepareEnroll()
{
    if (mDev != NULL) {
        if (mMode == MODE_ENROLL) {
            if (!inState(STATE_PREP_ENROLL)) {
                dpfj_finish_enrollment();
                int ret = dpfj_start_enrollment(mEnrollFormat);
                if (DPFPDD_SUCCESS == ret) {
                    fp_dbg("Enrollment started\n");
                } else {
                    fp_dbg("Unable to start enroll, error: %d\n", ret);
                }
                setTickRes(TICK_ENROLL, DPFPDD_SUCCESS == ret ? 1 : 0);
            } else {
                if (isNextTick(TICK_ENROLL)) {
                    fp_dbg("Enroll already prepared\n");
                }
            }
            return getTickRes(TICK_ENROLL) == 1 ? true : false;
        }
        return true;
    }
    return false;
}

void DPFPReader::finishEnroll()
{
    unsigned char* buff = NULL;
    int ret;
    mFmdSize = 0;
    while (true) {
        if (buff != NULL) {
            ret = dpfj_create_enrollment_fmd(buff, &mFmdSize);
        } else {
            ret = dpfj_create_enrollment_fmd(NULL, &mFmdSize);
        }
        if (DPFPDD_E_MORE_DATA == ret) {
            buff = new unsigned char[mFmdSize];
        } else {
            if (DPFPDD_SUCCESS == ret) {
                fp_dbg("FMD created successfully, size: %d\n", mFmdSize);
                dpfj_finish_enrollment();
                mFmd = buff;
                notifyEnroll();
            } else {
                if (buff != NULL)
                    delete [] buff;
                fp_dbg("Unable to create FMD, error: %d\n", ret);
            }
            break;
        }
    }
}

void DPFPReader::notifyReader()
{
    if (mReaderHandler != NULL) {
        mReaderHandler(mContext);
    }
}

void DPFPReader::notifyCapture()
{
    if (mCaptureHandler != NULL) {
        mCaptureHandler(mContext);
    }
}

void DPFPReader::notifyEnroll()
{
    if (mEnrollHandler != NULL) {
        mEnrollHandler(mContext);
    }
}

unsigned char* DPFPReader::extractFeature(DPFPDD_CAPTURE_CALLBACK_DATA_0* data,
    DPFJ_FMD_FORMAT fmt, unsigned int* sz)
{
    if (DPFPDD_SUCCESS == data->error) {
        if (data->capture_result.size > 0) {
            *sz = MAX_FMD_SIZE;
            unsigned char* buff = new unsigned char[*sz];
            if (buff != NULL) {
                int ret = dpfj_create_fmd_from_fid(data->capture_parm.image_fmt,
                    data->image_data, data->image_size, fmt, buff, sz);
                if (DPFPDD_SUCCESS == ret) {
                    fp_dbg("Feature extraction completed, size: %d\n", *sz);
                    return buff;
                } else {
                    fp_dbg("Unable to extract feature, error: %d\n", ret);
                }
            }
        } else {
            if (data->capture_result.quality == DPFPDD_QUALITY_CANCELED) {
                fp_dbg("Capture cancelled\n");
            } else {
                fp_dbg("Bad capture quality: %d\n", data->capture_result.quality);
            }
        }
    }
    return NULL;
}

void DPFPReader::handleCapture(DPFPDD_CAPTURE_CALLBACK_DATA_0* data)
{
    fp_dbg("Handle capture\n");
    if (mFeature != NULL)
        delete [] mFeature;
    mFeature = extractFeature(data, mCaptureFormat, &mFeatureSize);
    if (mFeature != NULL) {
        stopCapture(false);
        notifyCapture();
    }
}

void DPFPReader::handleEnroll(DPFPDD_CAPTURE_CALLBACK_DATA_0* data)
{
    fp_dbg("Handle enroll\n");
    DPFJ_FMD_FORMAT fmt = mEnrollFormat == DPFJ_FMD_DP_REG_FEATURES ?
        DPFJ_FMD_DP_PRE_REG_FEATURES : mEnrollFormat;
    mFeature = extractFeature(data, fmt, &mFeatureSize);
    if (mFeature != NULL) {
        notifyCapture();
        fp_dbg("Adding enrollment, feature size: %d\n", mFeatureSize);
        int ret = dpfj_add_to_enrollment(fmt, mFeature, mFeatureSize, 0);
        switch (ret)
        {
            case DPFPDD_SUCCESS:
                fp_dbg("Enrollment finished\n");
                stopCapture(false);
                finishEnroll();
                break;
            case DPFPDD_E_MORE_DATA:
                fp_dbg("Enroll the same finger again\n");
                break;
            default:
                fp_dbg("Enrollment error: %d\n", ret);
                break;
        }
    }
}

void DPFPReader::handleCallback(DPFPDD_CAPTURE_CALLBACK_DATA_0* data)
{
    if (mMode == MODE_ENROLL) {
        handleEnroll(data);
    } else {
        handleCapture(data);
    }
}

bool DPFPReader::inState(int state)
{
    if ((mStates & state) == state) {
        return true;
    }
    setState(state);
    return false;
}

void DPFPReader::setState(int state)
{
    mStates = mStates | state;
}

void DPFPReader::unsetState(int state)
{
    mStates = mStates & ~state;
}

void DPFPReader::prepTicks()
{
    mTicks = new DPFP_TICK[TICK_LAST];
    for (unsigned int i = 0; i < TICK_LAST; i++) {
        mTicks[i] = {0, 5000, 0};
    }
    mTicks[TICK_READER].interval = 1000;
}

bool DPFPReader::isNextTick(unsigned int tick)
{
    if ((mTicks[tick].last + mTicks[tick].interval) <= CurrentTime_milliseconds()) {
        mTicks[tick].last = CurrentTime_milliseconds();
        return true;
    }
    return false;
}

int DPFPReader::getTickRes(unsigned int tick)
{
    return mTicks[tick].result;
}

void DPFPReader::setTickRes(unsigned int tick, int res)
{
    mTicks[tick].result = res;
}

#ifdef _WIN32
void DPFPReader::setHandle(HWND handle)
{
    mHandle = handle;
}

HWND DPFPReader::getHandle()
{
    return mHandle;
}
#endif

unsigned int DPFPReader::getFeaturesLen()
{
    return DPFP_FEATURES_LEN;
}

unsigned int DPFPReader::getIdentificationLen()
{
    return DPFP_IDENTIFICATION_LEN;
}

vector<string> DPFPReader::getReaders(bool query)
{
    if (query) {
        queryReader();
    }
    return mReaders;
}

bool  DPFPReader::setReader(char* reader)
{
    int idx = -1;
    for (vector<string>::iterator it = mReaders.begin(); it < mReaders.end(); ++it) {
        string name = *it;
        idx++;
        if (name == reader) {
            mReaderIdx = idx;
            return true;
        }
    }
    return false;
}

int DPFPReader::getReaderIdx()
{
    return mReaderIdx;
}

bool DPFPReader::setReaderIdx(int reader)
{
    if (reader >= 0 && reader < mReaders.size()) {
        mReaderIdx = reader;
        return true;
    }
    return false;
}

int DPFPReader::getMode()
{
    return mMode;
}

void DPFPReader::setMode(int mode)
{
    mMode = mode;
}

bool DPFPReader::getState(int state)
{
    return (mStates & state) == state ? true : false;
}

void DPFPReader::setContext(void* ctx)
{
    mContext = ctx;
}

void DPFPReader::setReaderHandler(DPFP_READER_HANDLER handler)
{
    mReaderHandler = handler;
}

void DPFPReader::setCaptureHandler(DPFP_CAPTURE_HANDLER handler)
{
    mCaptureHandler = handler;
}

void DPFPReader::setEnrollHandler(DPFP_ENROLL_HANDLER handler)
{
    mEnrollHandler = handler;
}

unsigned char* DPFPReader::getFeature() const
{
    return mFeature;
}

unsigned int DPFPReader::getFeatureSize()
{
    return mFeatureSize;
}

unsigned char* DPFPReader::getFmd() const
{
    return mFmd;
}

unsigned int DPFPReader::getFmdSize()
{
    return mFmdSize;
}

void DPFPReader::doCapture()
{
    // ensure reader is present
    if (mReaderIdx < 0) {
        queryReader();
    }
    // ensure device is opened
    if (mReaderIdx >= 0 && mDev == NULL) {
        openReader();
    }
    // if device opened, check its status
    if (mDev != NULL) {
        queryStatus();
    }
    // notify reader presence
    if (!inState(STATE_REPORTED)) {
        notifyReader();
    }
    // if enroll, prepare enroll
    if (!prepareEnroll())
        return;
    // last prepare capture
    if (!prepareCapture())
        return;
}

void DPFPReader::doIdle()
{
    // cancel capture
    if (!cancelCapture())
        return;
    // close reader
    closeReader();
}

void DPFPReader::startCapture()
{
    if (mMode != MODE_NONE) {
        if (!inState(STATE_OP_CAPTURE)) {
            fp_dbg("Starting capture\n");
            unsetState(STATE_OP_IDLE);
            unsetState(STATE_CANCEL);
        }
    }
}

void DPFPReader::stopCapture(bool force)
{
    if (mMode != MODE_NONE) {
        if (!inState(STATE_OP_IDLE)) {
            fp_dbg("Stopping capture\n");
            unsetState(STATE_OP_CAPTURE);
            unsetState(STATE_REPORTED);
            unsetState(STATE_PREP_ENROLL);
            if (mMode != MODE_CONTINOUS_CAPTURE || force) {
                unsetState(STATE_PREP_CAPTURE);
            }
            if (mMode == MODE_CONTINOUS_CAPTURE && !force) {
                fp_dbg("Continue capture\n");
                startCapture();
            } else {
                mMode = MODE_NONE;
            }
        }
    }
}

bool DPFPReader::compare(unsigned char* feature, unsigned int featurelen,
    unsigned char* fmd, unsigned int fmdlen, DPFJ_FMD_FORMAT fmt)
{
    if (feature != NULL && fmd != NULL) {
        unsigned int fmr = 0;
        int ret = dpfj_compare(mCaptureFormat, feature, featurelen, 0, 
            fmt, fmd, fmdlen, 0, &fmr);
        if (DPFPDD_SUCCESS == ret) {
            if (fmr < mFalseMatchRate) {
                fp_dbg("Fingerprint matched, score: %d\n", fmr);
                return true;
            } else {
                fp_dbg("Fingerprint did not matched\n");
            }
        }
    }
    return false;
}

int DPFPReader::identify(unsigned char* feature, unsigned int featurelen,
    unsigned char** fmds, unsigned int* fmdlens, unsigned int fmdsz, DPFJ_FMD_FORMAT fmt)
{
    if (feature != NULL && fmds != NULL) {
        unsigned int sz = fmdsz;
        DPFJ_CANDIDATE* candidates = new DPFJ_CANDIDATE[fmdsz];
        unsigned int fmr = mFalseMatchRate;
        int ret = dpfj_identify(mCaptureFormat, feature, featurelen, 0, 
            fmt, fmdsz, fmds, fmdlens, fmr, &sz, candidates);
        if (DPFPDD_SUCCESS == ret) {
            if (sz > 0) {
                if (compare(feature, featurelen, fmds[candidates[0].fmd_idx],
                    fmdlens[candidates[0].fmd_idx], fmt)) {
                    fp_dbg("Identify matched index %d\n", candidates[0].fmd_idx);
                    return candidates[0].fmd_idx;
                } else {
                    fp_dbg("Identification did not match\n");
                }
            } else {
                fp_dbg("No matched candidate\n");
            }
        } else {
            fp_dbg("Unable to identify, error: %d\n", ret);
        }
    }
    return -1;
}

void DPAPICALL DPFPReader::captureCallback(void* ctx, unsigned int reserved,
    unsigned int sz, void* data)
{
    if (ctx == NULL || data == NULL) return;
    DPFPReader* reader = reinterpret_cast<DPFPReader*>(ctx);
#ifdef _WIN32
    PostMessage(reader->getHandle(), DPFPReader::WM_FP_MESSAGE, reinterpret_cast<WPARAM>(ctx),
        reinterpret_cast<LPARAM>(data));
#endif
#ifdef __linux__
    DPFPDD_CAPTURE_CALLBACK_DATA_0* cb = reinterpret_cast<DPFPDD_CAPTURE_CALLBACK_DATA_0*>(data);
    reader->handleCallback(cb);
#endif
}

#ifdef _WIN32
bool fp_handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (DPFPReader::WM_FP_MESSAGE == uMsg) {
        DPFPReader* reader = reinterpret_cast<DPFPReader*>(wParam);
        DPFPDD_CAPTURE_CALLBACK_DATA_0* data = reinterpret_cast<DPFPDD_CAPTURE_CALLBACK_DATA_0*>(lParam);
        reader->handleCallback(data);
        return true;
    }
    return false;
}
#endif