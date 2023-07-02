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

const Channel = require('../channel');
const io = require('socket.io-client');

class ProxyWorker extends Channel {

    init() {
        if (!this.options.url) {
            throw new Error('Required url option must be set!');
        }
        this.url = this.options.url;
        this.removes = [];
        this.connected = false;
        this.socket = io.connect(this.url, {reconnect: true});
        this.socket.on('connect', () => {
            this.log('Connected to %s', this.url);
            this.socket.emit('self-test');
        });
        this.socket.on('disconnect', () => {
            this.connected = false;
            this.log('Disconnected from %s', this.url);
        });
        this.socket.on('self-test', response => {
            if (response) {
                let svrName = response.substr(0, response.indexOf('-'));
                let svrVer = response.substr(response.indexOf('-') + 1);
                if (svrName === this.options.serverid) {
                    this.log('Proxy connection ready');
                    this.connected = true;
                    this.server = {name: svrName, protocol: svrVer};
                    this.syncTemplates();
                } else {
                    this.log('Unexpected server, required %s got %s', this.options.serverid ? this.options.serverid : 'NONE', response);
                }
            }
        });
    }

    isConnected() {
        return this.connected;
    }

    syncTemplates(id) {
        if (this.connected) {
            this.templates.forEach((value, key) => {
                if (id === undefined || id === key) {
                    this.socket.emit('reg-template', {id: key, template: value});
                }
            });
            while (this.removes.length) {
                let removed = this.removes.shift();
                this.socket.emit('unreg-template', {id: removed});
            }
        }
    }

    add(id, data) {
        if (!this.has(id)) {
            this.templates.set(id, data);
            this.syncTemplates(id);
            return true;
        }
        return false;
    }

    remove(id) {
        if (this.has(id)) {
            this.templates.delete(id);
            if (this.connected) {
                this.socket.emit('unreg-template', {id: id});
            } else {
                this.removes.push(id);
            }
            return true;
        }
        return false;
    }

    has(id) {
        return this.templates.has(id);
    }

    count() {
        return this.templates.size;
    }

    clear() {
        this.templates.clear;
        if (this.connected) {
            this.socket.emit('clear-template');
        }
    }

    identify(id, feature) {
        return new Promise((resolve, reject) => {
            if (this.connected) {
                this.socket.emit('identify', {feature: feature, workid: id});
                const f = () => {
                    this.socket.once('identify', response => {
                        // ensure response is for our
                        if (response.ref === id) {
                            this.log('Got identify response with %s from %s', JSON.stringify(response), this.url);
                            resolve(response);
                        } else {
                            // listen once again
                            f();
                        }
                    });
                }
                f();
            } else {
                resolve();
            }
        });
    }
}

module.exports = ProxyWorker;