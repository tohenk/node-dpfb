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

class Worker {

    static init() {
        this.type = process.argv.length > 2 ? process.argv[2].toLowerCase() : 'worker';
        switch (this.type) {
            case 'worker':
                const { parentPort, threadId } = require('worker_threads');
                this.id = threadId;
                this.port = parentPort;
                this.send = data => {
                    this.port.postMessage(data);
                }
                break;
            case 'cluster':
                this.id = process.pid;
                this.port = process;
                this.send = data => {
                    this.port.send(data);
                }
                break;
        }
        this.on = (event, handler) => {
            return this.port.on(event, handler);
        }
        return this;
    }
}

module.exports = Worker.init();