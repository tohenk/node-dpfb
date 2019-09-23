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

CFPHandler::CFPHandler():
    m_spCapture(NULL),
    m_spEnroll(NULL),
    m_spVerify(NULL),
    m_spConversion(NULL),
    m_spFeature(NULL),
    m_pEvent(NULL),
    m_FingerImage(NULL),
    m_FingerTemplate(NULL),
    m_FingerSample(NULL),
    m_Started(false),
    m_UserData(NULL)
{
    // ***** Create an instance of an object which implements IDPFPCapture. *****
    m_spCapture.CreateInstance(__uuidof(DPFPCapture));
    // ***** Create an instance of an object which implements IDPFPEnrollment. *****
    m_spEnroll.CreateInstance(__uuidof(DPFPEnrollment));
    // ***** Create an instance of an object which implements IDPFPEnrollment. *****
    m_spVerify.CreateInstance(__uuidof(DPFPVerification));
    // ***** Create an instance of an object which implements IDPFPSampleConversion. *****
    m_spConversion.CreateInstance(__uuidof(DPFPSampleConversion));
    // ***** Create an instance of an object which implements IDPFPFeatureExtraction. *****
    m_spFeature.CreateInstance(__uuidof(DPFPFeatureExtraction));
    // ***** Create event handler. *****
    CreateEventHandler();
}

CFPHandler::~CFPHandler()
{
    if (m_pEvent) {
        m_pEvent->ShutdownConnectionPoint();
        m_pEvent->Release();
        m_pEvent = NULL;
    }
}

HRESULT CFPHandler::OnInvoke(IDPFPCaptureEventHandler* pEventHandler, DISPID dispidMember,
    REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams,
    VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    DPFPDataPurposeEnum mode;
    DPFPEngXLib::DPFPCaptureFeedbackEnum feedback;
    int status = FP_STATUS_NONE;
    switch (dispidMember) {
    case FP_COMPLETE:
        m_FingerSample = V_DISPATCH(&(pdispparams->rgvarg)[0]);
        mode = m_OperationMode == FP_ACQUIRE_ENROLL ? DataPurposeEnrollment : DataPurposeVerification;
        feedback = m_spFeature->CreateFeatureSet(m_FingerSample, mode);
        if (feedback == DPFPEngXLib::CaptureFeedbackGood) {
            status = FP_STATUS_COMPLETE;
            m_FingerImage = m_spConversion->ConvertToPicture(m_FingerSample);
            m_FingerTemplate = m_spFeature->FeatureSet;
        }
        break;
    case FP_READER_DISCONNECT:
        status = FP_STATUS_DISCONNECTED;
        break;
    case FP_READER_CONNECTED:
        status = FP_STATUS_CONNECTED;
        break;
    case FP_FINGER_TOUCH:
        break;
    case FP_FINGER_GONE:
        break;
    case FP_SAMPLE_QUALITY:
        break;
    }
    if (m_Handler) {
        m_Handler(status);
    }
    return S_OK;
}

bool CFPHandler::CreateEventHandler()
{
    if (m_pEvent == NULL) {
        m_pEvent = new IDPFPCaptureEventHandler(*this, m_spCapture, &CFPHandler::OnInvoke);
        return true;
    }
    return false;
}

bool CFPHandler::Start()
{
    if (!m_Started) {
        m_Started = true;
        HRESULT res = m_spCapture->StartCapture();
        return res == S_OK ? true : false;
    }
    return false;
}

bool CFPHandler::Stop()
{
    if (m_Started) {
        m_Started = false;
        HRESULT res = m_spCapture->StopCapture();
        return res == S_OK ? true : false;
    }
    return false;
}

int CFPHandler::Verify(IDPFPTemplatePtr data)
{
    if (m_FingerTemplate) {
        IDPFPVerificationResultPtr res = m_spVerify->Verify(m_FingerTemplate, data);
        return res->Verified ? FP_VERIFY_MATCHED : FP_VERIFY_NO_MATCH;
    }
    return FP_VERIFY_NO_TEMPLATE;
}

int CFPHandler::Identify(IDPFPFeatureSetPtr tmpl, IDPFPTemplatePtr data)
{
    IDPFPVerificationResultPtr res = m_spVerify->Verify(tmpl, data);
    return res->Verified ? FP_VERIFY_MATCHED : FP_VERIFY_NO_MATCH;
}

void CFPHandler::ResetTemplate()
{
    m_spEnroll->Clear();
}

void CFPHandler::AddTemplate(IDPFPFeatureSetPtr tmpl)
{
    m_spEnroll->AddFeatures(tmpl);
}

bool CFPHandler::GenerateRegTemplate()
{
    if (m_spEnroll->TemplateStatus == TemplateStatusTemplateReady) {
        return true;
    }
    return false;
}

IDPFPTemplatePtr CFPHandler::GetRegTemplate()
{
    return m_spEnroll->Template;
}

