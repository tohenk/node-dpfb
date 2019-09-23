/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2019 Toha <tohenk@yahoo.com>
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

/*
 * Command line utility
 */

const CmdParser = module.exports = exports;

const Cmds = {};

CmdParser.PARAM_VAR = 1;
CmdParser.PARAM_BOOL = 2;

CmdParser.register = function(type, name, shortname, desc, varname, defaultValue) {
    if (!Cmds[name]) {
        Cmds[name] = {
            name: name,
            shortname: shortname,
            description: desc,
            type: type || PARAM_BOOL,
            value: defaultValue || null,
            varname: varname || null,
            accessible: true
        }
        this.lastCmd = name;
    }
    return this;
}

CmdParser.addBool = function(name, shortname, desc, defaultValue) {
    return this.register(this.PARAM_BOOL, name, shortname, desc, defaultValue);
}

CmdParser.addVar = function(name, shortname, desc, varname, defaultValue) {
    return this.register(this.PARAM_VAR, name, shortname, desc, varname, defaultValue);
}

CmdParser.setAccessible = function(accessible) {
    if (this.lastCmd && Cmds[this.lastCmd]) {
        Cmds[this.lastCmd].accessible = accessible;
    }
    return this;
}

CmdParser.get = function(name) {
    if (Cmds[name]) {
        return Cmds[name]['value'];
    }
}

CmdParser.has = function(name) {
    return Cmds[name] ? true : false;
}

CmdParser.hasShort = function(shortname) {
    for (var name in Cmds) {
        if (Cmds[name]['shortname'] == shortname) return name;
    }
}

CmdParser.dump = function() {
    var str, len = 0, _res = [], _cmds = [], _descs = [];
    for (var name in Cmds) {
        var Cmd = Cmds[name];
        if (!Cmd.accessible) continue;
        str = this.cmdStr(Cmd, false);
        if (Cmd.shortname) {
            str += ', ' + this.cmdStr(Cmd, true);
        }
        if (str.length > len) len = str.length;
        _cmds.push(str);
        _descs.push(Cmd.description);
    }
    len += 2;
    for (var i = 0; i < _cmds.length; i++) {
        str = _cmds[i];
        if (str.length < len) {
            str += ' '.repeat(len - str.length);
        }
        _res.push(str + _descs[i]);
    }
    return _res.join("\n");
}

CmdParser.cmdStr = function(Cmd, shortCmd) {
    var str = shortCmd ? '-' + Cmd.shortname : '--' + Cmd.name;
    if (Cmd.type == this.PARAM_VAR) {
        str += '=' + (Cmd.varname ? Cmd.varname : Cmd.name);
    }
    return str; 
}

CmdParser.parse = function(arguments) {
    var arguments = arguments || process.argv.slice(2);
    var err = null;
    while (true) {
        if (!arguments.length) break;
        var arg = arguments[0];
        var param = null;
        var value = null;
        var shortparam = false;
        // check for long parameter format
        if ('--' == arg.substr(0, 2)) {
            param = arg.substr(2);
        // check for short parameter format
        } else if ('-' == arg.substr(0, 1)) {
            param = arg.substr(1);
            shortparam = true;
        }
        // not parameter, just give up
        if (!param) break;
        // check for parameter separator
        if (param.indexOf('=') > 0) {
            value = param.substr(param.indexOf('=') + 1);
            param = param.substr(0, param.indexOf('='));
        }
        // try to get the standard parameter name
        if (shortparam) {
            var longname = this.hasShort(param);
            if (longname) param = longname;
        }
        // check the existence of parameter
        if (!this.has(param)) {
            err = 'Unknown argument "' + param + '".';
            break;
        }
        // validate parameter
        if (Cmds[param]['type'] == this.PARAM_VAR && !value) {
            err = 'Argument "' + param + '" need a value to be assigned.';
            break;
        }
        if (Cmds[param]['type'] == this.PARAM_BOOL && value) {
            err = 'Argument "' + param + '" doesn\'t accept a value.';
            break;
        }
        // set the value
        Cmds[param]['value'] = value ? value : true;
        // remove processed parameter
        arguments = arguments.slice(1);
    }
    if (err) {
        console.log(err);
        console.log('');
    }
    return err ? false : true;
}
