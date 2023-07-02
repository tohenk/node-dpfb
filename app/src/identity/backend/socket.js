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

const debug = require('debug')('identity:socket');

class Socket {

    clients = []
    handlers = []

    constructor(options) {
        this.options = options;
    }

    handle(handlers) {
        if (typeof handlers === 'object') {
            this.handlers.push(handlers);
        }
    }

    send(message, data) {
        this.log('Send %s with %s', message, data);
        this.clients.forEach(socket => {
            socket.emit(message, data);
        });
    }

    onstart() {
        if (this.io === undefined) {
            const { Server } = require('socket.io');
            const http = this.options.http;
            if (!http) {
                throw new Error('HTTP server must be passed in options!');
            }
            const config = this.options.config || {};
            if (!config.cors) {
                config.cors = {origin: '*'};
            }
            const namespace = `/${this.options.namespace ? this.options.namespace : ''}`;
            this.io = new Server(http, config);
            this.io.of(namespace)
                .on('connection', socket => {
                    this.handleConnection(socket);
                });
            const addr = http.address();
            const url = `${addr.port === 443 ? 'https' : 'http'}://${addr.family === 'IPv6' ? '[' + addr.address + ']' : addr.address}${addr.port !== 80 && addr.port !== 443 ? ':' + addr.port : ''}${namespace}`; 
            this.log('Socket connection ready on %s', url);
        }
    }

    handleConnection(socket) {
        if (this.clients.indexOf(socket) < 0) {
            this.clients.push(socket);
        }
        this.log('Client connected: %s', socket.id);
        socket
            .on('disconnect', () => {
                this.log('Client disconnected: %s', socket.id);
                const idx = this.clients.indexOf(socket);
                if (idx >= 0) {
                    this.clients.splice(idx, 1);
                }
            });
        this.handlers.forEach(handlers => {
            Object.keys(handlers).forEach(channel => {
                socket.on(channel, data => {
                    this.log('%s> handle message: %s', socket.id, channel);
                    socket.emit(channel, handlers[channel](null, data));
                });
            });
        });
    }

    log() {
        const args = Array.from(arguments);
        if (this.parent) {
            this.parent.log(...args);
        } else {
            debug(...args);
        }
    }
}

module.exports = Socket;