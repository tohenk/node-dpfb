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

#include "common.h"
#include "fp.h"

/////////////////////////////////////////////////////////////////////////////
// CFPHandler

CFPHandler::CFPHandler() :
    m_Initialized(false),
    m_ReaderConnected(false),
    m_hWnd(NULL),
    m_hOperation(0),
    m_OperationMode(FP_ACQUIRE_VERIFY),
    m_fxContext(0),
    m_mcContext(0),
    m_TemplateArray(NULL),
    m_nRegFingerprint(0),
    m_mcRegOptions(FT_DEFAULT_REG),
    m_FingerImage(NULL),
    m_FingerTemplate(NULL),
    m_Learning(FT_FALSE),
    m_UserData(NULL)
{
    ::ZeroMemory(&m_RegTemplate, sizeof(m_RegTemplate));
}

CFPHandler::~CFPHandler()
{
    delete [] m_RegTemplate.pbData;
    m_RegTemplate.cbData = 0;
    m_RegTemplate.pbData = NULL;

    Finalize();
}

int CFPHandler::DoInit()
{
    FT_RETCODE rc;

    // Create Context for Feature Extraction
    if (FT_OK != (rc = FX_createContext(&m_fxContext))) {
        return FP_INIT_ERR_FX;
    }

    // Create Context for Matching
    if (FT_OK != (rc = MC_createContext(&m_mcContext))) {
        return FP_INIT_ERR_MX;
    }

    // Get number of Pre-Enrollment feature sets needed to create on Enrollment template
    // allocate array that keeps those Pre-Enrollment and set the first index to 0;
    MC_SETTINGS mcSettings = {0};
    if (FT_OK != (rc = MC_getSettings(&mcSettings))) {
        return FP_INIT_ERR_SETTINGS;
    }

    m_NumberOfPreRegFeatures = mcSettings.numPreRegFeatures;
    if (NULL == (m_TemplateArray = new FT_BYTE*[m_NumberOfPreRegFeatures]))
        return FP_INIT_ERR_OUT_OF_MEMORY;

    return FP_INIT_OK;
}

void CFPHandler::ProcessImageBlob(const DATA_BLOB* data)
{
    if (data) {
        ProcessImage(data);
        ProcessTemplate(data);
    }
}

void CFPHandler::ProcessImage(const DATA_BLOB* data)
{
    m_FingerImageSize = 0;
    DPFPConvertSampleToBitmap(data, 0, &m_FingerImageSize);
    m_FingerImage = (PBITMAPINFO)new BYTE[m_FingerImageSize];
    DPFPConvertSampleToBitmap(data, (PBYTE)m_FingerImage, &m_FingerImageSize);
}

void CFPHandler::ProcessTemplate(const DATA_BLOB* data)
{
    m_FingerTemplate = NULL;
    m_FingerTemplateSize = 0;
    FT_BYTE* tmpl = NULL;
    FT_FTR_TYPE mode = m_OperationMode == FP_ACQUIRE_ENROLL ? FT_PRE_REG_FTR : FT_VER_FTR;
    FT_RETCODE rc = FX_getFeaturesLen(mode,
                                      &m_FingerTemplateSize,
                                      NULL); // Minimal Fingerprint feature set Length - do not use if possible
    if (FT_OK != rc)
        _com_issue_error(E_FAIL);

    // Extract Features (i.e. create fingerprint template)
    FT_IMG_QUALITY imgQuality;
    FT_FTR_QUALITY ftrQuality;
    FT_BOOL bSuccess = FT_FALSE;
    if (NULL == (tmpl = new FT_BYTE[m_FingerTemplateSize]))
        _com_issue_error(E_OUTOFMEMORY);
    rc = FX_extractFeatures(m_fxContext,              // Context of Feature Extraction
                            data->cbData,             // Size of the fingerprint image
                            data->pbData,             // Pointer to the image buffer
                            mode,                     // Requested Features mode
                            m_FingerTemplateSize,     // Length of the Features(template) buffer received previously from the SDK
                            tmpl,                     // Pointer to the buffer where the template to be stored
                            &imgQuality,              // Returned quality of the image. If feature extraction fails because used did not put finger on the reader well enough, this parameter be used to tell the user how to put the finger on the reader better. See FT_IMG_QUALITY enumeration.
                            &ftrQuality,              // Returned quality of the features. It may happen that the fingerprint of the user cannot be used. See FT_FTR_QUALITY enumeration.
                            &bSuccess);               // Returned result of Feature extraction. FT_TRUE means extracted OK.

    // Check if feature extraction succeeded
    if (FT_OK <= rc && bSuccess == FT_TRUE) {
        m_FingerTemplate = tmpl;
    }
}

