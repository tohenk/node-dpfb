/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2019 Toha <tohenk@yahoo.com>
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

/**
 * Logger utility.
 */

const fs            = require('fs');
const util          = require('util');
const ntUtil        = require('./util');

class Logger {

    constructor(filename) {
        this.logfile = filename;
        this.dateFormat = 'dd-MM HH:mm:ss.zzz';
        this.create();
    }

    create() {
        this.stdout = new fs.createWriteStream(this.logfile, {flags: 'a'});
        this.logger = new console.Console(this.stdout);
    }

    log() {
        const args = Array.from(arguments);
        const time = new Date();
        return new Promise((resolve, reject) => {
            this.rotate(time)
                .then(() => {
                    if (args.length) {
                        args[0] = ntUtil.formatDate(time, this.dateFormat) + ' ' + args[0];
                    }
                    const message = util.format.apply(null, args);
                    this.logger.log(message);
                    resolve(message);
                })
            ;
        });
    }

    rotate(time) {
        if (time == undefined) {
            time = new Date();
        }
        if (!this.time) {
            const info = fs.statSync(this.logfile);
            this.time = new Date(info.mtime);
        }
        if (time.getDate() != this.time.getDate()) {
            this.time = time;
            return new Promise((resolve, reject) => {
                var filename;
                var seq = 0;
                while (true) {
                    filename = util.format('%s.%d', this.logfile, seq++);
                    if (!fs.existsSync(filename)) {
                        break;
                    }
                }
                fs.rename(this.logfile, filename, (err) => {
                    if (err) return reject(err);
                    this.create();
                    resolve();
                });
            });
        } else {
            return Promise.resolve();
        }
    }
}

module.exports = exports = Logger;
