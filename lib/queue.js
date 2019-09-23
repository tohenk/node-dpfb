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

const EventEmitter  = require('events');

class Queue extends EventEmitter {
    constructor(queues, handler, check) {
        super();
        this.queues = queues;
        this.handler = handler;
        this.check = check;
        this.next();
    }

    applyHandler() {
        this.once('queue', this.handler);
    }

    start() {
        process.nextTick(() => {
            if (this.queues.length) {
                const queue = this.queues.shift();
                this.emit('queue', queue);
            }
        });
    }

    next() {
        if (this.queues.length) {
            if (this.pending) return;
            if (typeof this.check == 'function') {
                if (!this.check()) return;
            }
            this.applyHandler();
            this.start();
        } else {
            this.done();
        }
    }

    done() {
        process.nextTick(() => {
            this.emit('done');
        });
    }

    requeue(queues) {
        const processNext = this.queues.length == 0;
        Array.prototype.push.apply(this.queues, queues);
        if (processNext) this.next();
    }
}

module.exports = Queue;
