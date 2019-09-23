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


DPFP_STDAPI DPFPConvertSampleToBitmap(const DATA_BLOB* pSample, PBYTE pBitmap, size_t* pSize);

/////////////////////////////////////////////////////////////////////////////
// CFPHandler

class CFPHandler {
public:
    enum {
        WMUS_FP_NOTIFY = WM_USER + 1
    };
    enum {
        FP_INIT_OK,
        FP_INIT_ERR_FX,
        FP_INIT_ERR_MX,
        FP_INIT_ERR_SETTINGS,
        FP_INIT_ERR_OUT_OF_MEMORY
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
    HWND            m_hWnd;                     // Window handle used for Acquisition
    bool            m_Initialized;              // Handler initialization state
    bool            m_ReaderConnected;          // Fingerprint reader connected state
    int             m_Res;                      // Contains last initialization result
    FT_HANDLE       m_fxContext;                // Context for Feature Extraction functions, must be created before calling any of the FT_ functions
    FT_HANDLE       m_mcContext;                // Context for Matching functions, must be created before calling any of the MC_ functions
    HDPOPERATION    m_hOperation;               // Handle of the Operation, must be closed at exit.
    int             m_OperationMode;            // Acquire operation mode, can be FP_ACQUIRE_VERIFY or FP_ACQUIRE_ENROLL.
    int             m_NumberOfPreRegFeatures;   // Number of the pre-Enrollment fingerprint templates needed to create one Enrollment template.
    FT_BYTE**       m_TemplateArray;            // Array that keeps pre-Enrollment fingerprint templates. It is allocated with the size equal to number of pre-Enrollment templetes to create a Enrollment template (usually 4)
    int             m_nRegFingerprint;          // Pre-Enrollment number, index in the above array.
    FT_REG_OPTIONS  m_mcRegOptions;             // Enrollment options. Collected from check boxes of the dialog.
    DATA_BLOB       m_RegTemplate;              // BLOB that keeps Enrollment Template.
    PBITMAPINFO     m_FingerImage;              // Captured finger image
    size_t          m_FingerImageSize;          // Captured finger image size
    FT_BYTE*        m_FingerTemplate;           // Captured finger template
    int             m_FingerTemplateSize;       // Captured finger template size
    FT_BOOL         m_Learning;                 // Set learning mode when verifying
    void*           m_UserData;                 // Arbitrary user data

    int DoInit();
    void ProcessImageBlob(const DATA_BLOB* data);
    void ProcessImage(const DATA_BLOB* data);
    void ProcessTemplate(const DATA_BLOB* data);

public:
    bool Initialize();
    void Finalize();
    inline void SetHandle(HWND hWnd) {
        m_hWnd = hWnd;
    }
    inline int GetOperationMode() {
        return m_OperationMode;
    }
    inline void SetOperationMode(int op) {
        m_OperationMode = op;
    }
    inline int GetFeaturesLen() {
        return m_NumberOfPreRegFeatures;
    }
    inline int GetStatus() {
        return m_Res;
    }
    inline bool IsMessage(UINT msg) {
        return msg == WMUS_FP_NOTIFY;
    }
    inline bool IsReaderConnected() {
        return m_ReaderConnected;
    }
    inline bool IsAcquiring() {
        return m_hOperation != 0;
    }
    inline void SetUserData(void* userdata) {
        m_UserData = userdata;
    }
    inline void* GetUserData() {
        return m_UserData;
    }
    inline PBITMAPINFO GetFingerImage() {
        return m_FingerImage;
    }
    inline size_t GetFingerImageSize() {
        return m_FingerImageSize;
    }
    inline FT_BYTE* GetFingerTemplate() {
        return m_FingerTemplate;
    }
    inline int GetFingerTemplateSize() {
        return m_FingerTemplateSize;
    }
    inline int GetTemplateCount() {
        return m_nRegFingerprint;
    }
    bool Start();
    bool Stop();
    int Verify(const DATA_BLOB* data);
    void ResetTemplate();
    void AddTemplate(FT_BYTE* tmpl);
    void RemoveTemplate();
    bool GenerateRegTemplate();
    void GetRegTemplate(DATA_BLOB &rTemplate) const;
    int HandleMessage(WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(FP_HANDLER_INCLUDED_)
