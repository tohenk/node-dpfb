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

// https://www.codeproject.com/Articles/9014/Understanding-COM-Event-Handling

#ifndef TEVENT_HANDLER_H
#define TEVENT_HANDLER_H

#include <windows.h>

namespace TEventHandlerNamespace
{
  // Generic event handler template class (especially useful (but not limited to) for non-ATL clients).
  template <class event_handler_class, typename device_interface, typename device_event_interface>
  class TEventHandler: IDispatch
  {
    friend class class_event_handler;

    typedef HRESULT (event_handler_class::*on_invoke_handler)(
      TEventHandler<event_handler_class, device_interface, device_event_interface>* pthis,
      DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, 
      VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

  public :
    TEventHandler(event_handler_class& parent, device_interface* pdevice_interface,  // Non-ref counted.
      on_invoke_handler on_invoke) :
      m_cRef(1),
      m_Parent(parent),
      m_OnInvoke(on_invoke),
      m_pIConnectionPoint(0),
      m_dwEventCookie(0)
    {
      SetupConnectionPoint(pdevice_interface);
    }

    ~TEventHandler()
    {
      // Call ShutdownConnectionPoint() here JUST IN CASE connection points are still 
      // alive at this time. They should have been disconnected earlier.
      ShutdownConnectionPoint();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
      InterlockedIncrement(&m_cRef);

      return m_cRef;  
    }

    STDMETHOD_(ULONG, Release)()
    {
      InterlockedDecrement(&m_cRef);
      if (m_cRef == 0) {
        delete this;
        return 0;
      }

      return m_cRef;
    }

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject)
    {
      if (riid == IID_IUnknown) {
        *ppvObject = (IUnknown*)this;
        AddRef();
        return S_OK;
      }

      if ((riid == IID_IDispatch) || (riid == __uuidof(device_event_interface))) {
        *ppvObject = (IDispatch*)this;
        AddRef();
        return S_OK;
      }

      return E_NOINTERFACE;
    }

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {
      return E_NOTIMPL;
    }

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {
      return E_NOTIMPL;
    }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,
      DISPID* rgdispid)
    {
      return E_NOTIMPL;
    }

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
    {
      return (m_Parent.*m_OnInvoke)(this, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }

  protected:
    LONG                    m_cRef;
    // Pertaining to the owner of this object.
    event_handler_class&    m_Parent;  // Non-reference counted. This is to prevent circular references.
    // Pertaining to connection points.
    IConnectionPoint*       m_pIConnectionPoint;  // Ref counted of course.
    DWORD	                  m_dwEventCookie;
    on_invoke_handler        m_OnInvoke;

    void SetupConnectionPoint(device_interface* pdevice_interface)
    {
      IUnknown* pIUnknown = NULL;
      // QI this object itself for its IUnknown pointer which will be used 
      // later to connect to the Connection Point of the device_interface object.
      this->QueryInterface(IID_IUnknown, (void**)&pIUnknown);
      if (pIUnknown) {
        IConnectionPointContainer* pIConnectionPointContainer = NULL;
        // QI the pdevice_interface for its connection point.
        pdevice_interface->QueryInterface(IID_IConnectionPointContainer, (void**)&pIConnectionPointContainer);
        if (pIConnectionPointContainer) {
          pIConnectionPointContainer->FindConnectionPoint(__uuidof(device_event_interface), &m_pIConnectionPoint);
          pIConnectionPointContainer->Release();
          pIConnectionPointContainer = NULL;
        }
        if (m_pIConnectionPoint) {
          m_pIConnectionPoint->Advise(pIUnknown, &m_dwEventCookie);
        }
        pIUnknown->Release();
        pIUnknown = NULL;
      }
    }

  public:
    void ShutdownConnectionPoint()
    {
      if (m_pIConnectionPoint) {
        m_pIConnectionPoint->Unadvise(m_dwEventCookie);
        m_dwEventCookie = 0;
        m_pIConnectionPoint->Release();
        m_pIConnectionPoint = NULL;
      }
    }
  };
};

#endif