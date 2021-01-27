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

const path = require('path');
const myapp = require('./app');
const { app, Menu, Tray, Notification } = require('electron');

let trayIcon = null;

function getTrayMenu() {
    return  Menu.buildFromTemplate([
        {
            id: 'ctx-app-name',
            label: app.getName()
        },
        {
            id: 'ctx-separator-1',
            label: '-',
            type: 'separator'
        },
        {
            id: 'ctx-status-info',
            label: 'Ready'
        },
        {
            id: 'ctx-separator-2',
            label: '-',
            type: 'separator'
        },
        {
            id: 'ctx-app-quit',
            label: 'Quit',
            click: () => {
                app.quit();
            }
        }
    ]);
}

function createTrayIcon() {
    const iconName = 'tray-icon.png';
    const iconPath = path.join(__dirname, 'assets', 'icons', iconName);
    trayIcon = new Tray(iconPath);
    trayIcon.setToolTip(app.getName());
    trayIcon.setContextMenu(getTrayMenu());
}

function handleAppEvents() {
    app.on('ready', () => {
        createTrayIcon();
    })
    app.on('window-all-closed', () => {
        if (process.platform !== 'darwin') {
            app.quit();
        }
    });
    app.on('activate', () => {
        // nothing
    });
    app.on('will-quit', () => {
        // nothing
    });
}

// main entry

(function run() {
    handleAppEvents();
    myapp.run({
        title: app.getName(),
        onstatus: (status, priority) => {
            if (priority && Notification.isSupported()) {
                let notification = new Notification({
                    title: app.getName(),
                    body: status
                });
                notification.show();
            }
        }
    });
})();
