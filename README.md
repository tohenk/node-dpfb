# Digital Persona Fingerprint Bridge

## Introduction

Digital Persona Fingerprint Bridge (DPFB) aims to provide fingerprint
acquisition and identification in the browser environment. It is designed
to be compatible across browsers. DPFB has two main functions, first as
fingerprint acquisition to capture the fingerprint and send it to browser.
The second part is to identify fingerprint sent by the browser.

## How Does It Work

The workflow is described as follow:
```
    FP BRIDGE --- BROWSER --- SERVER --- FP SERVER
            Client side <- | -> Server side
```

Both FP BRIDGE and FP SERVER heavily utilize [socket.io](https://socket.io)
for its communication.

### FP BRIDGE

FP BRIDGE is a socket.io server while the BROWSER is a client. Normally it is
listening at port `7879`. FP BRIDGE accepts commands described below.

* `required-features`, it will return the numbers of fingerprints needed
  for enrollment.
* `set-options`, set the options, pass an options object with:

  * `enrollWithSamples` to include fingerprint samples when enrolling.

* `acquire`, start fingerprint acquisition for identification.

  Listen for the following events when capturing:
  * `acquire-status` which contains the capture status.
  * `acquire-complete` which contains the fingerprint data.

* `enroll`, start fingerprint enrollment.

  Listen for the following events when enrolling:
  * `enroll-status` which contains the capture status.
  * `enroll-complete` which contains the fingerprint data.
  * `enroll-finished` which contains the zipped fingerprint datas.

* `stop`, stop fingerprint acquisition or enrollment.

### FP SERVER

FP SERVER is a socket.io server. Normally it is listening at port `7978`.
FP SERVER accepts commands described below.

* `count-template`, it returns same event with an object contains `count`
  property.
* `reg-template`, pass an object with `id` and `template` property as argument.
  It returns same event with an object contains `id` and `success` property.
* `unreg-template`, pass an object with `id` property as argument.
  It returns same event with an object contains `id` and `success` property.
* `has-template`, pass an object with `id` property as argument to check if
  template has been registered. It returns same event with an object contains
  `id` and `exist` property.
* `clear-template`, clear all registered fingerprint templates.
* `identify`, pass an object with `feature` and `workid` property as argument
  to identify the feature against registered fingerprint templates.
  It returns same event with an object contains `ref` (work id), `id` (internal
  id of identification process), and `data` property. To check successfull
  operation of identification, examine `matched` property of `data`.

## Usage

### Requirements

* `node-gyp` must already been installed with its dependencies (build tools),
  to do so, type `npm install -g node-gyp`.
* Digital Persona U.are.U SDK has been installed.

### Building Distribution Package

Before building, be sure the make dependencies are all up to date, issue `npm update` to do so.

#### Building for 32-bit Windows Platform

```
npm run build:win32
npm run package:win32
```

The package then can be found in the `dist/Digital Persona Fingerprint Bridge-win32-ia32`
directory along with its executable `Digital Persona Fingerprint Bridge.exe`.

#### Building for 64-bit Windows Platform

```
npm run build:win64
npm run package:win64
```

The package then can be found in the `dist/Digital Persona Fingerprint Bridge-win32-x64`
directory along with its executable `Digital Persona Fingerprint Bridge.exe`.

### Running FP BRIDGE

Directly execute `Digital Persona Fingerprint Bridge.exe` from distribution
package or issue `npm start` if using source or when developing. When using
`npm start` make sure to build the binding correctly according to the running
operating system, either issue `npm run build:win32` or `npm run build:win64`.

### Running FP SERVER

The FP SERVER can be deployed in stand alone mode, which it directly identifies
fingerprint using the SDK. The other option is to act as proxy which queries
another stand alone FP SERVER located anywhere.

FP SERVER configurations is mainly located in the `package.json` under the
scripts section. Also there is `proxy.json` configuration for proxy mode.

#### Running as stand alone server on port 7978

```
npm run fpserver
```

#### Running as proxy server on port 7978

Running proxy worker on port 8001

```
npm run fpworker1
```

Running proxy worker on port 8002

```
npm run fpworker2
```

Running proxy server on port 7978

```
npm run fpproxy
```

## Live Demo

Live demo is available [here](https://ntlab.id/demo/digital-persona-fingerprint-bridge).

## Known Limitations

* It is currently support Windows and Linux only.
* On Linux, only FP SERVER has been tested and works as intended.
* When capturing fingerprint, DPFB app must have focused to be able receive
  captured data.
