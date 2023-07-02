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

class Channel {

    /**
     * Constructor.
     *
     * @param {object} options
     */
    constructor(options) {
        this.options = options || {};
        if (typeof this.options.logger === 'function') {
            this.logger = this.options.logger;
        }
        this.templates = new Map();
        this.init();
    }

    init() {
    }

    /**
     * Register identity template.
     *
     * @param {number} id Identity id
     * @param {string} data Identity data
     * @returns {boolean} Wheter identity successfully registered or not
     */
    add(id, data) {
        throw new TypeError('Not implemented');
    }

    /**
     * Deregister identity template.
     *
     * @param {number} id Identity id
     * @returns {boolean} Wheter identity successfully deregistered or not
     */
    remove(id) {
        throw new TypeError('Not implemented');
    }

    /**
     * Check if identity is already registered.
     *
     * @param {number} id Identity id
     * @returns {boolean} Returns true if identity was registered, false otherwise
     */
    has(id) {
        throw new TypeError('Not implemented');
    }

    /**
     * Get the number of registered identities.
     *
     * @returns {number} The number of registered identities
     */
    count() {
        throw new TypeError('Not implemented');
    }

    /**
     * Remove all registered identities.
     */
    clear() {
        throw new TypeError('Not implemented');
    }

    /**
     * Normalize template data.
     *
     * @param {any} data 
     * @returns {any}
     */
    normalize(data) {
        if (typeof this.options.normalize === 'function') {
            return this.options.normalize(data);
        }
        return data;
    }

    /**
     * Perform identification and the get matched identity.
     * 
     * The returned promise will have signature of:
     *
     * {
     *     ...
     *     "data": {
     *         "matched": matched_id_or_null
     *     }
     * }
     *
     * @param {string} id Identify sequence id
     * @param {string} feature Identity feature template
     * @returns {Promise}
     */
    identify(id, feature) {
        throw new TypeError('Not implemented');
    }

    /**
     * Log message to console.
     *
     * @param {...} args Arguments
     */
    log() {
        const args = Array.from(arguments);
        if (typeof this.logger === 'function') {
            this.logger(...args);
        } else {
            console.log(...args);
        }
    }

    /**
     * Generate an ID.
     *
     * @returns {string}
     */
    genId() {
        const crypto = require('crypto');
        const u = require('@ntlab/ntlib/util');
        const shasum = crypto.createHash('sha1');
        shasum.update(u.formatDate(new Date(), 'yyyyMMddHHmmsszzz') + (Math.random() * 1000000).toString());
        return shasum.digest('hex').substr(0, 8);
    }
}

module.exports = Channel;