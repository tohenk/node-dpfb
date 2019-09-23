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

const {Worker} = require('worker_threads');
const crypto = require('crypto');
const ntQueue = require('./lib/queue');
const ntUtil = require('./lib/util');

class FingerprintIdentifier {
    constructor(options) {
        var options = options || {};
        this.maxFinger = options.maxFinger || 100;
        this.maxWorker = options.maxWorker || require('os').cpus().length;
        this.workers = [];
        this.queues = [];
        this.templates = new Map();
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

    convertToBuffer(str) {
        const data = new Uint8Array(str.length);
        for (let i = 0; i < str.length; i++) {
            data[i] = str.charCodeAt(i);
        }

        return data.buffer;
    }

    identify(feature) {
        return new Promise((resolve, reject) => {
            if (!this.templates.size) {
                return reject('Templates empty');
            }
            const workId = this.hashgen();
            const ids = Array.from(this.templates.keys());
            const fingers = Array.from(this.templates.values());
            const count = Math.ceil(ids.length / this.maxFinger);
            console.log('Starting identify %s', workId);
            let worker = null;
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
            const q = new ntQueue(sequences, (seq) => {
                if (worker && worker.idle) {
                    let start = (seq - 1) * this.maxFinger;
                    let end = Math.min(start + this.maxFinger, ids.length - 1);
                    worker.workId = workId;
                    worker.idle = false;
                    worker.w.postMessage({cmd: 'verify', work: work, start: start, end: end});
                    worker = null;
                    q.next();
                }
            }, () => {
                worker = this.getWorker(workId);
                return worker && worker.idle ? true : false;
            });
            q.once('finish', (data) => {
                work.finish = Date.now();
                this.removeQueue(workId);
                if (data.matched != null) {
                    data.matched = ids[data.matched]
                }
                console.log('Done %s in %d ms, match is %s', workId, work.finish - work.start, data.matched != null ? data.matched : 'none');
                resolve({id: workId, data: data});
            });
            this.queues.push({
                id: workId,
                queue: q
            });
        });
    }

    getWorker(workId) {
        let worker = null;
        // find idle worker
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].idle) {
                worker = this.workers[i];
                break;
            }
        }
        // create worker if needed
        if (worker == null && this.workers.length <= this.maxWorker) {
            worker = {
                workId: workId,
                idle: false,
                w: new Worker('./worker.js')
            }
            worker.w.on('message', (data) => {
                switch (data.cmd) {
                    case 'done':
                        worker.idle = true;
                        this.processQueue(worker.workId, data.matched);
                        break;
                }
            });
            worker.w.on('error', (err) => {
                console.log(err);
            });
            worker.w.on('exit', (code) => {
                console.log('Exit: %d', code);
                delete this.workers[worker];
            });
            worker.w.on('online', () => {
                worker.idle = true;
                this.processQueue(worker.workId);
            });
            this.workers.push(worker);
        }
        return worker;
    }

    stopWorker(workId) {
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].workId == workId) {
                this.workers[i].w.postMessage({cmd: 'stop'});
            }
        }
    }

    isWorking(workId) {
        let count = 0;
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].workId == workId) {
                if (!this.workers[i].idle) {
                    count++;
                }
            }
        }
        return count > 0 ? true : false;
    }

    processQueue(workId, matched) {
        this.queues.forEach((q) => {
            if (q.id === workId && q.queue) {
                // worker is done
                if (typeof matched != 'undefined') {
                    // match has found
                    if (null != matched) {
                        this.stopWorker(workId);
                        q.queue.emit('finish', {matched: matched});
                    } else if (!this.isWorking(workId)) {
                        // no match
                        q.queue.emit('finish', {matched: null});
                    } else {
                        // process next queue
                        q.queue.next();
                    }
                } else {
                    // process next queue
                    q.queue.next();
                }
            }
        })
    }

    removeQueue(workId) {
        for (let i = 0; i < this.queues.length; i++) {
            if (this.queues[i].workId === workId) {
                this.queues.splice(i, 1);
                break;
            }
        }
    }

    hashgen() {
        const shasum = crypto.createHash('sha1');
        shasum.update(ntUtil.formatDate(new Date(), 'yyyyMMddHHmmsszzz') + (Math.random() * 1000000).toString());
        return shasum.digest('hex').substr(0, 8);
    }
}

module.exports = FingerprintIdentifier;