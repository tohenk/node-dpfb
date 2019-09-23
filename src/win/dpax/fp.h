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

#if !defined(FP_HANDLER_INCLUDED_)
#define FP_HANDLER_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CFPHandler

class CFPHandler;

// ***** Declare an event handling class using the TEventHandler template. *****
typedef TEventHandler<CFPHandler, IDPFPCapture, _IDPFPCaptureEvents>
    IDPFPCaptureEventHandler;
typedef void (*fp_message_handler)(int);

class CFPHandler {
public:
    enum {
        FP_COMPLETE = 0x1,
        FP_READER_DISCONNECT,
        FP_READER_CONNECTED,
        FP_FINGER_TOUCH,
        FP_FINGER_GONE,
        FP_SAMPLE_QUALITY
    };
    enum {
        FP_STATUS_NONE,
        FP_STATUS_COMPLETE,
        FP_STATUS_ERROR,
        FP_STATUS_DISCONNECTED,
        FP_STATUS_CONNECTED
    };
    enum {
        FP_ACQUIRE_VERIFY,
        FP_ACQUIRE_ENROLL
    };
    enum {
        FP_VERIFY_ERR,
        FP_VERIFY_NO_TEMPLATE,
        FP_VERIFY_NO_MATCH,
        FP_VERIFY_MATCHED
    };
    CFPHandler();
    ~CFPHandler();

protected:
    // Is operation started
    bool                        m_Started;
    // Message handler
    fp_message_handler          m_Handler;
    // Acquire operation mode, can be FP_ACQUIRE_VERIFY or FP_ACQUIRE_ENROLL.
    int                         m_OperationMode;
    // Arbitrary user data
    void*                       m_UserData;
    // Instance of a IDPFPCapture smart pointer.
    IDPFPCapturePtr             m_spCapture;
    // Pointer to a TEventHandler class which is specially tailored
    // to receiving events from the _IDPFPCaptureEvents events of an
    // IDPFPCapture object.
    IDPFPCaptureEventHandler*   m_pEvent;
    // Instance of a IDPFPEnrollment smart pointer.
    IDPFPEnrollmentPtr          m_spEnroll;
    // Instance of a IDPFPSampleConversion smart pointer.
    IDPFPVerificationPtr        m_spVerify;
    // Instance of a IDPFPSampleConversion smart pointer.
    IDPFPSampleConversionPtr    m_spConversion;
    // Instance of a IDPFPFeatureExtraction smart pointer.
    IDPFPFeatureExtractionPtr   m_spFeature;
    // Captured finger image
    IPictureDispPtr             m_FingerImage;
    // Captured finger template
    IDPFPFeatureSetPtr          m_FingerTemplate;
    // Captured finger sample
    IDPFPSamplePtr              m_FingerSample;

public:
    inline bool Initialize() {
        return true;
    }
    inline void SetHandler(fp_message_handler handler) {
        m_Handler = handler;
    }
    inline int GetOperationMode() {
        return m_OperationMode;
    }
    inline void SetOperationMode(int op) {
        m_OperationMode = op;
    }
    inline int GetFeaturesLen() {
        return m_spEnroll->FeaturesNeeded;
    }
    inline bool IsAcquiring() {
        return m_Started;
    }
    inline IPictureDispPtr GetFingerImage() {
        return m_FingerImage;
    }
    inline void SetFingerTemplate(IDPFPFeatureSetPtr tmpl) {
        m_FingerTemplate = tmpl;
    }
    inline IDPFPFeatureSetPtr GetFingerTemplate() {
        return m_FingerTemplate;
    }
    inline IDPFPSamplePtr GetFingerSample() {
        return m_FingerSample;
    }
    inline void SetUserData(void* userdata) {
        m_UserData = userdata;
    }
    inline void* GetUserData() {
        return m_UserData;
    }
    bool CreateEventHandler();
    bool Start();
    bool Stop();
    int Verify(IDPFPTemplatePtr data);
    int Identify(IDPFPFeatureSetPtr tmpl, IDPFPTemplatePtr data);
    void ResetTemplate();
    void AddTemplate(IDPFPFeatureSetPtr tmpl);
    bool GenerateRegTemplate();
    IDPFPTemplatePtr GetRegTemplate();
    HRESULT OnInvoke(IDPFPCaptureEventHandler* pEventHandler, DISPID dispidMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams,
        VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(FP_HANDLER_INCLUDED_)
