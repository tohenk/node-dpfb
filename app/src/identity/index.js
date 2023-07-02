/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Toha <tohenk@yahoo.com>
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

const util = require('util');
const ntutil = require('@ntlab/ntlib/util');

class Identity {

    ID_ACQUIRE = 1
    ID_ENROLL = 2
    ID_STOP = 3

    constructor(options) {
        this.options = options || {};
        this.backend = options.backend;
        this.win = this.options.win;
        this.proxies = this.options.proxies || [];
        this.channelType = this.options.channelType || 'worker';
        this.logger = this.options.logger;
        this.onstatus = this.options.onstatus;
        this.resetted = false;
        this.init();
        this.start();
    }

    init() {
        if (this.backend) {
            this.backend.parent = this;
            this.backend.handle(this.getStateCommands());
            this.backend.handle(this.getIdentityCommands());
        }
    }

    start() {
        if (this.backend && typeof this.backend.onstart === 'function') {
            this.backend.onstart();
        }
    }

    finalize() {
    }

    getIdentifier() {
        if (!this.identifier) {
            const options = {
                normalize: data => this.normalizeTemplate(data),
                logger: (...args) => this.log(...args),
            }
            if (this.proxies.length) {
                Object.assign(options, {proxies: this.proxies}, this.proxyOptions || {});
                const ChannelProxy = require('./channel/proxy');
                this.identifier = new ChannelProxy(options);
            } else {
                Object.assign(options, this.workerOptions || {});
                let ChannelClass;
                switch (this.channelType) {
                    case 'worker':
                        ChannelClass = require('./channel/worker');
                        break;
                    case 'cluster':
                        ChannelClass = require('./channel/cluster');
                        break;
                }
                if (!ChannelClass) {
                    throw new Error(`Unresolved channel type ${this.channelType}!`);
                }
                this.identifier = new ChannelClass(options);
            }
            if (this.id) {
                this.identifier.id = this.id;
            }
        }
        return this.identifier;
    }

    getStateCommands() {
    }

    getIdentityCommands() {
    }

    normalizeTemplate(data) {
        return data;
    }

    sendMessage(message, data) {
        if (this.backend) {
            this.backend.send(message, data);
        }
    }

    setStatus(status, priority = false) {
        if (typeof this.onstatus === 'function') {
            this.onstatus(status, priority);
        }
    }

    log() {
        const args = Array.from(arguments);
        const time = new Date();
        if (args.length) {
            let prefix = ntutil.formatDate(time, 'MM-dd HH:mm:ss.zzz');
            if (this.id) {
                prefix += ` ${this.id}>`;
            }
            args[0] = `${prefix} ${args[0]}`;
        }
        if (typeof this.logger === 'function') {
            const message = util.format(...args);
            this.logger(message);
        } else {
            console.log(...args);
        }
    }

    reset() {
        if (!this.resetted) {
            this.resetted = true;
            if (typeof this.onreset === 'function') {
                this.onreset();
            }
        }
    }
}

module.exports = Identity;