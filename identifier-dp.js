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

const os = require('os');
const FingerprintIdentifier = require('./identifier');
const { Worker } = require('worker_threads');
const ntQueue = require('@ntlab/ntlib/queue');

class FingerprintIdentifierDP extends FingerprintIdentifier {
    constructor(options) {
        super();
        var options = options || {};
        if (typeof options.logger == 'function') {
            this.logger = options.logger;
        }
        this.maxFinger = options.maxFinger || 160; // 10 passes
        this.maxWorker = options.maxWorker || require('os').cpus().length;
        this.templates = new Map();
        this.workers = [];
        this.processing = [];
    }

    add(id, data) {
        if (!this.has(id)) {
            this.templates.set(id, this.convertToBuffer(data));
            return true;
        }
        return false;
    }

    remove(id) {
        if (this.has(id)) {
            this.templates.delete(id);
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
        this.templates.clear();
    }

    convertToBuffer(str) {
        const data = new Uint8Array(str.length);
        for (let i = 0; i < str.length; i++) {
            data[i] = str.charCodeAt(i);
        }

        return data.buffer;
    }

    identify(id, feature) {
        return new Promise((resolve, reject) => {
            if (!this.templates.size) {
                return resolve({ref: id, data: null});
            }
            const workers = [];
            const workId = this.genId();
            const ids = Array.from(this.templates.keys());
            const fingers = Array.from(this.templates.values());
            const count = Math.ceil(ids.length / this.maxFinger);
            this.log('Starting identify %s with %d sample(s) for %s', workId, fingers.length, id);
            let sequences = [];
            for (let i = 1; i <= count; i++) {
                sequences.push(i);
            }
            const work = {
                id: workId,
                fingers: fingers,
                feature: this.convertToBuffer(feature),
                start: Date.now()
            }
            let result = {matched: null};
            const q = new ntQueue(sequences, (seq) => {
                let start = (seq - 1) * this.maxFinger;
                let end = Math.min(start + this.maxFinger, ids.length) - 1;
                let w = null;
                const cleanwork = (exit) => {
                    if (w) {
                        if (workers.indexOf(w) >= 0) {
                            workers.splice(workers.indexOf(w), 1);
                        }
                        if (this.processing.indexOf(w) >= 0) {
                            this.processing.splice(this.processing.indexOf(w), 1);
                        }
                        if (exit) {
                            this.workers.splice(this.workers.indexOf(w), 1);
                        }
                    }
                }
                // use idle worker if possible
                for (let i = 0; i < this.workers.length; i++) {
                    if (this.processing.indexOf(this.workers[i]) < 0) {
                        w = this.workers[i];
                        this.processing.push(w);
                        break;
                    }
                }
                if (w == null) {
                    w = new Worker('./worker.js');
                    w.on('error', (err) => {
                        this.log(err);
                        cleanwork();
                    });
                    w.on('exit', (code) => {
                        this.log('Exit: %d', code);
                        cleanwork(true);
                    });
                    this.workers.push(w);
                    this.processing.push(w);
                }
                w.once('message', (data) => {
                    switch (data.cmd) {
                        case 'done':
                            cleanwork();
                            this.log('Worker %d: %s done with %s', data.worker, workId, data.matched);
                            if (data.matched != null) {
                                result.matched = data.matched;
                                q.done();
                            } else {
                                q.next();
                            }
                            break;
                    }
                });
                w.once('error', (err) => cleanwork());
                w.once('exit', (code) => cleanwork(true));
                workers.push(w);
                w.postMessage({cmd: 'do', work: work, start: start, end: end});
                q.next();
            }, () => {
                return workers.length < this.maxWorker;
            });
            q.once('done', () => {
                // notify to stop when a matched is already found
                if (result.matched != null) {
                    for (let i = 0; i < workers.length; i++) {
                        workers[i].postMessage({cmd: 'stop'});
                    }
                }
                // wait for on going work
                let count = workers.length;
                let workdone;
                (workdone = () => {
                    if (!workers.length) {
                        work.finish = Date.now();
                        if (result.matched != null) {
                            result.matched = ids[result.matched]
                        }
                        this.log('Done %s in %d ms, match is %s for %s', workId, work.finish - work.start,
                            result.matched != null ? result.matched : 'none', id);
                        resolve({ref: id, id: workId, data: result});
                     } else {
                         if (count != workers.length) {
                             count = workers.length;
                             this.log('Still waiting %s worker(s) to finish for %s', workId, count, id);
                         }
                         setTimeout(workdone, 500);
                     }
                })();
            });
        });
    }
}

module.exports = FingerprintIdentifierDP;