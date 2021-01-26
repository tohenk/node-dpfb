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

class App {

    config = {}

    constructor() {
        this.initialize();
    }

    initialize() {
        let filename;
        // read configuration from command line values
        if (process.env.FP_CONFIG && fs.existsSync(process.env.FP_CONFIG)) {
            filename = process.env.FP_CONFIG;
        } else if (Cmd.get('config') && fs.existsSync(Cmd.get('config'))) {
            filename = Cmd.get('config');
        } else if (fs.existsSync(path.join(__dirname, 'config.json'))) {
            filename = path.join(__dirname, 'config.json');
        }
        if (filename) {
            console.log('Reading configuration %s', filename);
            this.config = JSON.parse(fs.readFileSync(filename));
        }
        // check for default configuration
        if (!this.config.debug) {
            this.config.debug = false;
        }
        if (Cmd.get('port')) {
            this.config.port = Cmd.get('port');
        }
        if (Cmd.get('mode')) {
            this.config.mode = Cmd.get('mode');
        }
        if (this.config.mode) {
            switch (this.config.mode.toLowerCase()) {
                case 'bridge':
                    this.config.mode = 1;
                    break;
                case 'verifier':
                    this.config.mode = 2;
                    break;
                case 'mixed':
                    this.config.mode = 3;
                    break;
            }
        }
    }

    run(options) {
        return new Promise((resolve, reject) => {
            options = options || {};
            const port = this.config.port || 7879;
            const http = require('http').createServer();
            const opts = {};
            if (this.config.cors) {
                opts.cors = this.config.cors;
            } else {
                opts.cors = {origin: '*'};
            }
            const io = require('socket.io')(http, opts);
            http.listen(port, () => {
                const logdir = this.config.logdir || path.join(__dirname, 'logs');
                if (!fs.existsSync(logdir)) fs.mkdirSync(logdir);
                const logfile = path.join(logdir, 'app.log');
                const logger = new Logger(logfile);
                const fp = new FingerprintBridge({
                    socket: io,
                    logger: logger,
                    mode: this.config.mode || 1,
                    proxies: this.config.proxies || [],
                    onstatus: (status, priority) => {
                        console.log('FP: %s', status);
                        if (typeof options.onstatus == 'function') {
                            options.onstatus(status, priority);
                        }
                    }
                });
                process.on('exit', (code) => {
                    fp.finalize();
                });
                process.on('SIGTERM', () => {
                    fp.finalize();
                });
                process.on('SIGINT', () => {
                    process.exit();
                });
                console.log('%s ready on port %s...', options.title ? options.title : 'DPFP Verifier', port);
                resolve(fp);
            });
        });
    }

}

const app = new App();

function run(options) {
    app.run(options);
}

if (require.main === module) {
    run();
} else {
    module.exports = {config: app.config, run: run}
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