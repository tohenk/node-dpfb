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

const {parentPort, threadId} = require('worker_threads');
const verifier = require('bindings')('dpaxver');
const util  = require('util');
const ntUtil = require('./lib/util');

let stopped = false;
let proccessing = false;

function verify(work, start, end) {
    stopped = false;
    proccessing = true;
    let count = 0;
    let matched = null;
    log('%d> Verifying %s from %d to %d', threadId, work.id, start, end);
    for (let idx = start; idx <= end; idx++) {
        if (stopped) {
            log('%d> Verify stopped %s', threadId, work.id);
            break;
        }
        count++;
        try {
            if (verifier.verify(work.feature, work.fingers[idx]) == 0) {
                log('%d> Found matched at %d', threadId, idx);
                matched = idx;
                break;
            }
        }
        catch (err) {
            error('%d> %d - %s', threadId, idx, err);
        }
    }
    proccessing = false;
    log('%d> Done verifying %d sample(s)', threadId, count);
    parentPort.postMessage({cmd: 'done', work: work, matched: matched, worker: threadId});
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
        case 'verify':
            verify(data.work, data.start, data.end);
            break;
        case 'stop':
            if (proccessing) {
                stopped = true;
            }
            break;
    }
});
