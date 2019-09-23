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

const args = process.argv.slice(2);
if (args.length && (args[0] == 'install' || args[0] == 'uninstall')) {
    service(args[0]);
} else {
    console.log('Usage: node fpserver.js [install|uninstall]');
}

function service(cmd) {
    const fs = require('fs');
    const path = require('path');
    const Service = require('node-windows').Service;
    const dir = fs.realpathSync(path.join(__dirname, '..', '..'));
    const app = path.join(dir, 'app.js');
    const svc = new Service({
        name: 'NodeFPServer',
        description: 'Node Fingerprint Server',
        script: app,
        env: {
            name: 'FP_CONFIG',
            value: path.join(dir, 'fpserver.json')
        }
    });
    svc.on('install', () => {
        svc.start();
    });
    switch (cmd) {
        case 'install':
            svc.install();
            break;
        case 'uninstall':
            svc.uninstall();
            break;
    }
}
