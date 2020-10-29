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

const util  = require('util');
const JSZip = require('jszip');
const ntUtil = require('./lib/util');

class FingerprintBridge {
    VERSION = 'DPFPBRIDGE-1.0'
    MODE_BRIDGE = 1
    MODE_VERIFIER = 2
    MODE_MIXED = 3
    FP_ACQUIRE = 1
    FP_ENROLL = 2
    FP_STOP = 3

    constructor(options) {
        var options = options || {};
        if (undefined == options.socket) {
            throw new Error('Required option socket is not supplied!');
        }
        this.mode = options.mode || this.MODE_BRIDGE;
        this.proxies = options.proxies || [];
        this.socket = options.socket;
        this.logger = options.logger;
        this.onstatus = options.onstatus;
        this.enrollWithSamples = true;
        this.initialized = false;
        this.init();
    }

    init() {
        if (this.mode == this.MODE_BRIDGE || this.mode == this.MODE_MIXED) {
            this.dp = require('./dpax');
            this.dp.init(() => {
                this.initialized = true;
            });
        }
        if ((this.mode == this.MODE_VERIFIER || this.mode == this.MODE_MIXED) && this.proxies.length) {
            this.getIdentifier();
        }
        this.socket.on('connection', (con) => {
            this.handleConnection(con);
        });
    }

    finalize() {
        if (this.initialized) {
            this.dp.exit();
        }
    }

    handleConnection(con) {
        con.on('disconnect', () => {
            this.log('Connection %s ended', con.id);
        });
        con.on('self-test', () => {
            con.emit('self-test', this.VERSION);
        });
        if (this.mode == this.MODE_BRIDGE || this.mode == this.MODE_MIXED) {
            this.log('Bridge connection %s', con.id);
            this.handleBridgeCommand(con);
        }
        if (this.mode == this.MODE_VERIFIER || this.mode == this.MODE_MIXED) {
            this.log('Verifier connection %s', con.id);
            this.handleVerifierCommand(con);
        }
    }

    handleBridgeCommand(con) {
        con.on('required-features', () => {
            if (this.initialized) {
                if (undefined == this.featuresLen) {
                    this.featuresLen = this.dp.getFeaturesLen();
                }
                con.emit('required-features', this.featuresLen);
            }
        });
        con.on('set-options', (options) => {
            if (undefined != options.enrollWithSamples) {
                this.enrollWithSamples = options.enrollWithSamples;
            }
        });
        con.on('acquire', () => {
            this.fingerOp(this.FP_ACQUIRE, con);
        });
        con.on('enroll', () => {
            this.fingerOp(this.FP_ENROLL, con);
        });
        con.on('stop', () => {
            this.fingerOp(this.FP_STOP, con);
        });
    }

    handleVerifierCommand(con) {
        con.on('identify', (data) => {
            if (data.feature) {
                this.fingerIdentify(con, data.feature, data.workid);
            }
        });
        con.on('count-template', () => {
            const count = this.getIdentifier().count();
            con.emit('count-template', {
                count: count
            });
        });
        con.on('reg-template', (data) => {
            if (data.id && data.template) {
                if (data.force && this.getIdentifier().has(data.id)) {
                    this.getIdentifier().remove(data.id);
                }
                const success = this.getIdentifier().add(data.id, data.template);
                this.log('Register template %d [%s]', data.id, success ? 'OK' : 'FAIL');
                con.emit('reg-template', {
                    id: data.id,
                    success: success
                });
            }
        });
        con.on('unreg-template', (data) => {
            if (data.id) {
                const success = this.getIdentifier().remove(data.id);
                this.log('Unregister template %d [%s]', data.id, success ? 'OK' : 'FAIL');
                con.emit('unreg-template', {
                    id: data.id,
                    success: success
                });
            }
        });
        con.on('has-template', (data) => {
            if (data.id) {
                const exist = this.getIdentifier().has(data.id);
                con.emit('has-template', {
                    id: data.id,
                    exist: exist
                });
            }
        });
        con.on('clear-template', () => {
            this.getIdentifier().clear();
        });
    }

    fingerOp(op, con) {
        if (this.initialized) {
            const stopAcquire = (callback) => {
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
                if (op == this.FP_ENROLL) {
                    this.fingers = [];
                }
                this.dp.startAcquire(op == this.FP_ENROLL ? true : false, (status, image, data) => {
                    switch (status) {
                    case 'disconnected':
                        this.setStatus('Connect fingerprint reader', true);
                        con.emit(op == this.FP_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        break;
                    case 'connected':
                        this.setStatus('Swipe your finger', true);
                        con.emit(op == this.FP_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        break;
                    case 'error':
                        this.setStatus('Error occured, try again', true);
                        con.emit(op == this.FP_ENROLL ? 'enroll-status' : 'acquire-status', {status: status});
                        break;
                    case 'complete':
                        if (op == this.FP_ENROLL) {
                            this.setStatus('Enroll completed', true);
                            this.fingers.push(data);
                            con.emit('enroll-complete', {image: image, data: data});
                        } else {
                            this.setStatus('Acquire completed', true);
                            stopAcquire(() => {
                                con.emit('acquire-complete', {image: image, data: data});
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
                                .then((buffer) => {
                                    this.setStatus('Notify for enroll finished');
                                    con.emit('enroll-finished', {template: buffer});
                                })
                            ;
                        });
                        break;
                    }
                });
            }
            // main operation
            stopAcquire(() => {
                if (op !== this.FP_STOP) {
                    startAcquire();
                }
            });
        }
    }

    fingerIdentify(con, feature, workid) {
        const id = workid || con.id;
        this.getIdentifier().identify(id, feature)
            .then((data) => {
                this.log('Got identify response for %s => %s', id, JSON.stringify(data));
                con.emit('identify', data);
            })
        ;
    }

    getIdentifier() {
        if (!this.identifier) {
            if (this.proxies.length) {
                const FPIdentifierProxy = require('./identifier-proxy');
                this.identifier = new FPIdentifierProxy({proxies: this.proxies, logger: this.log});
            } else {
                const FPIdentifierDP = require('./identifier-dp');
                this.identifier = new FPIdentifierDP({logger: this.log});
            }
        }
        return this.identifier;
    }

    setStatus(status, priority = false) {
        if (this.logger) {
            this.logger.log(status);
        }
        if (typeof this.onstatus == 'function') {
            this.onstatus(status, priority);
        }
    }

    log() {
        const args = Array.from(arguments);
        const time = new Date();
        if (args.length) {
            args[0] = ntUtil.formatDate(time, 'dd-MM HH:mm:ss.zzz') + ' ' + args[0];
        }
        const message = util.format.apply(null, args);
        console.log(message);
    }
}

module.exports = FingerprintBridge;