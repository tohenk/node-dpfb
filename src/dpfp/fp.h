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

#if !defined(DPFP_FP_H_INCLUDED)
#define DPFP_FP_H_INCLUDED

//#define DPFP_DBG

#include <chrono>
#ifdef DPFP_DBG
#include <iostream>
#endif
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

#include <dpfj.h>
#include <dpfpdd.h>

#define DPFP_FEATURES_LEN 4;
#define DPFP_IDENTIFICATION_LEN 16;

#ifdef DPFP_DBG
#define fp_dbg(...) printf(__VA_ARGS__)
#else
#define fp_dbg(...)
#endif

using namespace std;

#ifdef _WIN32
bool fp_handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

typedef struct {
    uint64_t last;
    unsigned int interval;
    int result;
} DPFP_TICK;

typedef void (*DPFP_READER_HANDLER)(void*);
typedef void (*DPFP_CAPTURE_HANDLER)(void*);
typedef void (*DPFP_ENROLL_HANDLER)(void*);

class DPFPReader {
public:    
#ifdef _WIN32
    enum {
        WM_FP_MESSAGE = WM_USER + 1403,
    };
#endif
    enum {
        TICK_READER,
        TICK_STATUS,
        TICK_ENROLL,
        TICK_CAPTURE,
        TICK_CANCEL,
        TICK_LAST = TICK_CANCEL + 1,
    };
    enum {
        MODE_NONE,
        MODE_CAPTURE,
        MODE_CONTINOUS_CAPTURE,
        MODE_ENROLL,
    };
    enum {
        STATE_QUERY         = 0x0001,
        STATE_STATUS        = 0x0002,
        STATE_REPORTED      = 0x0004,
        STATE_CANCEL        = 0x0008,
        STATE_PREP_CAPTURE  = 0x0010,
        STATE_PREP_ENROLL   = 0x0020,
        STATE_OP_IDLE       = 0x0040,
        STATE_OP_CAPTURE    = 0x0080,
    };

private:
#ifdef _WIN32
    HWND mHandle;
#endif
    DPFPDD_DEV mDev;
    vector<string> mReaders;
    int mReaderIdx;
    bool mInitialized;
    int mMode;
    int mStates;
    DPFP_TICK* mTicks;
    unsigned char* mFeature;
    unsigned int mFeatureSize;
    unsigned char* mFmd;
    unsigned int mFmdSize;
    DPFJ_FMD_FORMAT mCaptureFormat;
    DPFJ_FMD_FORMAT mEnrollFormat;
    unsigned int mFalseMatchRate;
    void* mContext;
    DPFP_READER_HANDLER mReaderHandler;
    DPFP_CAPTURE_HANDLER mCaptureHandler;
    DPFP_ENROLL_HANDLER mEnrollHandler;
    void initialize();
    void finalize();
    void queryReader();
    void queryStatus();
    void openReader();
    void closeReader();
    bool prepareCapture();
    bool cancelCapture();
    bool prepareEnroll();
    void finishEnroll();
    void notifyReader();
    void notifyCapture();
    void notifyEnroll();
    unsigned char* extractFeature(DPFPDD_CAPTURE_CALLBACK_DATA_0* data,
        DPFJ_FMD_FORMAT fmt, unsigned int* sz);
    void handleCapture(DPFPDD_CAPTURE_CALLBACK_DATA_0* data);
    void handleEnroll(DPFPDD_CAPTURE_CALLBACK_DATA_0* data);
    static void DPAPICALL captureCallback(void* ctx, unsigned int reserved,
        unsigned int sz, void* data);

private:
    bool inState(int state);
    void setState(int state);
    void unsetState(int state);
    void prepTicks();
    bool isNextTick(unsigned int tick);
    int getTickRes(unsigned int tick);
    void setTickRes(unsigned int tick, int res);

public:
    void handleCallback(DPFPDD_CAPTURE_CALLBACK_DATA_0* data);

public:
    DPFPReader();
    ~DPFPReader();
#ifdef _WIN32
    void setHandle(HWND handle);
    HWND getHandle();
#endif
    unsigned int getFeaturesLen();
    unsigned int getIdentificationLen();
    vector<string> getReaders();
    bool setReader(char* reader);
    int getReaderIdx();
    bool setReaderIdx(int reader);
    int getMode();
    void setMode(int mode);
    bool getState(int state);
    void setContext(void* ctx);
    void setReaderHandler(DPFP_READER_HANDLER handler);
    void setCaptureHandler(DPFP_CAPTURE_HANDLER handler);
    void setEnrollHandler(DPFP_ENROLL_HANDLER handler);
    unsigned char* getFeature() const;
    unsigned int getFeatureSize();
    unsigned char* getFmd() const;
    unsigned int getFmdSize();
    void doCapture();
    void doIdle();
    void startCapture();
    void stopCapture(bool force = true);
    bool compare(unsigned char* feature, unsigned int featurelen,
        unsigned char* fmd, unsigned int fmdlen, DPFJ_FMD_FORMAT fmt = DPFJ_FMD_DP_REG_FEATURES);
    int identify(unsigned char* feature, unsigned int featurelen,
        unsigned char** fmds, unsigned int* fmdlens, unsigned int fmdsz, DPFJ_FMD_FORMAT fmt = DPFJ_FMD_DP_REG_FEATURES);
};

// https://codereview.stackexchange.com/questions/132852/easy-to-use-c-class-for-asking-current-time-stamp-in-milli-micro-and-nanose

inline uint64_t CurrentTime_milliseconds() 
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline uint64_t CurrentTime_microseconds() 
{
    return std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline uint64_t CurrentTime_nanoseconds()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#endif // !defined(DPFP_FP_H_INCLUDED)
