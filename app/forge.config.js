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

const fs = require('fs');
const path = require('path');

module.exports = {
    packagerConfig: {
        asar: true,
        icon: 'assets/icons/app'
    },
    rebuildConfig: {},
    makers: [
        {
            name: '@electron-forge/maker-squirrel',
            config: {
                name: 'DPFB',
                iconUrl: 'https://raw.githubusercontent.com/tohenk/node-dpfb/master/app/assets/icons/app.ico',
                setupIcon: path.join(__dirname, 'assets', 'icons', 'app.ico'),
            }
        },
        {
            name: '@electron-forge/maker-zip',
            platforms: ['win32', 'darwin', 'linux'],
        }
    ],
    plugins: [
        {
            name: '@electron-forge/plugin-auto-unpack-natives',
            config: {},
        },
    ],
    hooks: {
        postMake: async (forgeConfig, results) => {
            results.forEach(result => {
                if (Array.isArray(result.artifacts)) {
                    result.artifacts.forEach(artifact => {
                        const artifactName = path.basename(artifact);
                        if (artifactName.match(/\.(exe|zip)$/)) {
                            const artifactSafename = artifactName.replace(result.packageJSON.productName, `${result.packageJSON.productName}-${result.platform}-${result.arch}`).replace(/\s/g, '-');
                            if (artifactName !== artifactSafename) {
                                fs.renameSync(artifact, path.join(path.dirname(artifact), artifactSafename));
                            }
                        }
                    });
                }
            });
        }
    }
}
