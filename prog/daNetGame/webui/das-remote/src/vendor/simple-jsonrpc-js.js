// https://github.com/jershell/simple-jsonrpc-js

// The MIT License (MIT)

// Copyright (c) 2019 QuickResto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// direct link: https://raw.githubusercontent.com/jershell/simple-jsonrpc-js/master/simple-jsonrpc-js.js

class JsonRpcConfig {
  MM_PROPAGATE = 0
  MM_LOG = 1
  MM_SILENCE = 2

  missingMethodPolicy = this.MM_PROPAGATE
}

(function (root) {
    'use strict';
    /*
     name: simple-jsonrpc-js
     version: 1.0.0
     */
    var _Promise = Promise;

    if (typeof _Promise === 'undefined') {
        _Promise = root.Promise;
    }

    if (_Promise === undefined) {
        throw 'Promise is not supported! Use latest version node/browser or promise-polyfill';
    }

    var isUndefined = function (value) {
        return value === undefined;
    };

    var isArray = Array.isArray;

    var isObject = function (value) {
        var type = typeof value;
        return value != null && (type == 'object' || type == 'function');
    };

    var isFunction = function (target) {
        return typeof target === 'function'
    };

    var isString = function (value) {
        return typeof value === 'string';
    };

    var isEmpty = function (value) {
        if (isObject(value)) {
            for (var idx in value) {
                if (value.hasOwnProperty(idx)) {
                    return false;
                }
            }
            return true;
        }
        if (isArray(value)) {
            return !value.length;
        }
        return !value;
    };

    var forEach = function (target, callback) {
        if (isArray(target)) {
            return target.map(callback);
        }
        else {
            for (var _key in target) {
                if (target.hasOwnProperty(_key)) {
                    callback(target[_key]);
                }
            }
        }
    };

    var clone = function (value) {
        return JSON.parse(JSON.stringify(value));
    };

    var ERRORS = {
        "PARSE_ERROR": {
            "code": -32700,
            "message": "Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text."
        },
        "INVALID_REQUEST": {
            "code": -32600,
            "message": "Invalid Request. The JSON sent is not a valid Request object."
        },
        "METHOD_NOT_FOUND": {
            "code": -32601,
            "message": "Method not found. The method does not exist / is not available."
        },
        "INVALID_PARAMS": {
            "code": -32602,
            "message": "Invalid params. Invalid method parameter(s)."
        },
        "INTERNAL_ERROR": {
            "code": -32603,
            "message": "Internal error. Internal JSON-RPC error."
        }
    };

    function ServerError(code, message, data) {
        this.message = message || "";
        this.code = code || -32000;

        if (Boolean(data)) {
            this.data = data;
        }
    }

    ServerError.prototype = new Error();

    var simple_jsonrpc = function (config) {

        config = config || new JsonPrcConfig()

        var self = this,
            waitingframe = {},
            id = 0,
            dispatcher = {};


        function setError(jsonrpcError, exception) {
            var error = clone(jsonrpcError);
            if (!!exception) {
                if (isObject(exception) && exception.hasOwnProperty("message")) {
                    error.data = exception.message;
                }
                else if (isString(exception)) {
                    error.data = exception;
                }

                if (exception instanceof ServerError) {
                    error = {
                        message: exception.message,
                        code: exception.code
                    };
                    if (exception.hasOwnProperty('data')) {
                        error.data = exception.data;
                    }
                }
            }
            return error;
        }

        function isPromise(thing) {
            return !!thing && 'function' === typeof thing.then;
        }

        function isError(message) {
            return !!message.error;
        }

        function isRequest(message) {
            return !!message.method;
        }

        function isResponse(message) {
            return message.hasOwnProperty('result') && message.hasOwnProperty('id');
        }

        function beforeResolve(message) {
            var promises = [];
            if (isArray(message)) {
                forEach(message, function (msg) {
                    promises.push(resolver(msg));
                });
            }
            else if (isObject(message)) {
                promises.push(resolver(message));
            }

            return _Promise.all(promises)
                .then(function (result) {

                    var toStream = [];
                    forEach(result, function (r) {
                        if (!isUndefined(r)) {
                            toStream.push(r);
                        }
                    });

                    if (toStream.length === 1) {
                        self.toStream(JSON.stringify(toStream[0]));
                    }
                    else if (toStream.length > 1) {
                        self.toStream(JSON.stringify(toStream));
                    }
                    return result;
                });
        }

        function resolver(message) {
            try {
                if (isError(message)) {
                    return rejectRequest(message);
                }
                else if (isResponse(message)) {
                    return resolveRequest(message);
                }
                else if (isRequest(message)) {
                    return handleRemoteRequest(message);
                }
                else {
                    return _Promise.resolve({
                        "id": null,
                        "jsonrpc": "2.0",
                        "error": setError(ERRORS.INVALID_REQUEST)
                    });
                }
            }
            catch (e) {
                console.error('Resolver error:' + e.message, e);
                return _Promise.reject(e);
            }
        }

        function rejectRequest(error) {
            if (waitingframe.hasOwnProperty(error.id)) {
                waitingframe[error.id].reject(error.error);
            }
            else {
                console.log('Unknown request', error);
            }
        }

        function resolveRequest(result) {
            if (waitingframe.hasOwnProperty(result.id)) {
                waitingframe[result.id].resolve(result.result);
                delete waitingframe[result.id];
            }
            else {
                console.log('unknown request', result);
            }
        }

        function handleRemoteRequest(request) {
            if (dispatcher.hasOwnProperty(request.method)) {
                try {
                    var result;

                    if (request.hasOwnProperty('params')) {
                        if (dispatcher[request.method].params == "pass") {
                            result = dispatcher[request.method].fn.call(dispatcher, request.params);
                        }
                        else if (isArray(request.params)) {
                            result = dispatcher[request.method].fn.apply(dispatcher, request.params);
                        }
                        else if (isObject(request.params)) {
                            if (dispatcher[request.method].params instanceof Array) {
                                var argsValues = [];
                                dispatcher[request.method].params.forEach(function (arg) {

                                    if (request.params.hasOwnProperty(arg)) {
                                        argsValues.push(request.params[arg]);
                                        delete request.params[arg];
                                    }
                                    else {
                                        argsValues.push(undefined);
                                    }
                                });

                                if (Object.keys(request.params).length > 0) {
                                    return _Promise.resolve({
                                        "jsonrpc": "2.0",
                                        "id": request.id,
                                        "error": setError(ERRORS.INVALID_PARAMS, {
                                            message: "Params: " + Object.keys(request.params).toString() + " not used"
                                        })
                                    });
                                }
                                else {
                                    result = dispatcher[request.method].fn.apply(dispatcher, argsValues);
                                }
                            }
                            else {
                                return _Promise.resolve({
                                    "jsonrpc": "2.0",
                                    "id": request.id,
                                    "error": setError(ERRORS.INVALID_PARAMS, "Undeclared arguments of the method " + request.method)
                                });
                            }
                        }
                        else {
                            result = dispatcher[request.method].fn.apply(dispatcher, [request.params]);
                        }
                    }
                    else {
                        result = dispatcher[request.method].fn();
                    }

                    if (request.hasOwnProperty('id')) {
                        if (isPromise(result)) {
                            return result.then(function (res) {
                                if (isUndefined(res)) {
                                    res = true;
                                }
                                return {
                                    "jsonrpc": "2.0",
                                    "id": request.id,
                                    "result": res
                                };
                            })
                                .catch(function (e) {
                                    return {
                                        "jsonrpc": "2.0",
                                        "id": request.id,
                                        "error": setError(ERRORS.INTERNAL_ERROR, e)
                                    };
                                });
                        }
                        else {

                            if (isUndefined(result)) {
                                result = true;
                            }

                            return _Promise.resolve({
                                "jsonrpc": "2.0",
                                "id": request.id,
                                "result": result
                            });
                        }
                    }
                    else {
                        return _Promise.resolve(); //nothing, it notification
                    }
                }
                catch (e) {
                    console.error(e)
                    return _Promise.resolve({
                        "jsonrpc": "2.0",
                        "id": request.id,
                        "error": setError(ERRORS.INTERNAL_ERROR, e)
                    });
                }
            }
            else {
                if (config.missingMethodPolicy != config.MM_PROPAGATE) {
                    if (config.missingMethodPolicy == config.MM_LOG) {
                        console.log(JSON.stringify(setError(ERRORS.METHOD_NOT_FOUND, {
                            message: request.method
                        })))
                    }
                    return _Promise.resolve({});
                }
                else {
                    return _Promise.resolve({
                        "jsonrpc": "2.0",
                        "id": request.id,
                        "error": setError(ERRORS.METHOD_NOT_FOUND, {
                            message: request.method
                        })
                    });
                }
            }
        }

        function notification(method, params) {
            var message = {
                "jsonrpc": "2.0",
                "method": method,
            };

            if (!isUndefined(params)) {
                message.params = params;
            }

            return message;
        }

        function call(method, params) {
            id += 1;
            var message = {
                "jsonrpc": "2.0",
                "method": method,
                "id": id
            };

            if (!isUndefined(params)) {
                message.params = params;
            }

            return {
                promise: new _Promise(function (resolve, reject) {
                    waitingframe[id.toString()] = {
                        resolve: resolve,
                        reject: reject
                    };
                }),
                message: message
            };
        }

        self.toStream = function (a) {
            console.log('Need define the toStream method before use');
            console.log(arguments);
        };

        self.dispatch = function (functionName, paramsNameFn, fn) {

            if (isString(functionName) && paramsNameFn == "pass" && isFunction(fn)) {
                dispatcher[functionName] = {
                    fn: fn,
                    params: paramsNameFn
                };
            }
            else if (isString(functionName) && isArray(paramsNameFn) && isFunction(fn)) {
                dispatcher[functionName] = {
                    fn: fn,
                    params: paramsNameFn
                };
            }
            else if (isString(functionName) && isFunction(paramsNameFn) && isUndefined(fn)) {
                dispatcher[functionName] = {
                    fn: paramsNameFn,
                    params: null
                };
            }
            else {
                throw new Error('Missing required argument: functionName - string, paramsNameFn - string or function');
            }
        };

        self.on = self.dispatch;

        self.off = function (functionName) {
          delete dispatcher[functionName];
        };

        self.call = function (method, params) {
            var _call = call(method, params);
            self.toStream(JSON.stringify(_call.message));
            return _call.promise;
        };

        self.notification = function (method, params) {
            self.toStream(JSON.stringify(notification(method, params)));
        };

        self.batch = function (requests) {
            var promises = [];
            var message = [];

            forEach(requests, function (req) {
                if (req.hasOwnProperty('call')) {
                    var _call = call(req.call.method, req.call.params);
                    message.push(_call.message);
                    //TODO(jershell): batch reject if one promise reject, so catch reject and resolve error as result;
                    promises.push(_call.promise.then(function (res) {
                        return res;
                    }, function (err) {
                        return err;
                    }));
                }
                else if (req.hasOwnProperty('notification')) {
                    message.push(notification(req.notification.method, req.notification.params));
                }
            });

            self.toStream(JSON.stringify(message));
            return _Promise.all(promises);
        };

        self.messageHandler = function (rawMessage) {
            try {
                var message = JSON.parse(rawMessage);
                return beforeResolve(message);
            }
            catch (e) {
                console.log("Error in messageHandler(): ", e);
                self.toStream(JSON.stringify({
                    "id": null,
                    "jsonrpc": "2.0",
                    "error": ERRORS.PARSE_ERROR
                }));
                return _Promise.reject(e);
            }
        };

        self.customException = function (code, message, data) {
            return new ServerError(code, message, data);
        };
    };

    if (typeof define == 'function' && define.amd) {
        define('simple_jsonrpc', [], function () {
            return simple_jsonrpc;
        });
    }
    else if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
        module.exports = simple_jsonrpc;
    }
    else if (typeof root !== "undefined") {
        root.simple_jsonrpc = simple_jsonrpc;
    }
    else {
        return simple_jsonrpc;
    }
})(this);
