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

const path = require('path');
const Cmd = require('./lib/cmd');

Cmd.addBool('help', 'h', 'Show program usage').setAccessible(false);
Cmd.addVar('config', '', 'Read app configuration from file', 'config-file');
Cmd.addVar('port', 'p', 'Set web server port to listen', 'port');
Cmd.addVar('mode', 'm', 'Set server mode: bridge, verifier, or mixed', 'mode');
Cmd.addVar('logdir', '', 'Set the log file location', 'directory');

if (!Cmd.parse() || (Cmd.get('help') && usage())) {
    process.exit();
}

const fs = require('fs');
const Logger = require('./lib/logger');
const FingerprintBridge = require('./bridge');

let config = {};
let configFile;
// read configuration from command line values
if (process.env.FP_CONFIG && fs.existsSync(process.env.FP_CONFIG)) {
    configFile = process.env.FP_CONFIG;
} else if (Cmd.get('config') && fs.existsSync(Cmd.get('config'))) {
    configFile = Cmd.get('config');
} else if (fs.existsSync(path.join(__dirname, 'config.json'))) {
    configFile = path.join(__dirname, 'config.json');
}
if (configFile) {
    console.log('Reading configuration %s', configFile);
    config = JSON.parse(fs.readFileSync(configFile));
}
// check for default configuration
if (!config.debug) {
    config.debug = false;
}
if (Cmd.get('port')) {
    config.port = Cmd.get('port');
}
if (Cmd.get('mode')) {
    config.mode = Cmd.get('mode');
}
if (config.mode) {
    switch (config.mode.toLowerCase()) {
        case 'bridge':
            config.mode = 1;
            break;
        case 'verifier':
            config.mode = 2;
            break;
        case 'mixed':
            config.mode = 3;
            break;
    }
}

function startFingerprintBridge(options) {
    var options = options || {};
    const port = config.port || 7879;
    const http = require('http').createServer();
    const io = require('socket.io')(http);
    http.listen(port, () => {
        const logdir = config.logdir || path.join(__dirname, 'logs');
        if (!fs.existsSync(logdir)) fs.mkdirSync(logdir);
        const logfile = path.join(logdir, 'app.log');
        const logger = new Logger(logfile);
        fp = new FingerprintBridge({
            socket: io,
            logger: logger,
            mode: config.mode || 1,
            proxies: config.proxies || [],
            onstatus: (status, priority) => {
                console.log('FP: %s', status);
                if (typeof options.onstatus == 'function') {
                    options.onstatus(status, priority);
                }
            }
        });
        console.log('%s ready on port %s...', options.title ? options.title : 'DPFP Verifier', port);
    });
}

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

if (require.main === module) {
    startFingerprintBridge();
} else {
    module.exports = {config: config, run: startFingerprintBridge}
}
