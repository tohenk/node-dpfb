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

#if !defined(SHARED_MSG_H_INCLUDED)
#define SHARED_MSG_H_INCLUDED

typedef bool (*msg_handler)(UINT, WPARAM, LPARAM);

class MsgWin {
public:
    MsgWin();
    ~MsgWin();

protected:
    HINSTANCE       mhInstance;
    HWND            mhWnd;
    LPCWSTR         mWndClass;
    LPCWSTR         mTitle;
    msg_handler     mHandler;

    void CreateHandle();
    void DestroyHandle();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    inline HWND GetHandle() {
        return mhWnd;
    }
    inline void SetHandler(msg_handler handler) {
        mHandler = handler;
    }
    void ProcessMessages();
};

#endif // !defined(SHARED_MSG_H_INCLUDED)
