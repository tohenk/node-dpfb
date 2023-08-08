/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2023 Toha <tohenk@yahoo.com>
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

const Cmd = require('@ntlab/ntlib/cmd');

Cmd.addBool('help', 'h', 'Show program usage').setAccessible(false);
Cmd.addVar('config', '', 'Read app configuration from file', 'config-file');
Cmd.addVar('port', 'p', 'Set web server port to listen', 'port');

if (require('./squirrel-event') || !Cmd.parse() || (Cmd.get('help') && usage())) {
    process.exit();
}

const fs = require('fs');
const path = require('path');
const Work = require('@ntlab/work/work');
const { app, dialog, ipcMain, BrowserWindow, Menu, Tray } = require('electron');

class MainApp
{
    config = {}
    logs = []
    ready = false

    initialize() {
        let filename;
        // read configuration from command line values
        if (process.env.APP_CONFIG && fs.existsSync(process.env.APP_CONFIG)) {
            filename = process.env.APP_CONFIG;
        } else if (Cmd.get('config') && fs.existsSync(Cmd.get('config'))) {
            filename = Cmd.get('config');
        } else if (fs.existsSync(path.join(__dirname, 'config.json'))) {
            filename = path.join(__dirname, 'config.json');
        }
        this.initializeConfig(filename);
        this.initializeIpc();
    }

    initializeConfig(filename) {
        if (fs.existsSync(filename)) {
            console.log('Reading configuration %s', filename);
            this.config = JSON.parse(fs.readFileSync(filename));
        }
        Object.assign(this.config, this.getConfigHandlers());
        if (!this.config.rootdir) {
            this.config.rootdir = __dirname;
        }
        if (!this.config.workdir) {
            this.config.workdir = process.cwd();
        }
        this.config.staticdir = path.join(this.config.rootdir, 'assets', 'static');
        this.config.icondir = path.join(this.config.rootdir, 'assets', 'icons');
        // check for default configuration
        if (typeof this.config.debug === 'undefined') {
            this.config.debug = false;
        }
        if (Cmd.has('port')) {
            this.config.port = parseInt(Cmd.get('port'));
        }
    }

    initializeIpc() {
        ipcMain.handle('state', (event, data) => {
            return {ready: this.ready};
        });
    }

    getConfigHandlers() {
        return {
            getTranslatedPath: path => {
                if (app.isPackaged && path.match(/app\.asar/)) {
                    const translated = path.replace(/app\.asar/, 'app.asar.unpacked');
                    const ofs = require('original-fs');
                    if (ofs.existsSync(translated)) {
                        path = translated;
                    }
                }
                return path;
            }
        }
    }

    getTrayMenu() {
        return  Menu.buildFromTemplate([
            {
                id: 'ctx-show-main',
                label: app.getName(),
                click: () => {
                    if (!this.win.isVisible()) {
                        this.win.show();
                    }
                }
            },
            {
                id: 'ctx-separator-1',
                label: '-',
                type: 'separator'
            },
            {
                id: 'ctx-app-quit',
                label: 'Quit',
                click: () => {
                    this.quit = true;
                    app.quit();
                }
            }
        ]);
    }

    getCommonPreferences() {
        return {
            webPreferences: {
                preload: path.join(this.config.staticdir, 'preload.js'),
            }
        };
    }

    showError(err) {
        if (this.splash) {
            this.splash.close();
        }
        if (!this.win.isVisible()) {
            this.win.show();
        }
        this.win.loadFile(path.join(this.config.staticdir, 'error', 'error.html'));
        this.win.webContents.on('dom-ready', () => {
            const util = require('util');
            this.win.webContents.send('error-message', util.inspect(err));
        });
    }

    createWin(options, fileOrUrl = null) {
        const w = new BrowserWindow(Object.assign({}, options, this.getCommonPreferences()));
        if (fileOrUrl) {
            if (fileOrUrl.match(/^http(s)?\:\/\/(.*)$/)) {
                w.loadURL(fileOrUrl);
            } else if (fs.existsSync(fileOrUrl)) {
                w.loadFile(fileOrUrl);
            }
        }
        return w;
    }

    createWindow() {
        return new Promise((resolve, reject) => {
            this.win = this.createWin({show: false, width: 600, height: 400, center: true},
                path.join(this.config.staticdir, 'log', 'log.html'));
            this.win.on('close', e => {
                if (!this.quit) {
                    e.preventDefault();
                    this.win.hide();
                }
            });
            const baseOptions = {parent: this.win, modal: true, minimizable: false, maximizable: false};
            this.splash = this.createWin(Object.assign({width: 500, height: 300, frame: false}, baseOptions),
                path.join(this.config.staticdir, 'splash', 'splash.html'));
            this.splash.webContents.on('dom-ready', () => resolve());
        });
    }

    configureApp() {
        return new Promise((resolve, reject) => {
            if (!this.config.debug) {
                Menu.setApplicationMenu(null);
            }
            resolve();
        });
    }

    createTrayIcon() {
        return new Promise((resolve, reject) => {
            const iconName = 'tray-icon.png';
            this.trayIcon = new Tray(path.join(this.config.icondir, iconName));
            this.trayIcon.setToolTip(app.getName());
            this.trayIcon.setContextMenu(this.getTrayMenu());
            resolve();
        });
    }

    startApp() {
        const MyApp = require('./src/app');
        this.app = new MyApp(this.config);
        Work.works([
            [w => this.createWindow()],
            [w => this.configureApp()],
            [w => this.createTrayIcon()],
            [w => this.app.start(this)],
        ])
        .then(() => {
            console.log(`App ${app.getName()} is ready...`);
            this.ready = true;
            BrowserWindow.getAllWindows().forEach(win => {
                win.webContents.send('ready', {ready: true});
            });
        })
        .catch(err => {
            this.showError(err);
        });
    }

    handleEvents() {
        app.whenReady().then(() => {
            if (!app.requestSingleInstanceLock()) {
                dialog.showMessageBoxSync({title: 'Error', message: `${app.getName()} already running!`, type: 'error'});
                app.quit();
            } else {
                this.startApp();
            }
        });
        app
            .on('window-all-closed', () => {
                if (process.platform !== 'darwin') {
                    app.quit();
                }
            })
            .on('activate', () => {
                // nothing
            })
            .on('will-quit', () => {
                // nothing
            });
    }

    getName() {
        return app.getName();
    }
    
    run() {
        this.initialize();
        this.handleEvents();
    }
}

// main entry

(function run() {
    new MainApp().run();
})();

// usage help

function usage() {
    console.log('Usage:');
    console.log('  node %s [options]', path.basename(process.argv[1]));
    console.log('');
    console.log('Options:');
    console.log(Cmd.dump());
    console.log('');
    return true;
}