bool CFPHandler::Initialize()
{
    if (!m_Initialized) {
        m_Res = DoInit();
        m_Initialized = true;
    }

    return m_Res == FP_INIT_OK;
}

void CFPHandler::Finalize()
{
    if (IsAcquiring()) {
        Stop();
    }
    if (m_fxContext) {
        FX_closeContext(m_fxContext);
        m_fxContext = 0;
    }
    if (m_mcContext) {
        MC_closeContext(m_mcContext);
        m_mcContext = 0;
    }
    RemoveTemplate();
}

bool CFPHandler::Start()
{
    if (Initialize() && m_hWnd != NULL && m_hOperation == 0) {
        // Start Enrollment.
        DP_ACQUISITION_PRIORITY ePriority = DP_PRIORITY_NORMAL; // Using Normal Priority, i.e. fingerprint will be sent to 
                                                                // this process only if it has active window on the desktop.
        _com_test_error(DPFPCreateAcquisition(ePriority, GUID_NULL, DP_SAMPLE_TYPE_IMAGE, m_hWnd, WMUS_FP_NOTIFY, &m_hOperation));
        _com_test_error(DPFPStartAcquisition(m_hOperation));

        return true;
    }
    return false;
}

bool CFPHandler::Stop()
{
    if (Initialize() && m_hOperation) {
        DPFPStopAcquisition(m_hOperation);    // No error checking - what can we do at the end anyway?
        DPFPDestroyAcquisition(m_hOperation);
        m_hOperation = 0;

        return true;
    }
    return false;
}

int CFPHandler::Verify(const DATA_BLOB* data)
{
    if (m_FingerTemplate != NULL) {
        FT_BOOL bRegFeaturesChanged = FT_FALSE;
        FT_VER_SCORE VerScore = FT_UNKNOWN_SCORE;
        double dFalseAcceptProbability = 0.0;
        FT_BOOL bSuccess = FT_FALSE;

        FT_RETCODE rc = MC_verifyFeaturesEx(m_mcContext,              // Matching Context
                                            data->cbData,              // Pointer to the Enrollment template content
                                            data->pbData,              // Length of the Enrollment template
                                            m_FingerTemplateSize,     // Length of the Verification template
                                            m_FingerTemplate,         // Pointer to the Verification template content
                                            m_Learning,               // Whether the Learning is desired - got it from checkbox in the dialog
                                            &bRegFeaturesChanged,     // Out: Whether the Learning actually happened
                                            NULL,                     // Reserved, must be NULL
                                            &VerScore,                // Out: Matching score, see score enumeration FT_VER_SCORE
                                            &dFalseAcceptProbability, // Out: Probability to falsely match these fingerprints
                                            &bSuccess);               // Returned result of fingerprint match. FT_TRUE means fingerprints matched.
        if (FT_OK <= rc) {
            return bSuccess ? FP_VERIFY_MATCHED : FP_VERIFY_NO_MATCH;
        }
        return FP_VERIFY_ERR;
    }
    return FP_VERIFY_NO_TEMPLATE;
}

void CFPHandler::ResetTemplate()
{
    ::ZeroMemory(m_TemplateArray, sizeof(FT_BYTE**)*m_NumberOfPreRegFeatures);
    m_nRegFingerprint = 0;  // This is index of the array where the first template is put.
}

void CFPHandler::AddTemplate(FT_BYTE* tmpl)
{
    m_TemplateArray[m_nRegFingerprint++] = tmpl;
}

void CFPHandler::RemoveTemplate()
{
    if (m_TemplateArray) {
        for (int i=0; i<m_NumberOfPreRegFeatures; ++i)
            if(m_TemplateArray[i]) delete [] m_TemplateArray[i], m_TemplateArray[i] = NULL; // Delete Pre-Enrollment feature sets stored in the array.

        delete [] m_TemplateArray; // delete the array itself.
        m_TemplateArray = NULL;
    }
}

