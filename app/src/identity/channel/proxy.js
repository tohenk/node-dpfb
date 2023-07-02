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

const Channel = require('./index');

class Proxy extends Channel {

    init() {
        this.worker = this.options.worker;
        this.proxies = this.options.proxies;
        this.proxies.forEach((url) => {
            const Worker = require(this.worker);
            this.workers.push(new Worker({serverid: this.options.serverid, url: url, logger: this.logger}));
        });
        this.next = 0;
    }

    add(id, data) {
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].has(id)) {
                return false;
            }
        }
        this.workers[this.next].add(id, data);
        if (++this.next === this.workers.length) {
            this.next = 0;
        }
        return true;
    }

    remove(id) {
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].has(id)) {
                this.workers[i].remove(id);
                return true;
            }
        }
        return false;
    }

    has(id) {
        for (let i = 0; i < this.workers.length; i++) {
            if (this.workers[i].has(id)) {
                return true;
            }
        }
        return false;
    }

    count() {
        let count = 0;
        for (let i = 0; i < this.workers.length; i++) {
            count += this.workers[i].count();
        }
        return count;
    }

    clear() {
        for (let i = 0; i < this.workers.length; i++) {
            this.workers[i].clear();
        }
    }

    identify(id, feature) {
        return new Promise((resolve, reject) => {
            let count = 0;
            let done = 0;
            let start = Date.now();
            let resolved = false;
            const workId = this.genId();
            this.workers.forEach(worker => {
                if (worker.isConnected()) {
                    count++;
                    worker.identify(workId, feature)
                        .then(response => {
                            done++;
                            if (response.data) {
                                // matched found or all has been done
                                if ((response.data.matched != null || done === count) && !resolved) {
                                    resolved = true;
                                    let finish = Date.now();
                                    this.log('Done in %d ms, match is %s for %s', finish - start,
                                        response.data.matched != null ? response.data.matched : 'none', id);
                                    response.ref = id;
                                    resolve(response);
                                }
                            } else {
                                if (done === count && !resolved) {
                                    resolve({ref: id, data: {matched: null}});
                                }
                            }
                        })
                    ;
                }
            });
            // no worker, just assume as no matching found
            if (count === 0) {
                resolve({ref: id, data: {matched: null}});
            }
        });
    }
}

module.exports = Proxy;