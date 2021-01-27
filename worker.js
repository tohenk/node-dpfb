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

const { parentPort, threadId } = require('worker_threads');
const util  = require('util');
const ntUtil = require('@ntlab/ntlib/util');
const ntQueue = require('@ntlab/ntlib/queue');
const dpfp = require('@ntlab/dplib');

const identifyCount = dpfp.getIdentificationLen();
let queue = null;

function verify(work, start, end) {
    let count = 0;
    let matched = null;
    log('%d> Verifying %s from %d to %d', threadId, work.id, start, end);
    let fps = [];
    let current = start;
    while (current <= end) {
        let data = {
            start: current,
            fmds: []
        };
        for (let i = 0; i < identifyCount; i++) {
            if (current + i > end) {
                break;
            }
            data.fmds.push(work.fingers[current + i]);
        }
        fps.push(data);
        current += identifyCount;
    }
    queue = new ntQueue(fps, (d) => {
        try {
            count += d.fmds.length;
            dpfp.identify(work.feature, d.fmds)
                .then((idx) => {
                    if (idx >= 0) {
                        log('%d> Found matched at %d', threadId, d.start + idx);
                        matched = d.start + idx;
                        queue.done();
                    } else {
                        queue.next();
                    }
                })
                .catch((err) => {
                    error('%d> %d - %s', threadId, d.start, err);
                    queue.next();
                })
            ;
        }
        catch (err) {
            error('%d> %d - %s', threadId, d.start, err);
            queue.next();
        }
    });
    queue.once('done', () => {
        log('%d> Done verifying %d sample(s)', threadId, count);
        parentPort.postMessage({cmd: 'done', work: work, matched: matched, worker: threadId});
    });
}

function fmt(args) {
    if (Array.isArray(args) && args.length) {
        const time = new Date();
        args[0] = ntUtil.formatDate(time, 'dd-MM HH:mm:ss.zzz') + ' ' + args[0];
        return util.format.apply(null, args);
    }
}

function log() {
    console.log(fmt(Array.from(arguments)));
}

function error() {
    console.error(fmt(Array.from(arguments)));
}

parentPort.on('message', (data) => {
    switch (data.cmd) {
        case 'do':
            verify(data.work, data.start, data.end);
            break;
        case 'stop':
            if (queue) {
                log('%d> Stopping queue', threadId);
                queue.clear();
                queue.done();
            }
            break;
    }
});