bool  CFPHandler::GenerateRegTemplate()
{
    FT_BYTE* tmpl = NULL;
    if (m_nRegFingerprint == m_NumberOfPreRegFeatures) { // We collected enough Pre-Enrollment feature sets, create Enrollment template out of them.
        // Get the recommended length of the fingerprint template (features).
        // It actually could be done once at the beginning, but this is just sample code...
        int iRecommendedRegFtrLen = 0;
        FT_RETCODE rc = MC_getFeaturesLen(FT_REG_FTR, m_mcRegOptions, &iRecommendedRegFtrLen, NULL);

        if (FT_OK != rc) _com_issue_error(E_FAIL);

        if (NULL == (tmpl = new FT_BYTE[iRecommendedRegFtrLen]))
            _com_issue_error(E_OUTOFMEMORY);

        FT_BOOL bSuccess = FT_FALSE;
        rc = MC_generateRegFeatures(m_mcContext,              // Matching Context
                                    m_mcRegOptions,           // Options - collected from checkboxes in the dialog
                                    m_NumberOfPreRegFeatures, // Number of Pre-Enrollment feature sets submitted. It must be number received previously from the SDK.
                                    m_FingerTemplateSize,     // Length of every Pre-Enrollment feature sets in the array
                                    m_TemplateArray,          // Array of pointers to the Pre-Enrollment feature sets
                                    iRecommendedRegFtrLen,    // Length of the Enrollment Features(template) buffer received previously from the SDK
                                    tmpl,                     // Pointer to the buffer where the Enrollment template to be stored
                                    NULL,                     // Reserved. Must always be NULL.
                                    &bSuccess);               // Returned result of Enrollment Template creation. FT_TRUE means extracted OK.

        if (FT_OK <= rc && bSuccess == FT_TRUE) {
            // Enrollment template is generated.
            // Normally it should be saved in some database, etc.
            // Here we just put it into a BLOB to use later for verification.
            m_RegTemplate.pbData = tmpl;
            m_RegTemplate.cbData = iRecommendedRegFtrLen;

            return true;
        }
    }
    return false;
}

void CFPHandler::GetRegTemplate(DATA_BLOB &rRegTemplate) const
{
    if (m_RegTemplate.cbData && m_RegTemplate.pbData) { // only copy template if it is not empty
        // Delete the old stuff that may be in the template.
        delete [] rRegTemplate.pbData;
        rRegTemplate.pbData = NULL;
        rRegTemplate.cbData = 0;

        // Copy the new template, but only if it has been created.
        rRegTemplate.pbData = new BYTE[m_RegTemplate.cbData];
        if (!rRegTemplate.pbData) _com_issue_error(E_OUTOFMEMORY);
        ::CopyMemory(rRegTemplate.pbData, m_RegTemplate.pbData, m_RegTemplate.cbData);
        rRegTemplate.cbData = m_RegTemplate.cbData;
    }
}

int CFPHandler::HandleMessage(WPARAM wParam, LPARAM lParam)
{
    int status = FP_STATUS_NONE;
    DATA_BLOB* pImageBlob = NULL;
    switch (wParam)
    {
    case WN_COMPLETED:
        status = FP_STATUS_COMPLETE;
        pImageBlob = reinterpret_cast<DATA_BLOB*>(lParam);
        ProcessImageBlob(pImageBlob);
        break;
    case WN_ERROR:
        status = FP_STATUS_ERROR;
        break;
    case WN_DISCONNECT:
        status = FP_STATUS_DISCONNECTED;
        m_ReaderConnected = false;
        break;
    case WN_RECONNECT:
        status = FP_STATUS_CONNECTED;
        m_ReaderConnected = true;
        break;
    case WN_SAMPLE_QUALITY:
        break;
    case WN_FINGER_TOUCHED:
        break;
    case WN_FINGER_GONE:
        break;
    case WN_IMAGE_READY:
        break;
    case WN_OPERATION_STOPPED:
        break;
    default:
        break;
    }
    return status;
}