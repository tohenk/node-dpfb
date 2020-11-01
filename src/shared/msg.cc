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

#include <windows.h>
#include "msg.h"

MsgWin::MsgWin()
{
    mWndClass = L"FPMessageReceiver";
    mTitle = L"Fingerprint Message Receiver";
    CreateHandle();
}

MsgWin::~MsgWin()
{
    DestroyHandle();
}

void MsgWin::CreateHandle()
{
    mhInstance = GetModuleHandle(NULL);
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = &MsgWin::WndProc;
    wx.hInstance = mhInstance;
    wx.lpszClassName = mWndClass;
    if (RegisterClassEx(&wx)) {
        mhWnd = CreateWindowEx(0, mWndClass, mTitle, WS_POPUP | WS_EX_DLGMODALFRAME /*| WS_VISIBLE*/,
            0, 0, 0, 0, NULL, NULL, NULL, NULL);
        SetWindowLongPtr(mhWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}

void MsgWin::DestroyHandle()
{
    if (mhWnd) {
        UnregisterClass(mWndClass, mhInstance);
        DestroyWindow(mhWnd);
    }
}

void MsgWin::ProcessMessages()
{
    MSG msg;
    // must be PeekMessage
    if (PeekMessage(&msg, mhWnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK MsgWin::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    try {
        MsgWin *self = reinterpret_cast<MsgWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (self) {
            if (self->mHandler) {
                if (self->mHandler(uMsg, wParam, lParam)) {
                    return 0;
                }
            }
        }
    }
    catch (...) {
        // nothing
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
