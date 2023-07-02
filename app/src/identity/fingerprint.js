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

const Identity = require('./index');
const path = require('path');
const util = require('util');
const JSZip = require('jszip');
const debug = require('debug')('identity:fingerprint');

class FingerprintId extends Identity {

    VERSION = 'DPFPBRIDGE-1.0'

    init() {
        super.init();
        this.ready = false;
        this.id = 'FP';
        this.enrollWithSamples = typeof this.options.enrollWithSamples !== 'undefined' ?
            this.options.enrollWithSamples : true;
        this.workerOptions = {
            worker: path.join(__dirname, 'worker', 'fingerprint'),
            maxWorks: 160,
        }
        this.proxyOptions = {
            serverid: 'DPFPBRIDGE',
            worker: path.join(__dirname, 'worker', 'proxy'),
        }
        this.dp = require('@ntlab/dplib');
        if (this.dp.init()) {
            this.getIdentifier();
            this.featuresLen = this.dp.getFeaturesLen();
            this.ready = true;
        }
    }

    finalize() {
        this.dp.exit();
    }

    getStateCommands() {
        return {
            'self-test': (event, data) => this.VERSION,
            'connect': (event, data) => this.ready,
        }
    }

    getIdentityCommands() {
        return {
            'required-features': (event, data) => {
                return this.featuresLen;
            },
            'set-options': (event, data) => {
                if (undefined !== data.enrollWithSamples) {
                    this.enrollWithSamples = data.enrollWithSamples;
                    return true;
                }
                return false;
            },
            'acquire': (event, data) => {
                this.doOp(this.ID_ACQUIRE);
                return true;
            },
            'enroll': (event, data) => {
                this.doOp(this.ID_ENROLL);
                return true;
            },
            'stop': (event, data) => {
                this.doOp(this.ID_STOP);
                return true;
            },
        }
    }

    normalizeTemplate(data) {
        if (typeof data === 'string') {
            const buff = new Uint8Array(data.length);
            for (let i = 0; i < data.length; i++) {
                buff[i] = data.charCodeAt(i);
            }
            data = buff;
        }
        if (data instanceof Uint8Array || Buffer.isBuffer(data)) {
            data = data.buffer;
        }
        return data;
    }

    doOp(op) {
        if (this.ready) {
            const stopAcquire = callback => {
                this.setStatus('Stopping acquire');
                if (this.dp.isAcquiring()) {
                    this.dp.stopAcquire(() => {
                        this.setStatus('Acquire stopped');
                        callback();
                    });
                } else {
                    this.setStatus('Stop not required');
                    callback();
                }
            }
            const startAcquire = () => {
                this.setStatus('Starting acquire');
                if (op === this.ID_ENROLL) {
                    this.fingers = [];
                }
                let xstatus = null;
                this.dp.startAcquire(op === this.ID_ENROLL ? true : false, (status, data) => {
                    switch (status) {
                    case 'disconnected':
                        if (xstatus != status) {
                            xstatus = status;
                            this.setStatus('Connect fingerprint reader', true);
                            this.sendMessage(op === this.ID_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        }
                        break;
                    case 'connected':
                        if (xstatus != status) {
                            xstatus = status;
                            this.setStatus('Swipe your finger', true);
                            this.sendMessage(op === this.ID_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        }
                        break;
                    case 'error':
                        this.setStatus('Error occured, try again', true);
                        this.sendMessage(op === this.ID_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        break;
                    case 'complete':
                        if (op === this.ID_ENROLL) {
                            this.setStatus('Enroll completed', true);
                            this.fingers.push(data);
                            this.sendMessage('enroll-complete', {data: data});
                        } else {
                            this.setStatus('Acquire completed', true);
                            stopAcquire(() => {
                                this.sendMessage('acquire-complete', {data: data});
                            });
                        }
                        break;
                    case 'enrolled':
                        this.setStatus('Enroll finished', true);
                        stopAcquire(() => {
                            const zip = new JSZip();
                            zip.file('TMPL', data);
                            if (this.enrollWithSamples) {
                                for (let i = 0; i < this.fingers.length; i++) {
                                    zip.file(util.format('S%s', i + 1), this.fingers[i]);
                                }
                            }
                            zip.generateAsync({type: 'nodebuffer'})
                                .then(buffer => {
                                    this.setStatus('Notify for enroll finished');
                                    this.sendMessage('enroll-finished', {template: buffer});
                                })
                            ;
                        });
                        break;
                    }
                });
            }
            // main operation
            stopAcquire(() => {
                if (op !== this.ID_STOP) {
                    startAcquire();
                }
            });
        }
    }

    doCmd(cmd, data = {}) {
        const res = {success: false};
        switch (cmd) {
            case 'identify':
                return this.fingerIdentify(data.feature, data.workid);
            case 'count-template':
                res.success = true;
                res.count = this.getIdentifier().count();
                break;
            case 'reg-template':
                if (data.id && data.template) {
                    if (data.force && this.getIdentifier().has(data.id)) {
                        this.getIdentifier().remove(data.id);
                    }
                    res.id = data.id;
                    res.success = this.getIdentifier().add(data.id, data.template);
                    debug(`Register template ${data.id} [${res.success ? 'OK' : 'FAIL'}]`);
                }
                break;
            case 'unreg-template':
                if (data.id) {
                    res.id = data.id;
                    res.success = this.getIdentifier().remove(data.id);
                    debug(`Unregister template ${data.id} [${res.success ? 'OK' : 'FAIL'}]`);
                }
                break;
            case 'has-template':
                if (data.id) {
                    res.id = data.id;
                    res.success = this.getIdentifier().has(data.id);
                }
                break;
            case 'clear-template':
                this.getIdentifier().clear();
                res.success = true;
                break;
        }
        return res;
    }

    fingerIdentify(feature, workid) {
        return this.getIdentifier().identify(workid, feature);
    }

    onreset() {
        this.doCmd('clear-template');
    }
}

module.exports = FingerprintId;