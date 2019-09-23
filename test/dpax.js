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

const dp = require("../dpax");

let templ;

function verify() {
	console.log("Swipe your finger to verify...");
	dp.startAcquire((status, image, data) => {
		switch (status) {
		case 'disconnected':
			break;
		case 'connected':
			break;
		case 'error':
			break;
		case 'complete':
			if (dp.verify(templ, (success) => {
				dp.stopAcquire(() => {
					if (success) {
						console.log('Finger matched');
						dp.exit();
					} else {
						console.log('Finger not matched, try again');
						verify();
					}
				});
			}));
			break;
		}
	});
}

function enroll() {
	let stage = 1;
	let stages = dp.getFeaturesLen();
	console.log("Enroll your finger! You will need swipe your finger %d times.", stages);
	dp.startAcquire(true, (status, image, data) => {
		switch (status) {
		case 'disconnected':
			console.log('Please connect fingerprint device...');
			break;
		case 'connected':
			console.log('Swipe your finger to enroll...');
			break;
		case 'error':
			console.log('Error occurred, please try again...');
			break;
		case 'complete':
			console.log('Finger acquired (%d/%d)', stage, stages);
			if (data) {
				//console.log('--DATA:%d--', data.length);
				//console.log(data);
			}
			stage++;
			break;
		case 'enrolled':
			if (data) {
				//console.log('--TEMPLATE:%d--', data.length);
				//console.log(data);
				templ = data;
			}
			dp.stopAcquire(() => {
				if (templ) {
					verify();
				}
			});
			break;
		}
	});
}
const ret = dp.init(() => {
	enroll();
});
