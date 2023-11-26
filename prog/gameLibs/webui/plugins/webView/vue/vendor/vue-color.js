(function (global, factory) {
  typeof exports === 'object' && typeof module !== 'undefined' ? module.exports = factory() :
  typeof define === 'function' && define.amd ? define(factory) :
  (global.VColor = factory());
}(this, (function () { 'use strict';

  /**
   * The base implementation of `_.clamp` which doesn't coerce arguments.
   *
   * @private
   * @param {number} number The number to clamp.
   * @param {number} [lower] The lower bound.
   * @param {number} upper The upper bound.
   * @returns {number} Returns the clamped number.
   */
  function baseClamp(number, lower, upper) {
    if (number === number) {
      if (upper !== undefined) {
        number = number <= upper ? number : upper;
      }
      if (lower !== undefined) {
        number = number >= lower ? number : lower;
      }
    }
    return number;
  }

  var _baseClamp = baseClamp;

  var _baseClamp$1 = /*#__PURE__*/Object.freeze({
    default: _baseClamp,
    __moduleExports: _baseClamp
  });

  /**
   * Checks if `value` is the
   * [language type](http://www.ecma-international.org/ecma-262/7.0/#sec-ecmascript-language-types)
   * of `Object`. (e.g. arrays, functions, objects, regexes, `new Number(0)`, and `new String('')`)
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is an object, else `false`.
   * @example
   *
   * _.isObject({});
   * // => true
   *
   * _.isObject([1, 2, 3]);
   * // => true
   *
   * _.isObject(_.noop);
   * // => true
   *
   * _.isObject(null);
   * // => false
   */
  function isObject(value) {
    var type = typeof value;
    return value != null && (type == 'object' || type == 'function');
  }

  var isObject_1 = isObject;

  var isObject$1 = /*#__PURE__*/Object.freeze({
    default: isObject_1,
    __moduleExports: isObject_1
  });

  var commonjsGlobal = typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : typeof self !== 'undefined' ? self : {};

  function createCommonjsModule(fn, module) {
  	return module = { exports: {} }, fn(module, module.exports), module.exports;
  }

  /** Detect free variable `global` from Node.js. */
  var freeGlobal = typeof commonjsGlobal == 'object' && commonjsGlobal && commonjsGlobal.Object === Object && commonjsGlobal;

  var _freeGlobal = freeGlobal;

  var _freeGlobal$1 = /*#__PURE__*/Object.freeze({
    default: _freeGlobal,
    __moduleExports: _freeGlobal
  });

  var require$$0 = ( _freeGlobal$1 && _freeGlobal ) || _freeGlobal$1;

  var freeGlobal$1 = require$$0;

  /** Detect free variable `self`. */
  var freeSelf = typeof self == 'object' && self && self.Object === Object && self;

  /** Used as a reference to the global object. */
  var root = freeGlobal$1 || freeSelf || Function('return this')();

  var _root = root;

  var _root$1 = /*#__PURE__*/Object.freeze({
    default: _root,
    __moduleExports: _root
  });

  var require$$0$1 = ( _root$1 && _root ) || _root$1;

  var root$1 = require$$0$1;

  /** Built-in value references. */
  var Symbol = root$1.Symbol;

  var _Symbol = Symbol;

  var _Symbol$1 = /*#__PURE__*/Object.freeze({
    default: _Symbol,
    __moduleExports: _Symbol
  });

  var require$$0$2 = ( _Symbol$1 && _Symbol ) || _Symbol$1;

  var Symbol$1 = require$$0$2;

  /** Used for built-in method references. */
  var objectProto = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty = objectProto.hasOwnProperty;

  /**
   * Used to resolve the
   * [`toStringTag`](http://ecma-international.org/ecma-262/7.0/#sec-object.prototype.tostring)
   * of values.
   */
  var nativeObjectToString = objectProto.toString;

  /** Built-in value references. */
  var symToStringTag = Symbol$1 ? Symbol$1.toStringTag : undefined;

  /**
   * A specialized version of `baseGetTag` which ignores `Symbol.toStringTag` values.
   *
   * @private
   * @param {*} value The value to query.
   * @returns {string} Returns the raw `toStringTag`.
   */
  function getRawTag(value) {
    var isOwn = hasOwnProperty.call(value, symToStringTag),
        tag = value[symToStringTag];

    try {
      value[symToStringTag] = undefined;
    } catch (e) {}

    var result = nativeObjectToString.call(value);
    {
      if (isOwn) {
        value[symToStringTag] = tag;
      } else {
        delete value[symToStringTag];
      }
    }
    return result;
  }

  var _getRawTag = getRawTag;

  var _getRawTag$1 = /*#__PURE__*/Object.freeze({
    default: _getRawTag,
    __moduleExports: _getRawTag
  });

  /** Used for built-in method references. */
  var objectProto$1 = Object.prototype;

  /**
   * Used to resolve the
   * [`toStringTag`](http://ecma-international.org/ecma-262/7.0/#sec-object.prototype.tostring)
   * of values.
   */
  var nativeObjectToString$1 = objectProto$1.toString;

  /**
   * Converts `value` to a string using `Object.prototype.toString`.
   *
   * @private
   * @param {*} value The value to convert.
   * @returns {string} Returns the converted string.
   */
  function objectToString(value) {
    return nativeObjectToString$1.call(value);
  }

  var _objectToString = objectToString;

  var _objectToString$1 = /*#__PURE__*/Object.freeze({
    default: _objectToString,
    __moduleExports: _objectToString
  });

  var require$$1 = ( _getRawTag$1 && _getRawTag ) || _getRawTag$1;

  var require$$2 = ( _objectToString$1 && _objectToString ) || _objectToString$1;

  var Symbol$2 = require$$0$2,
      getRawTag$1 = require$$1,
      objectToString$1 = require$$2;

  /** `Object#toString` result references. */
  var nullTag = '[object Null]',
      undefinedTag = '[object Undefined]';

  /** Built-in value references. */
  var symToStringTag$1 = Symbol$2 ? Symbol$2.toStringTag : undefined;

  /**
   * The base implementation of `getTag` without fallbacks for buggy environments.
   *
   * @private
   * @param {*} value The value to query.
   * @returns {string} Returns the `toStringTag`.
   */
  function baseGetTag(value) {
    if (value == null) {
      return value === undefined ? undefinedTag : nullTag;
    }
    return (symToStringTag$1 && symToStringTag$1 in Object(value))
      ? getRawTag$1(value)
      : objectToString$1(value);
  }

  var _baseGetTag = baseGetTag;

  var _baseGetTag$1 = /*#__PURE__*/Object.freeze({
    default: _baseGetTag,
    __moduleExports: _baseGetTag
  });

  /**
   * Checks if `value` is object-like. A value is object-like if it's not `null`
   * and has a `typeof` result of "object".
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is object-like, else `false`.
   * @example
   *
   * _.isObjectLike({});
   * // => true
   *
   * _.isObjectLike([1, 2, 3]);
   * // => true
   *
   * _.isObjectLike(_.noop);
   * // => false
   *
   * _.isObjectLike(null);
   * // => false
   */
  function isObjectLike(value) {
    return value != null && typeof value == 'object';
  }

  var isObjectLike_1 = isObjectLike;

  var isObjectLike$1 = /*#__PURE__*/Object.freeze({
    default: isObjectLike_1,
    __moduleExports: isObjectLike_1
  });

  var require$$0$3 = ( _baseGetTag$1 && _baseGetTag ) || _baseGetTag$1;

  var require$$1$1 = ( isObjectLike$1 && isObjectLike_1 ) || isObjectLike$1;

  var baseGetTag$1 = require$$0$3,
      isObjectLike$2 = require$$1$1;

  /** `Object#toString` result references. */
  var symbolTag = '[object Symbol]';

  /**
   * Checks if `value` is classified as a `Symbol` primitive or object.
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a symbol, else `false`.
   * @example
   *
   * _.isSymbol(Symbol.iterator);
   * // => true
   *
   * _.isSymbol('abc');
   * // => false
   */
  function isSymbol(value) {
    return typeof value == 'symbol' ||
      (isObjectLike$2(value) && baseGetTag$1(value) == symbolTag);
  }

  var isSymbol_1 = isSymbol;

  var isSymbol$1 = /*#__PURE__*/Object.freeze({
    default: isSymbol_1,
    __moduleExports: isSymbol_1
  });

  var require$$1$2 = ( isObject$1 && isObject_1 ) || isObject$1;

  var require$$1$3 = ( isSymbol$1 && isSymbol_1 ) || isSymbol$1;

  var isObject$2 = require$$1$2,
      isSymbol$2 = require$$1$3;

  /** Used as references for various `Number` constants. */
  var NAN = 0 / 0;

  /** Used to match leading and trailing whitespace. */
  var reTrim = /^\s+|\s+$/g;

  /** Used to detect bad signed hexadecimal string values. */
  var reIsBadHex = /^[-+]0x[0-9a-f]+$/i;

  /** Used to detect binary string values. */
  var reIsBinary = /^0b[01]+$/i;

  /** Used to detect octal string values. */
  var reIsOctal = /^0o[0-7]+$/i;

  /** Built-in method references without a dependency on `root`. */
  var freeParseInt = parseInt;

  /**
   * Converts `value` to a number.
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to process.
   * @returns {number} Returns the number.
   * @example
   *
   * _.toNumber(3.2);
   * // => 3.2
   *
   * _.toNumber(Number.MIN_VALUE);
   * // => 5e-324
   *
   * _.toNumber(Infinity);
   * // => Infinity
   *
   * _.toNumber('3.2');
   * // => 3.2
   */
  function toNumber(value) {
    if (typeof value == 'number') {
      return value;
    }
    if (isSymbol$2(value)) {
      return NAN;
    }
    if (isObject$2(value)) {
      var other = typeof value.valueOf == 'function' ? value.valueOf() : value;
      value = isObject$2(other) ? (other + '') : other;
    }
    if (typeof value != 'string') {
      return value === 0 ? value : +value;
    }
    value = value.replace(reTrim, '');
    var isBinary = reIsBinary.test(value);
    return (isBinary || reIsOctal.test(value))
      ? freeParseInt(value.slice(2), isBinary ? 2 : 8)
      : (reIsBadHex.test(value) ? NAN : +value);
  }

  var toNumber_1 = toNumber;

  var toNumber$1 = /*#__PURE__*/Object.freeze({
    default: toNumber_1,
    __moduleExports: toNumber_1
  });

  var require$$0$4 = ( _baseClamp$1 && _baseClamp ) || _baseClamp$1;

  var require$$2$1 = ( toNumber$1 && toNumber_1 ) || toNumber$1;

  var baseClamp$1 = require$$0$4,
      toNumber$2 = require$$2$1;

  /**
   * Clamps `number` within the inclusive `lower` and `upper` bounds.
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Number
   * @param {number} number The number to clamp.
   * @param {number} [lower] The lower bound.
   * @param {number} upper The upper bound.
   * @returns {number} Returns the clamped number.
   * @example
   *
   * _.clamp(-10, -5, 5);
   * // => -5
   *
   * _.clamp(10, -5, 5);
   * // => 5
   */
  function clamp(number, lower, upper) {
    if (upper === undefined) {
      upper = lower;
      lower = undefined;
    }
    if (upper !== undefined) {
      upper = toNumber$2(upper);
      upper = upper === upper ? upper : 0;
    }
    if (lower !== undefined) {
      lower = toNumber$2(lower);
      lower = lower === lower ? lower : 0;
    }
    return baseClamp$1(toNumber$2(number), lower, upper);
  }

  var clamp_1 = clamp;

  var root$2 = require$$0$1;

  /**
   * Gets the timestamp of the number of milliseconds that have elapsed since
   * the Unix epoch (1 January 1970 00:00:00 UTC).
   *
   * @static
   * @memberOf _
   * @since 2.4.0
   * @category Date
   * @returns {number} Returns the timestamp.
   * @example
   *
   * _.defer(function(stamp) {
   *   console.log(_.now() - stamp);
   * }, _.now());
   * // => Logs the number of milliseconds it took for the deferred invocation.
   */
  var now = function() {
    return root$2.Date.now();
  };

  var now_1 = now;

  var now$1 = /*#__PURE__*/Object.freeze({
    default: now_1,
    __moduleExports: now_1
  });

  var require$$1$4 = ( now$1 && now_1 ) || now$1;

  var isObject$3 = require$$1$2,
      now$2 = require$$1$4,
      toNumber$3 = require$$2$1;

  /** Error message constants. */
  var FUNC_ERROR_TEXT = 'Expected a function';

  /* Built-in method references for those with the same name as other `lodash` methods. */
  var nativeMax = Math.max,
      nativeMin = Math.min;

  /**
   * Creates a debounced function that delays invoking `func` until after `wait`
   * milliseconds have elapsed since the last time the debounced function was
   * invoked. The debounced function comes with a `cancel` method to cancel
   * delayed `func` invocations and a `flush` method to immediately invoke them.
   * Provide `options` to indicate whether `func` should be invoked on the
   * leading and/or trailing edge of the `wait` timeout. The `func` is invoked
   * with the last arguments provided to the debounced function. Subsequent
   * calls to the debounced function return the result of the last `func`
   * invocation.
   *
   * **Note:** If `leading` and `trailing` options are `true`, `func` is
   * invoked on the trailing edge of the timeout only if the debounced function
   * is invoked more than once during the `wait` timeout.
   *
   * If `wait` is `0` and `leading` is `false`, `func` invocation is deferred
   * until to the next tick, similar to `setTimeout` with a timeout of `0`.
   *
   * See [David Corbacho's article](https://css-tricks.com/debouncing-throttling-explained-examples/)
   * for details over the differences between `_.debounce` and `_.throttle`.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Function
   * @param {Function} func The function to debounce.
   * @param {number} [wait=0] The number of milliseconds to delay.
   * @param {Object} [options={}] The options object.
   * @param {boolean} [options.leading=false]
   *  Specify invoking on the leading edge of the timeout.
   * @param {number} [options.maxWait]
   *  The maximum time `func` is allowed to be delayed before it's invoked.
   * @param {boolean} [options.trailing=true]
   *  Specify invoking on the trailing edge of the timeout.
   * @returns {Function} Returns the new debounced function.
   * @example
   *
   * // Avoid costly calculations while the window size is in flux.
   * jQuery(window).on('resize', _.debounce(calculateLayout, 150));
   *
   * // Invoke `sendMail` when clicked, debouncing subsequent calls.
   * jQuery(element).on('click', _.debounce(sendMail, 300, {
   *   'leading': true,
   *   'trailing': false
   * }));
   *
   * // Ensure `batchLog` is invoked once after 1 second of debounced calls.
   * var debounced = _.debounce(batchLog, 250, { 'maxWait': 1000 });
   * var source = new EventSource('/stream');
   * jQuery(source).on('message', debounced);
   *
   * // Cancel the trailing debounced invocation.
   * jQuery(window).on('popstate', debounced.cancel);
   */
  function debounce(func, wait, options) {
    var lastArgs,
        lastThis,
        maxWait,
        result,
        timerId,
        lastCallTime,
        lastInvokeTime = 0,
        leading = false,
        maxing = false,
        trailing = true;

    if (typeof func != 'function') {
      throw new TypeError(FUNC_ERROR_TEXT);
    }
    wait = toNumber$3(wait) || 0;
    if (isObject$3(options)) {
      leading = !!options.leading;
      maxing = 'maxWait' in options;
      maxWait = maxing ? nativeMax(toNumber$3(options.maxWait) || 0, wait) : maxWait;
      trailing = 'trailing' in options ? !!options.trailing : trailing;
    }

    function invokeFunc(time) {
      var args = lastArgs,
          thisArg = lastThis;

      lastArgs = lastThis = undefined;
      lastInvokeTime = time;
      result = func.apply(thisArg, args);
      return result;
    }

    function leadingEdge(time) {
      // Reset any `maxWait` timer.
      lastInvokeTime = time;
      // Start the timer for the trailing edge.
      timerId = setTimeout(timerExpired, wait);
      // Invoke the leading edge.
      return leading ? invokeFunc(time) : result;
    }

    function remainingWait(time) {
      var timeSinceLastCall = time - lastCallTime,
          timeSinceLastInvoke = time - lastInvokeTime,
          timeWaiting = wait - timeSinceLastCall;

      return maxing
        ? nativeMin(timeWaiting, maxWait - timeSinceLastInvoke)
        : timeWaiting;
    }

    function shouldInvoke(time) {
      var timeSinceLastCall = time - lastCallTime,
          timeSinceLastInvoke = time - lastInvokeTime;

      // Either this is the first call, activity has stopped and we're at the
      // trailing edge, the system time has gone backwards and we're treating
      // it as the trailing edge, or we've hit the `maxWait` limit.
      return (lastCallTime === undefined || (timeSinceLastCall >= wait) ||
        (timeSinceLastCall < 0) || (maxing && timeSinceLastInvoke >= maxWait));
    }

    function timerExpired() {
      var time = now$2();
      if (shouldInvoke(time)) {
        return trailingEdge(time);
      }
      // Restart the timer.
      timerId = setTimeout(timerExpired, remainingWait(time));
    }

    function trailingEdge(time) {
      timerId = undefined;

      // Only invoke if we have `lastArgs` which means `func` has been
      // debounced at least once.
      if (trailing && lastArgs) {
        return invokeFunc(time);
      }
      lastArgs = lastThis = undefined;
      return result;
    }

    function cancel() {
      if (timerId !== undefined) {
        clearTimeout(timerId);
      }
      lastInvokeTime = 0;
      lastArgs = lastCallTime = lastThis = timerId = undefined;
    }

    function flush() {
      return timerId === undefined ? result : trailingEdge(now$2());
    }

    function debounced() {
      var time = now$2(),
          isInvoking = shouldInvoke(time);

      lastArgs = arguments;
      lastThis = this;
      lastCallTime = time;

      if (isInvoking) {
        if (timerId === undefined) {
          return leadingEdge(lastCallTime);
        }
        if (maxing) {
          // Handle invocations in a tight loop.
          timerId = setTimeout(timerExpired, wait);
          return invokeFunc(lastCallTime);
        }
      }
      if (timerId === undefined) {
        timerId = setTimeout(timerExpired, wait);
      }
      return result;
    }
    debounced.cancel = cancel;
    debounced.flush = flush;
    return debounced;
  }

  var debounce_1 = debounce;

  /**
   * Copyright (c) 2013-present, Facebook, Inc.
   *
   * This source code is licensed under the MIT license found in the
   * LICENSE file in the root directory of this source tree.
   */

  /**
   * Use invariant() to assert state which your program assumes to be true.
   *
   * Provide sprintf-style format (only %s is supported) and arguments
   * to provide information about what broke and what you were
   * expecting.
   *
   * The invariant message will be stripped in production, but the invariant
   * will remain to ensure logic does not differ in production.
   */

  var invariant = function(condition, format, a, b, c, d, e, f) {

    if (!condition) {
      var error;
      if (format === undefined) {
        error = new Error(
          'Minified exception occurred; use the non-minified dev environment ' +
          'for the full error message and additional helpful warnings.'
        );
      } else {
        var args = [a, b, c, d, e, f];
        var argIndex = 0;
        error = new Error(
          format.replace(/%s/g, function() { return args[argIndex++]; })
        );
        error.name = 'Invariant Violation';
      }

      error.framesToPop = 1; // we don't care about invariant's own frame
      throw error;
    }
  };

  var browser = invariant;

  /*
  object-assign
  (c) Sindre Sorhus
  @license MIT
  */
  /* eslint-disable no-unused-vars */
  var getOwnPropertySymbols = Object.getOwnPropertySymbols;
  var hasOwnProperty$1 = Object.prototype.hasOwnProperty;
  var propIsEnumerable = Object.prototype.propertyIsEnumerable;

  function toObject(val) {
  	if (val === null || val === undefined) {
  		throw new TypeError('Object.assign cannot be called with null or undefined');
  	}

  	return Object(val);
  }

  function shouldUseNative() {
  	try {
  		if (!Object.assign) {
  			return false;
  		}

  		// Detect buggy property enumeration order in older V8 versions.

  		// https://bugs.chromium.org/p/v8/issues/detail?id=4118
  		var test1 = new String('abc');  // eslint-disable-line no-new-wrappers
  		test1[5] = 'de';
  		if (Object.getOwnPropertyNames(test1)[0] === '5') {
  			return false;
  		}

  		// https://bugs.chromium.org/p/v8/issues/detail?id=3056
  		var test2 = {};
  		for (var i = 0; i < 10; i++) {
  			test2['_' + String.fromCharCode(i)] = i;
  		}
  		var order2 = Object.getOwnPropertyNames(test2).map(function (n) {
  			return test2[n];
  		});
  		if (order2.join('') !== '0123456789') {
  			return false;
  		}

  		// https://bugs.chromium.org/p/v8/issues/detail?id=3056
  		var test3 = {};
  		'abcdefghijklmnopqrst'.split('').forEach(function (letter) {
  			test3[letter] = letter;
  		});
  		if (Object.keys(Object.assign({}, test3)).join('') !==
  				'abcdefghijklmnopqrst') {
  			return false;
  		}

  		return true;
  	} catch (err) {
  		// We don't expect any of the above to throw, but better to be safe.
  		return false;
  	}
  }

  var index = shouldUseNative() ? Object.assign : function (target, source) {
  	var arguments$1 = arguments;

  	var from;
  	var to = toObject(target);
  	var symbols;

  	for (var s = 1; s < arguments.length; s++) {
  		from = Object(arguments$1[s]);

  		for (var key in from) {
  			if (hasOwnProperty$1.call(from, key)) {
  				to[key] = from[key];
  			}
  		}

  		if (getOwnPropertySymbols) {
  			symbols = getOwnPropertySymbols(from);
  			for (var i = 0; i < symbols.length; i++) {
  				if (propIsEnumerable.call(from, symbols[i])) {
  					to[symbols[i]] = from[symbols[i]];
  				}
  			}
  		}
  	}

  	return to;
  };

  var component = /-?\d+(\.\d+)?%?/g;
  function extractComponents(color) {
    return color.match(component);
  }

  var extractComponents_1 = extractComponents;

  function clamp$1(val, min, max) {
    return Math.min(Math.max(val, min), max);
  }

  var clamp_1$1 = clamp$1;

  var extractComponents$1 = extractComponents_1;
  var clamp$2 = clamp_1$1;

  function parseHslComponent(component, i) {
    component = parseFloat(component);

    switch(i) {
      case 0:
        return clamp$2(component, 0, 360);
      case 1:
      case 2:
        return clamp$2(component, 0, 100);
      case 3:
        return clamp$2(component, 0, 1);
    }
  }

  function hsl(color) {
    return extractComponents$1(color).map(parseHslComponent);
  }

  var hsl_1 = hsl;

  function expand(hex) {
    var result = "#";

    for (var i = 1; i < hex.length; i++) {
      var val = hex.charAt(i);
      result += val + val;
    }

    return result;
  }

  function hex(hex) {
    // #RGB or #RGBA
    if(hex.length === 4 || hex.length === 5) {
      hex = expand(hex);
    }

    var rgb = [
      parseInt(hex.substring(1,3), 16),
      parseInt(hex.substring(3,5), 16),
      parseInt(hex.substring(5,7), 16)
    ];

    // #RRGGBBAA
    if (hex.length === 9) {
      var alpha = parseFloat((parseInt(hex.substring(7,9), 16) / 255).toFixed(2));
      rgb.push(alpha);
    }

    return rgb;
  }

  var hex_1 = hex;

  var extractComponents$2 = extractComponents_1;
  var clamp$3 = clamp_1$1;

  function parseRgbComponent(component, i) {
    if (i < 3) {
      if (component.indexOf('%') != -1) {
        return Math.round(255 * clamp$3(parseInt(component, 10), 0, 100)/100);
      } else {
        return clamp$3(parseInt(component, 10), 0, 255);
      }
    } else {
      return clamp$3(parseFloat(component), 0, 1);
    } 
  }

  function rgb(color) {
    return extractComponents$2(color).map(parseRgbComponent);
  }

  var rgb_1 = rgb;

  function hsl2rgb(hsl) {
    var h = hsl[0] / 360,
        s = hsl[1] / 100,
        l = hsl[2] / 100,
        t1, t2, t3, rgb, val;

    if (s == 0) {
      val = l * 255;
      return [val, val, val];
    }

    if (l < 0.5)
      { t2 = l * (1 + s); }
    else
      { t2 = l + s - l * s; }
    t1 = 2 * l - t2;

    rgb = [0, 0, 0];
    for (var i = 0; i < 3; i++) {
      t3 = h + 1 / 3 * - (i - 1);
      t3 < 0 && t3++;
      t3 > 1 && t3--;

      if (6 * t3 < 1)
        { val = t1 + (t2 - t1) * 6 * t3; }
      else if (2 * t3 < 1)
        { val = t2; }
      else if (3 * t3 < 2)
        { val = t1 + (t2 - t1) * (2 / 3 - t3) * 6; }
      else
        { val = t1; }

      rgb[i] = val * 255;
    }

    return rgb;
  }

  var hsl2rgb_1 = hsl2rgb;

  var hsl$1 = hsl_1;
  var hex$1 = hex_1;
  var rgb$1 = rgb_1;
  var hsl2rgb$1 = hsl2rgb_1;

  function hsl2rgbParse(color) {
    var h = hsl$1(color);
    var r = hsl2rgb$1(h);

    // handle alpha since hsl2rgb doesn't know (or care!) about it
    if(h.length === 4) {
      r.push(h[3]);
    }

    return r;
  }

  var space2parser = {
    "#" : hex$1,
    "hsl" : hsl2rgbParse,
    "rgb" : rgb$1
  };

  function parse(color) {
    for(var scheme in space2parser) {
      if(color.indexOf(scheme) === 0) {
        return space2parser[scheme](color);
      }
    }
  }

  parse.rgb = rgb$1;
  parse.hsl = hsl$1;
  parse.hex = hex$1;

  var index$1 = parse;

  function rgb2hsv(rgb) {
    var r = rgb[0],
        g = rgb[1],
        b = rgb[2],
        min = Math.min(r, g, b),
        max = Math.max(r, g, b),
        delta = max - min,
        h, s, v;

    if (max == 0)
      { s = 0; }
    else
      { s = (delta/max * 1000)/10; }

    if (max == min)
      { h = 0; }
    else if (r == max)
      { h = (g - b) / delta; }
    else if (g == max)
      { h = 2 + (b - r) / delta; }
    else if (b == max)
      { h = 4 + (r - g) / delta; }

    h = Math.min(h * 60, 360);

    if (h < 0)
      { h += 360; }

    v = ((max / 255) * 1000) / 10;

    return [h, s, v];
  }

  var rgb2hsv_1 = rgb2hsv;

  var clamp$4 = clamp_1$1;

  function componentToHex(c) {
    var value = Math.round(clamp$4(c, 0, 255));
    var hex   = value.toString(16);

    return hex.length == 1 ? "0" + hex : hex;
  }

  function rgb2hex(rgb) {
    var alpha = rgb.length === 4 ? componentToHex(rgb[3] * 255) : "";

    return "#" + componentToHex(rgb[0]) + componentToHex(rgb[1]) + componentToHex(rgb[2]) + alpha;
  }

  var rgb2hex_1 = rgb2hex;

  function hsv2hsl(hsv) {
    var h = hsv[0],
        s = hsv[1] / 100,
        v = hsv[2] / 100,
        sl, l;

    l = (2 - s) * v;
    sl = s * v;
    sl /= (l <= 1) ? l : 2 - l;
    sl = sl || 0;
    l /= 2;
    return [h, sl * 100, l * 100];
  }

  var hsv2hsl_1 = hsv2hsl;

  function hsv2rgb(hsv) {
    var h = hsv[0] / 60,
        s = hsv[1] / 100,
        v = hsv[2] / 100,
        hi = Math.floor(h) % 6;

    var f = h - Math.floor(h),
        p = 255 * v * (1 - s),
        q = 255 * v * (1 - (s * f)),
        t = 255 * v * (1 - (s * (1 - f))),
        v = 255 * v;

    switch(hi) {
      case 0:
        return [v, t, p];
      case 1:
        return [q, v, p];
      case 2:
        return [p, v, t];
      case 3:
        return [p, q, v];
      case 4:
        return [t, p, v];
      case 5:
        return [v, p, q];
    }
  }

  var hsv2rgb_1 = hsv2rgb;

  var debounce$1 = debounce_1,
      isObject$4 = require$$1$2;

  /** Error message constants. */
  var FUNC_ERROR_TEXT$1 = 'Expected a function';

  /**
   * Creates a throttled function that only invokes `func` at most once per
   * every `wait` milliseconds. The throttled function comes with a `cancel`
   * method to cancel delayed `func` invocations and a `flush` method to
   * immediately invoke them. Provide `options` to indicate whether `func`
   * should be invoked on the leading and/or trailing edge of the `wait`
   * timeout. The `func` is invoked with the last arguments provided to the
   * throttled function. Subsequent calls to the throttled function return the
   * result of the last `func` invocation.
   *
   * **Note:** If `leading` and `trailing` options are `true`, `func` is
   * invoked on the trailing edge of the timeout only if the throttled function
   * is invoked more than once during the `wait` timeout.
   *
   * If `wait` is `0` and `leading` is `false`, `func` invocation is deferred
   * until to the next tick, similar to `setTimeout` with a timeout of `0`.
   *
   * See [David Corbacho's article](https://css-tricks.com/debouncing-throttling-explained-examples/)
   * for details over the differences between `_.throttle` and `_.debounce`.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Function
   * @param {Function} func The function to throttle.
   * @param {number} [wait=0] The number of milliseconds to throttle invocations to.
   * @param {Object} [options={}] The options object.
   * @param {boolean} [options.leading=true]
   *  Specify invoking on the leading edge of the timeout.
   * @param {boolean} [options.trailing=true]
   *  Specify invoking on the trailing edge of the timeout.
   * @returns {Function} Returns the new throttled function.
   * @example
   *
   * // Avoid excessively updating the position while scrolling.
   * jQuery(window).on('scroll', _.throttle(updatePosition, 100));
   *
   * // Invoke `renewToken` when the click event is fired, but not more than once every 5 minutes.
   * var throttled = _.throttle(renewToken, 300000, { 'trailing': false });
   * jQuery(element).on('click', throttled);
   *
   * // Cancel the trailing throttled invocation.
   * jQuery(window).on('popstate', throttled.cancel);
   */
  function throttle(func, wait, options) {
    var leading = true,
        trailing = true;

    if (typeof func != 'function') {
      throw new TypeError(FUNC_ERROR_TEXT$1);
    }
    if (isObject$4(options)) {
      leading = 'leading' in options ? !!options.leading : leading;
      trailing = 'trailing' in options ? !!options.trailing : trailing;
    }
    return debounce$1(func, wait, {
      'leading': leading,
      'maxWait': wait,
      'trailing': trailing
    });
  }

  var throttle_1 = throttle;

  /**
   * Removes all key-value entries from the list cache.
   *
   * @private
   * @name clear
   * @memberOf ListCache
   */
  function listCacheClear() {
    this.__data__ = [];
    this.size = 0;
  }

  var _listCacheClear = listCacheClear;

  var _listCacheClear$1 = /*#__PURE__*/Object.freeze({
    default: _listCacheClear,
    __moduleExports: _listCacheClear
  });

  /**
   * Performs a
   * [`SameValueZero`](http://ecma-international.org/ecma-262/7.0/#sec-samevaluezero)
   * comparison between two values to determine if they are equivalent.
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to compare.
   * @param {*} other The other value to compare.
   * @returns {boolean} Returns `true` if the values are equivalent, else `false`.
   * @example
   *
   * var object = { 'a': 1 };
   * var other = { 'a': 1 };
   *
   * _.eq(object, object);
   * // => true
   *
   * _.eq(object, other);
   * // => false
   *
   * _.eq('a', 'a');
   * // => true
   *
   * _.eq('a', Object('a'));
   * // => false
   *
   * _.eq(NaN, NaN);
   * // => true
   */
  function eq(value, other) {
    return value === other || (value !== value && other !== other);
  }

  var eq_1 = eq;

  var eq$1 = /*#__PURE__*/Object.freeze({
    default: eq_1,
    __moduleExports: eq_1
  });

  var require$$0$5 = ( eq$1 && eq_1 ) || eq$1;

  var eq$2 = require$$0$5;

  /**
   * Gets the index at which the `key` is found in `array` of key-value pairs.
   *
   * @private
   * @param {Array} array The array to inspect.
   * @param {*} key The key to search for.
   * @returns {number} Returns the index of the matched value, else `-1`.
   */
  function assocIndexOf(array, key) {
    var length = array.length;
    while (length--) {
      if (eq$2(array[length][0], key)) {
        return length;
      }
    }
    return -1;
  }

  var _assocIndexOf = assocIndexOf;

  var _assocIndexOf$1 = /*#__PURE__*/Object.freeze({
    default: _assocIndexOf,
    __moduleExports: _assocIndexOf
  });

  var require$$0$6 = ( _assocIndexOf$1 && _assocIndexOf ) || _assocIndexOf$1;

  var assocIndexOf$1 = require$$0$6;

  /** Used for built-in method references. */
  var arrayProto = Array.prototype;

  /** Built-in value references. */
  var splice = arrayProto.splice;

  /**
   * Removes `key` and its value from the list cache.
   *
   * @private
   * @name delete
   * @memberOf ListCache
   * @param {string} key The key of the value to remove.
   * @returns {boolean} Returns `true` if the entry was removed, else `false`.
   */
  function listCacheDelete(key) {
    var data = this.__data__,
        index = assocIndexOf$1(data, key);

    if (index < 0) {
      return false;
    }
    var lastIndex = data.length - 1;
    if (index == lastIndex) {
      data.pop();
    } else {
      splice.call(data, index, 1);
    }
    --this.size;
    return true;
  }

  var _listCacheDelete = listCacheDelete;

  var _listCacheDelete$1 = /*#__PURE__*/Object.freeze({
    default: _listCacheDelete,
    __moduleExports: _listCacheDelete
  });

  var assocIndexOf$2 = require$$0$6;

  /**
   * Gets the list cache value for `key`.
   *
   * @private
   * @name get
   * @memberOf ListCache
   * @param {string} key The key of the value to get.
   * @returns {*} Returns the entry value.
   */
  function listCacheGet(key) {
    var data = this.__data__,
        index = assocIndexOf$2(data, key);

    return index < 0 ? undefined : data[index][1];
  }

  var _listCacheGet = listCacheGet;

  var _listCacheGet$1 = /*#__PURE__*/Object.freeze({
    default: _listCacheGet,
    __moduleExports: _listCacheGet
  });

  var assocIndexOf$3 = require$$0$6;

  /**
   * Checks if a list cache value for `key` exists.
   *
   * @private
   * @name has
   * @memberOf ListCache
   * @param {string} key The key of the entry to check.
   * @returns {boolean} Returns `true` if an entry for `key` exists, else `false`.
   */
  function listCacheHas(key) {
    return assocIndexOf$3(this.__data__, key) > -1;
  }

  var _listCacheHas = listCacheHas;

  var _listCacheHas$1 = /*#__PURE__*/Object.freeze({
    default: _listCacheHas,
    __moduleExports: _listCacheHas
  });

  var assocIndexOf$4 = require$$0$6;

  /**
   * Sets the list cache `key` to `value`.
   *
   * @private
   * @name set
   * @memberOf ListCache
   * @param {string} key The key of the value to set.
   * @param {*} value The value to set.
   * @returns {Object} Returns the list cache instance.
   */
  function listCacheSet(key, value) {
    var data = this.__data__,
        index = assocIndexOf$4(data, key);

    if (index < 0) {
      ++this.size;
      data.push([key, value]);
    } else {
      data[index][1] = value;
    }
    return this;
  }

  var _listCacheSet = listCacheSet;

  var _listCacheSet$1 = /*#__PURE__*/Object.freeze({
    default: _listCacheSet,
    __moduleExports: _listCacheSet
  });

  var require$$0$7 = ( _listCacheClear$1 && _listCacheClear ) || _listCacheClear$1;

  var require$$1$5 = ( _listCacheDelete$1 && _listCacheDelete ) || _listCacheDelete$1;

  var require$$2$2 = ( _listCacheGet$1 && _listCacheGet ) || _listCacheGet$1;

  var require$$3 = ( _listCacheHas$1 && _listCacheHas ) || _listCacheHas$1;

  var require$$4 = ( _listCacheSet$1 && _listCacheSet ) || _listCacheSet$1;

  var listCacheClear$1 = require$$0$7,
      listCacheDelete$1 = require$$1$5,
      listCacheGet$1 = require$$2$2,
      listCacheHas$1 = require$$3,
      listCacheSet$1 = require$$4;

  /**
   * Creates an list cache object.
   *
   * @private
   * @constructor
   * @param {Array} [entries] The key-value pairs to cache.
   */
  function ListCache(entries) {
    var this$1 = this;

    var index = -1,
        length = entries == null ? 0 : entries.length;

    this.clear();
    while (++index < length) {
      var entry = entries[index];
      this$1.set(entry[0], entry[1]);
    }
  }

  // Add methods to `ListCache`.
  ListCache.prototype.clear = listCacheClear$1;
  ListCache.prototype['delete'] = listCacheDelete$1;
  ListCache.prototype.get = listCacheGet$1;
  ListCache.prototype.has = listCacheHas$1;
  ListCache.prototype.set = listCacheSet$1;

  var _ListCache = ListCache;

  var _ListCache$1 = /*#__PURE__*/Object.freeze({
    default: _ListCache,
    __moduleExports: _ListCache
  });

  var require$$1$6 = ( _ListCache$1 && _ListCache ) || _ListCache$1;

  var ListCache$1 = require$$1$6;

  /**
   * Removes all key-value entries from the stack.
   *
   * @private
   * @name clear
   * @memberOf Stack
   */
  function stackClear() {
    this.__data__ = new ListCache$1;
    this.size = 0;
  }

  var _stackClear = stackClear;

  var _stackClear$1 = /*#__PURE__*/Object.freeze({
    default: _stackClear,
    __moduleExports: _stackClear
  });

  /**
   * Removes `key` and its value from the stack.
   *
   * @private
   * @name delete
   * @memberOf Stack
   * @param {string} key The key of the value to remove.
   * @returns {boolean} Returns `true` if the entry was removed, else `false`.
   */
  function stackDelete(key) {
    var data = this.__data__,
        result = data['delete'](key);

    this.size = data.size;
    return result;
  }

  var _stackDelete = stackDelete;

  var _stackDelete$1 = /*#__PURE__*/Object.freeze({
    default: _stackDelete,
    __moduleExports: _stackDelete
  });

  /**
   * Gets the stack value for `key`.
   *
   * @private
   * @name get
   * @memberOf Stack
   * @param {string} key The key of the value to get.
   * @returns {*} Returns the entry value.
   */
  function stackGet(key) {
    return this.__data__.get(key);
  }

  var _stackGet = stackGet;

  var _stackGet$1 = /*#__PURE__*/Object.freeze({
    default: _stackGet,
    __moduleExports: _stackGet
  });

  /**
   * Checks if a stack value for `key` exists.
   *
   * @private
   * @name has
   * @memberOf Stack
   * @param {string} key The key of the entry to check.
   * @returns {boolean} Returns `true` if an entry for `key` exists, else `false`.
   */
  function stackHas(key) {
    return this.__data__.has(key);
  }

  var _stackHas = stackHas;

  var _stackHas$1 = /*#__PURE__*/Object.freeze({
    default: _stackHas,
    __moduleExports: _stackHas
  });

  var baseGetTag$2 = require$$0$3,
      isObject$5 = require$$1$2;

  /** `Object#toString` result references. */
  var asyncTag = '[object AsyncFunction]',
      funcTag = '[object Function]',
      genTag = '[object GeneratorFunction]',
      proxyTag = '[object Proxy]';

  /**
   * Checks if `value` is classified as a `Function` object.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a function, else `false`.
   * @example
   *
   * _.isFunction(_);
   * // => true
   *
   * _.isFunction(/abc/);
   * // => false
   */
  function isFunction(value) {
    if (!isObject$5(value)) {
      return false;
    }
    // The use of `Object#toString` avoids issues with the `typeof` operator
    // in Safari 9 which returns 'object' for typed arrays and other constructors.
    var tag = baseGetTag$2(value);
    return tag == funcTag || tag == genTag || tag == asyncTag || tag == proxyTag;
  }

  var isFunction_1 = isFunction;

  var isFunction$1 = /*#__PURE__*/Object.freeze({
    default: isFunction_1,
    __moduleExports: isFunction_1
  });

  var root$3 = require$$0$1;

  /** Used to detect overreaching core-js shims. */
  var coreJsData = root$3['__core-js_shared__'];

  var _coreJsData = coreJsData;

  var _coreJsData$1 = /*#__PURE__*/Object.freeze({
    default: _coreJsData,
    __moduleExports: _coreJsData
  });

  var require$$0$8 = ( _coreJsData$1 && _coreJsData ) || _coreJsData$1;

  var coreJsData$1 = require$$0$8;

  /** Used to detect methods masquerading as native. */
  var maskSrcKey = (function() {
    var uid = /[^.]+$/.exec(coreJsData$1 && coreJsData$1.keys && coreJsData$1.keys.IE_PROTO || '');
    return uid ? ('Symbol(src)_1.' + uid) : '';
  }());

  /**
   * Checks if `func` has its source masked.
   *
   * @private
   * @param {Function} func The function to check.
   * @returns {boolean} Returns `true` if `func` is masked, else `false`.
   */
  function isMasked(func) {
    return !!maskSrcKey && (maskSrcKey in func);
  }

  var _isMasked = isMasked;

  var _isMasked$1 = /*#__PURE__*/Object.freeze({
    default: _isMasked,
    __moduleExports: _isMasked
  });

  /** Used for built-in method references. */
  var funcProto = Function.prototype;

  /** Used to resolve the decompiled source of functions. */
  var funcToString = funcProto.toString;

  /**
   * Converts `func` to its source code.
   *
   * @private
   * @param {Function} func The function to convert.
   * @returns {string} Returns the source code.
   */
  function toSource(func) {
    if (func != null) {
      try {
        return funcToString.call(func);
      } catch (e) {}
      try {
        return (func + '');
      } catch (e) {}
    }
    return '';
  }

  var _toSource = toSource;

  var _toSource$1 = /*#__PURE__*/Object.freeze({
    default: _toSource,
    __moduleExports: _toSource
  });

  var require$$0$9 = ( isFunction$1 && isFunction_1 ) || isFunction$1;

  var require$$1$7 = ( _isMasked$1 && _isMasked ) || _isMasked$1;

  var require$$3$1 = ( _toSource$1 && _toSource ) || _toSource$1;

  var isFunction$2 = require$$0$9,
      isMasked$1 = require$$1$7,
      isObject$6 = require$$1$2,
      toSource$1 = require$$3$1;

  /**
   * Used to match `RegExp`
   * [syntax characters](http://ecma-international.org/ecma-262/7.0/#sec-patterns).
   */
  var reRegExpChar = /[\\^$.*+?()[\]{}|]/g;

  /** Used to detect host constructors (Safari). */
  var reIsHostCtor = /^\[object .+?Constructor\]$/;

  /** Used for built-in method references. */
  var funcProto$1 = Function.prototype,
      objectProto$2 = Object.prototype;

  /** Used to resolve the decompiled source of functions. */
  var funcToString$1 = funcProto$1.toString;

  /** Used to check objects for own properties. */
  var hasOwnProperty$2 = objectProto$2.hasOwnProperty;

  /** Used to detect if a method is native. */
  var reIsNative = RegExp('^' +
    funcToString$1.call(hasOwnProperty$2).replace(reRegExpChar, '\\$&')
    .replace(/hasOwnProperty|(function).*?(?=\\\()| for .+?(?=\\\])/g, '$1.*?') + '$'
  );

  /**
   * The base implementation of `_.isNative` without bad shim checks.
   *
   * @private
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a native function,
   *  else `false`.
   */
  function baseIsNative(value) {
    if (!isObject$6(value) || isMasked$1(value)) {
      return false;
    }
    var pattern = isFunction$2(value) ? reIsNative : reIsHostCtor;
    return pattern.test(toSource$1(value));
  }

  var _baseIsNative = baseIsNative;

  var _baseIsNative$1 = /*#__PURE__*/Object.freeze({
    default: _baseIsNative,
    __moduleExports: _baseIsNative
  });

  /**
   * Gets the value at `key` of `object`.
   *
   * @private
   * @param {Object} [object] The object to query.
   * @param {string} key The key of the property to get.
   * @returns {*} Returns the property value.
   */
  function getValue(object, key) {
    return object == null ? undefined : object[key];
  }

  var _getValue = getValue;

  var _getValue$1 = /*#__PURE__*/Object.freeze({
    default: _getValue,
    __moduleExports: _getValue
  });

  var require$$0$a = ( _baseIsNative$1 && _baseIsNative ) || _baseIsNative$1;

  var require$$1$8 = ( _getValue$1 && _getValue ) || _getValue$1;

  var baseIsNative$1 = require$$0$a,
      getValue$1 = require$$1$8;

  /**
   * Gets the native function at `key` of `object`.
   *
   * @private
   * @param {Object} object The object to query.
   * @param {string} key The key of the method to get.
   * @returns {*} Returns the function if it's native, else `undefined`.
   */
  function getNative(object, key) {
    var value = getValue$1(object, key);
    return baseIsNative$1(value) ? value : undefined;
  }

  var _getNative = getNative;

  var _getNative$1 = /*#__PURE__*/Object.freeze({
    default: _getNative,
    __moduleExports: _getNative
  });

  var require$$0$b = ( _getNative$1 && _getNative ) || _getNative$1;

  var getNative$1 = require$$0$b,
      root$4 = require$$0$1;

  /* Built-in method references that are verified to be native. */
  var Map = getNative$1(root$4, 'Map');

  var _Map = Map;

  var _Map$1 = /*#__PURE__*/Object.freeze({
    default: _Map,
    __moduleExports: _Map
  });

  var getNative$2 = require$$0$b;

  /* Built-in method references that are verified to be native. */
  var nativeCreate = getNative$2(Object, 'create');

  var _nativeCreate = nativeCreate;

  var _nativeCreate$1 = /*#__PURE__*/Object.freeze({
    default: _nativeCreate,
    __moduleExports: _nativeCreate
  });

  var require$$0$c = ( _nativeCreate$1 && _nativeCreate ) || _nativeCreate$1;

  var nativeCreate$1 = require$$0$c;

  /**
   * Removes all key-value entries from the hash.
   *
   * @private
   * @name clear
   * @memberOf Hash
   */
  function hashClear() {
    this.__data__ = nativeCreate$1 ? nativeCreate$1(null) : {};
    this.size = 0;
  }

  var _hashClear = hashClear;

  var _hashClear$1 = /*#__PURE__*/Object.freeze({
    default: _hashClear,
    __moduleExports: _hashClear
  });

  /**
   * Removes `key` and its value from the hash.
   *
   * @private
   * @name delete
   * @memberOf Hash
   * @param {Object} hash The hash to modify.
   * @param {string} key The key of the value to remove.
   * @returns {boolean} Returns `true` if the entry was removed, else `false`.
   */
  function hashDelete(key) {
    var result = this.has(key) && delete this.__data__[key];
    this.size -= result ? 1 : 0;
    return result;
  }

  var _hashDelete = hashDelete;

  var _hashDelete$1 = /*#__PURE__*/Object.freeze({
    default: _hashDelete,
    __moduleExports: _hashDelete
  });

  var nativeCreate$2 = require$$0$c;

  /** Used to stand-in for `undefined` hash values. */
  var HASH_UNDEFINED = '__lodash_hash_undefined__';

  /** Used for built-in method references. */
  var objectProto$3 = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$3 = objectProto$3.hasOwnProperty;

  /**
   * Gets the hash value for `key`.
   *
   * @private
   * @name get
   * @memberOf Hash
   * @param {string} key The key of the value to get.
   * @returns {*} Returns the entry value.
   */
  function hashGet(key) {
    var data = this.__data__;
    if (nativeCreate$2) {
      var result = data[key];
      return result === HASH_UNDEFINED ? undefined : result;
    }
    return hasOwnProperty$3.call(data, key) ? data[key] : undefined;
  }

  var _hashGet = hashGet;

  var _hashGet$1 = /*#__PURE__*/Object.freeze({
    default: _hashGet,
    __moduleExports: _hashGet
  });

  var nativeCreate$3 = require$$0$c;

  /** Used for built-in method references. */
  var objectProto$4 = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$4 = objectProto$4.hasOwnProperty;

  /**
   * Checks if a hash value for `key` exists.
   *
   * @private
   * @name has
   * @memberOf Hash
   * @param {string} key The key of the entry to check.
   * @returns {boolean} Returns `true` if an entry for `key` exists, else `false`.
   */
  function hashHas(key) {
    var data = this.__data__;
    return nativeCreate$3 ? (data[key] !== undefined) : hasOwnProperty$4.call(data, key);
  }

  var _hashHas = hashHas;

  var _hashHas$1 = /*#__PURE__*/Object.freeze({
    default: _hashHas,
    __moduleExports: _hashHas
  });

  var nativeCreate$4 = require$$0$c;

  /** Used to stand-in for `undefined` hash values. */
  var HASH_UNDEFINED$1 = '__lodash_hash_undefined__';

  /**
   * Sets the hash `key` to `value`.
   *
   * @private
   * @name set
   * @memberOf Hash
   * @param {string} key The key of the value to set.
   * @param {*} value The value to set.
   * @returns {Object} Returns the hash instance.
   */
  function hashSet(key, value) {
    var data = this.__data__;
    this.size += this.has(key) ? 0 : 1;
    data[key] = (nativeCreate$4 && value === undefined) ? HASH_UNDEFINED$1 : value;
    return this;
  }

  var _hashSet = hashSet;

  var _hashSet$1 = /*#__PURE__*/Object.freeze({
    default: _hashSet,
    __moduleExports: _hashSet
  });

  var require$$0$d = ( _hashClear$1 && _hashClear ) || _hashClear$1;

  var require$$1$9 = ( _hashDelete$1 && _hashDelete ) || _hashDelete$1;

  var require$$2$3 = ( _hashGet$1 && _hashGet ) || _hashGet$1;

  var require$$3$2 = ( _hashHas$1 && _hashHas ) || _hashHas$1;

  var require$$4$1 = ( _hashSet$1 && _hashSet ) || _hashSet$1;

  var hashClear$1 = require$$0$d,
      hashDelete$1 = require$$1$9,
      hashGet$1 = require$$2$3,
      hashHas$1 = require$$3$2,
      hashSet$1 = require$$4$1;

  /**
   * Creates a hash object.
   *
   * @private
   * @constructor
   * @param {Array} [entries] The key-value pairs to cache.
   */
  function Hash(entries) {
    var this$1 = this;

    var index = -1,
        length = entries == null ? 0 : entries.length;

    this.clear();
    while (++index < length) {
      var entry = entries[index];
      this$1.set(entry[0], entry[1]);
    }
  }

  // Add methods to `Hash`.
  Hash.prototype.clear = hashClear$1;
  Hash.prototype['delete'] = hashDelete$1;
  Hash.prototype.get = hashGet$1;
  Hash.prototype.has = hashHas$1;
  Hash.prototype.set = hashSet$1;

  var _Hash = Hash;

  var _Hash$1 = /*#__PURE__*/Object.freeze({
    default: _Hash,
    __moduleExports: _Hash
  });

  var require$$0$e = ( _Hash$1 && _Hash ) || _Hash$1;

  var require$$2$4 = ( _Map$1 && _Map ) || _Map$1;

  var Hash$1 = require$$0$e,
      ListCache$2 = require$$1$6,
      Map$1 = require$$2$4;

  /**
   * Removes all key-value entries from the map.
   *
   * @private
   * @name clear
   * @memberOf MapCache
   */
  function mapCacheClear() {
    this.size = 0;
    this.__data__ = {
      'hash': new Hash$1,
      'map': new (Map$1 || ListCache$2),
      'string': new Hash$1
    };
  }

  var _mapCacheClear = mapCacheClear;

  var _mapCacheClear$1 = /*#__PURE__*/Object.freeze({
    default: _mapCacheClear,
    __moduleExports: _mapCacheClear
  });

  /**
   * Checks if `value` is suitable for use as unique object key.
   *
   * @private
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is suitable, else `false`.
   */
  function isKeyable(value) {
    var type = typeof value;
    return (type == 'string' || type == 'number' || type == 'symbol' || type == 'boolean')
      ? (value !== '__proto__')
      : (value === null);
  }

  var _isKeyable = isKeyable;

  var _isKeyable$1 = /*#__PURE__*/Object.freeze({
    default: _isKeyable,
    __moduleExports: _isKeyable
  });

  var require$$0$f = ( _isKeyable$1 && _isKeyable ) || _isKeyable$1;

  var isKeyable$1 = require$$0$f;

  /**
   * Gets the data for `map`.
   *
   * @private
   * @param {Object} map The map to query.
   * @param {string} key The reference key.
   * @returns {*} Returns the map data.
   */
  function getMapData(map, key) {
    var data = map.__data__;
    return isKeyable$1(key)
      ? data[typeof key == 'string' ? 'string' : 'hash']
      : data.map;
  }

  var _getMapData = getMapData;

  var _getMapData$1 = /*#__PURE__*/Object.freeze({
    default: _getMapData,
    __moduleExports: _getMapData
  });

  var require$$0$g = ( _getMapData$1 && _getMapData ) || _getMapData$1;

  var getMapData$1 = require$$0$g;

  /**
   * Removes `key` and its value from the map.
   *
   * @private
   * @name delete
   * @memberOf MapCache
   * @param {string} key The key of the value to remove.
   * @returns {boolean} Returns `true` if the entry was removed, else `false`.
   */
  function mapCacheDelete(key) {
    var result = getMapData$1(this, key)['delete'](key);
    this.size -= result ? 1 : 0;
    return result;
  }

  var _mapCacheDelete = mapCacheDelete;

  var _mapCacheDelete$1 = /*#__PURE__*/Object.freeze({
    default: _mapCacheDelete,
    __moduleExports: _mapCacheDelete
  });

  var getMapData$2 = require$$0$g;

  /**
   * Gets the map value for `key`.
   *
   * @private
   * @name get
   * @memberOf MapCache
   * @param {string} key The key of the value to get.
   * @returns {*} Returns the entry value.
   */
  function mapCacheGet(key) {
    return getMapData$2(this, key).get(key);
  }

  var _mapCacheGet = mapCacheGet;

  var _mapCacheGet$1 = /*#__PURE__*/Object.freeze({
    default: _mapCacheGet,
    __moduleExports: _mapCacheGet
  });

  var getMapData$3 = require$$0$g;

  /**
   * Checks if a map value for `key` exists.
   *
   * @private
   * @name has
   * @memberOf MapCache
   * @param {string} key The key of the entry to check.
   * @returns {boolean} Returns `true` if an entry for `key` exists, else `false`.
   */
  function mapCacheHas(key) {
    return getMapData$3(this, key).has(key);
  }

  var _mapCacheHas = mapCacheHas;

  var _mapCacheHas$1 = /*#__PURE__*/Object.freeze({
    default: _mapCacheHas,
    __moduleExports: _mapCacheHas
  });

  var getMapData$4 = require$$0$g;

  /**
   * Sets the map `key` to `value`.
   *
   * @private
   * @name set
   * @memberOf MapCache
   * @param {string} key The key of the value to set.
   * @param {*} value The value to set.
   * @returns {Object} Returns the map cache instance.
   */
  function mapCacheSet(key, value) {
    var data = getMapData$4(this, key),
        size = data.size;

    data.set(key, value);
    this.size += data.size == size ? 0 : 1;
    return this;
  }

  var _mapCacheSet = mapCacheSet;

  var _mapCacheSet$1 = /*#__PURE__*/Object.freeze({
    default: _mapCacheSet,
    __moduleExports: _mapCacheSet
  });

  var require$$0$h = ( _mapCacheClear$1 && _mapCacheClear ) || _mapCacheClear$1;

  var require$$1$a = ( _mapCacheDelete$1 && _mapCacheDelete ) || _mapCacheDelete$1;

  var require$$2$5 = ( _mapCacheGet$1 && _mapCacheGet ) || _mapCacheGet$1;

  var require$$3$3 = ( _mapCacheHas$1 && _mapCacheHas ) || _mapCacheHas$1;

  var require$$4$2 = ( _mapCacheSet$1 && _mapCacheSet ) || _mapCacheSet$1;

  var mapCacheClear$1 = require$$0$h,
      mapCacheDelete$1 = require$$1$a,
      mapCacheGet$1 = require$$2$5,
      mapCacheHas$1 = require$$3$3,
      mapCacheSet$1 = require$$4$2;

  /**
   * Creates a map cache object to store key-value pairs.
   *
   * @private
   * @constructor
   * @param {Array} [entries] The key-value pairs to cache.
   */
  function MapCache(entries) {
    var this$1 = this;

    var index = -1,
        length = entries == null ? 0 : entries.length;

    this.clear();
    while (++index < length) {
      var entry = entries[index];
      this$1.set(entry[0], entry[1]);
    }
  }

  // Add methods to `MapCache`.
  MapCache.prototype.clear = mapCacheClear$1;
  MapCache.prototype['delete'] = mapCacheDelete$1;
  MapCache.prototype.get = mapCacheGet$1;
  MapCache.prototype.has = mapCacheHas$1;
  MapCache.prototype.set = mapCacheSet$1;

  var _MapCache = MapCache;

  var _MapCache$1 = /*#__PURE__*/Object.freeze({
    default: _MapCache,
    __moduleExports: _MapCache
  });

  var require$$0$i = ( _MapCache$1 && _MapCache ) || _MapCache$1;

  var ListCache$3 = require$$1$6,
      Map$2 = require$$2$4,
      MapCache$1 = require$$0$i;

  /** Used as the size to enable large array optimizations. */
  var LARGE_ARRAY_SIZE = 200;

  /**
   * Sets the stack `key` to `value`.
   *
   * @private
   * @name set
   * @memberOf Stack
   * @param {string} key The key of the value to set.
   * @param {*} value The value to set.
   * @returns {Object} Returns the stack cache instance.
   */
  function stackSet(key, value) {
    var data = this.__data__;
    if (data instanceof ListCache$3) {
      var pairs = data.__data__;
      if (!Map$2 || (pairs.length < LARGE_ARRAY_SIZE - 1)) {
        pairs.push([key, value]);
        this.size = ++data.size;
        return this;
      }
      data = this.__data__ = new MapCache$1(pairs);
    }
    data.set(key, value);
    this.size = data.size;
    return this;
  }

  var _stackSet = stackSet;

  var _stackSet$1 = /*#__PURE__*/Object.freeze({
    default: _stackSet,
    __moduleExports: _stackSet
  });

  var require$$1$b = ( _stackClear$1 && _stackClear ) || _stackClear$1;

  var require$$2$6 = ( _stackDelete$1 && _stackDelete ) || _stackDelete$1;

  var require$$3$4 = ( _stackGet$1 && _stackGet ) || _stackGet$1;

  var require$$4$3 = ( _stackHas$1 && _stackHas ) || _stackHas$1;

  var require$$5 = ( _stackSet$1 && _stackSet ) || _stackSet$1;

  var ListCache$4 = require$$1$6,
      stackClear$1 = require$$1$b,
      stackDelete$1 = require$$2$6,
      stackGet$1 = require$$3$4,
      stackHas$1 = require$$4$3,
      stackSet$1 = require$$5;

  /**
   * Creates a stack cache object to store key-value pairs.
   *
   * @private
   * @constructor
   * @param {Array} [entries] The key-value pairs to cache.
   */
  function Stack(entries) {
    var data = this.__data__ = new ListCache$4(entries);
    this.size = data.size;
  }

  // Add methods to `Stack`.
  Stack.prototype.clear = stackClear$1;
  Stack.prototype['delete'] = stackDelete$1;
  Stack.prototype.get = stackGet$1;
  Stack.prototype.has = stackHas$1;
  Stack.prototype.set = stackSet$1;

  var _Stack = Stack;

  var _Stack$1 = /*#__PURE__*/Object.freeze({
    default: _Stack,
    __moduleExports: _Stack
  });

  /** Used to stand-in for `undefined` hash values. */
  var HASH_UNDEFINED$2 = '__lodash_hash_undefined__';

  /**
   * Adds `value` to the array cache.
   *
   * @private
   * @name add
   * @memberOf SetCache
   * @alias push
   * @param {*} value The value to cache.
   * @returns {Object} Returns the cache instance.
   */
  function setCacheAdd(value) {
    this.__data__.set(value, HASH_UNDEFINED$2);
    return this;
  }

  var _setCacheAdd = setCacheAdd;

  var _setCacheAdd$1 = /*#__PURE__*/Object.freeze({
    default: _setCacheAdd,
    __moduleExports: _setCacheAdd
  });

  /**
   * Checks if `value` is in the array cache.
   *
   * @private
   * @name has
   * @memberOf SetCache
   * @param {*} value The value to search for.
   * @returns {number} Returns `true` if `value` is found, else `false`.
   */
  function setCacheHas(value) {
    return this.__data__.has(value);
  }

  var _setCacheHas = setCacheHas;

  var _setCacheHas$1 = /*#__PURE__*/Object.freeze({
    default: _setCacheHas,
    __moduleExports: _setCacheHas
  });

  var require$$1$c = ( _setCacheAdd$1 && _setCacheAdd ) || _setCacheAdd$1;

  var require$$2$7 = ( _setCacheHas$1 && _setCacheHas ) || _setCacheHas$1;

  var MapCache$2 = require$$0$i,
      setCacheAdd$1 = require$$1$c,
      setCacheHas$1 = require$$2$7;

  /**
   *
   * Creates an array cache object to store unique values.
   *
   * @private
   * @constructor
   * @param {Array} [values] The values to cache.
   */
  function SetCache(values) {
    var this$1 = this;

    var index = -1,
        length = values == null ? 0 : values.length;

    this.__data__ = new MapCache$2;
    while (++index < length) {
      this$1.add(values[index]);
    }
  }

  // Add methods to `SetCache`.
  SetCache.prototype.add = SetCache.prototype.push = setCacheAdd$1;
  SetCache.prototype.has = setCacheHas$1;

  var _SetCache = SetCache;

  var _SetCache$1 = /*#__PURE__*/Object.freeze({
    default: _SetCache,
    __moduleExports: _SetCache
  });

  /**
   * A specialized version of `_.some` for arrays without support for iteratee
   * shorthands.
   *
   * @private
   * @param {Array} [array] The array to iterate over.
   * @param {Function} predicate The function invoked per iteration.
   * @returns {boolean} Returns `true` if any element passes the predicate check,
   *  else `false`.
   */
  function arraySome(array, predicate) {
    var index = -1,
        length = array == null ? 0 : array.length;

    while (++index < length) {
      if (predicate(array[index], index, array)) {
        return true;
      }
    }
    return false;
  }

  var _arraySome = arraySome;

  var _arraySome$1 = /*#__PURE__*/Object.freeze({
    default: _arraySome,
    __moduleExports: _arraySome
  });

  /**
   * Checks if a `cache` value for `key` exists.
   *
   * @private
   * @param {Object} cache The cache to query.
   * @param {string} key The key of the entry to check.
   * @returns {boolean} Returns `true` if an entry for `key` exists, else `false`.
   */
  function cacheHas(cache, key) {
    return cache.has(key);
  }

  var _cacheHas = cacheHas;

  var _cacheHas$1 = /*#__PURE__*/Object.freeze({
    default: _cacheHas,
    __moduleExports: _cacheHas
  });

  var require$$0$j = ( _SetCache$1 && _SetCache ) || _SetCache$1;

  var require$$1$d = ( _arraySome$1 && _arraySome ) || _arraySome$1;

  var require$$2$8 = ( _cacheHas$1 && _cacheHas ) || _cacheHas$1;

  var SetCache$1 = require$$0$j,
      arraySome$1 = require$$1$d,
      cacheHas$1 = require$$2$8;

  /** Used to compose bitmasks for value comparisons. */
  var COMPARE_PARTIAL_FLAG = 1,
      COMPARE_UNORDERED_FLAG = 2;

  /**
   * A specialized version of `baseIsEqualDeep` for arrays with support for
   * partial deep comparisons.
   *
   * @private
   * @param {Array} array The array to compare.
   * @param {Array} other The other array to compare.
   * @param {number} bitmask The bitmask flags. See `baseIsEqual` for more details.
   * @param {Function} customizer The function to customize comparisons.
   * @param {Function} equalFunc The function to determine equivalents of values.
   * @param {Object} stack Tracks traversed `array` and `other` objects.
   * @returns {boolean} Returns `true` if the arrays are equivalent, else `false`.
   */
  function equalArrays(array, other, bitmask, customizer, equalFunc, stack) {
    var isPartial = bitmask & COMPARE_PARTIAL_FLAG,
        arrLength = array.length,
        othLength = other.length;

    if (arrLength != othLength && !(isPartial && othLength > arrLength)) {
      return false;
    }
    // Assume cyclic values are equal.
    var stacked = stack.get(array);
    if (stacked && stack.get(other)) {
      return stacked == other;
    }
    var index = -1,
        result = true,
        seen = (bitmask & COMPARE_UNORDERED_FLAG) ? new SetCache$1 : undefined;

    stack.set(array, other);
    stack.set(other, array);

    // Ignore non-index properties.
    while (++index < arrLength) {
      var arrValue = array[index],
          othValue = other[index];

      if (customizer) {
        var compared = isPartial
          ? customizer(othValue, arrValue, index, other, array, stack)
          : customizer(arrValue, othValue, index, array, other, stack);
      }
      if (compared !== undefined) {
        if (compared) {
          continue;
        }
        result = false;
        break;
      }
      // Recursively compare arrays (susceptible to call stack limits).
      if (seen) {
        if (!arraySome$1(other, function(othValue, othIndex) {
              if (!cacheHas$1(seen, othIndex) &&
                  (arrValue === othValue || equalFunc(arrValue, othValue, bitmask, customizer, stack))) {
                return seen.push(othIndex);
              }
            })) {
          result = false;
          break;
        }
      } else if (!(
            arrValue === othValue ||
              equalFunc(arrValue, othValue, bitmask, customizer, stack)
          )) {
        result = false;
        break;
      }
    }
    stack['delete'](array);
    stack['delete'](other);
    return result;
  }

  var _equalArrays = equalArrays;

  var _equalArrays$1 = /*#__PURE__*/Object.freeze({
    default: _equalArrays,
    __moduleExports: _equalArrays
  });

  var root$5 = require$$0$1;

  /** Built-in value references. */
  var Uint8Array = root$5.Uint8Array;

  var _Uint8Array = Uint8Array;

  var _Uint8Array$1 = /*#__PURE__*/Object.freeze({
    default: _Uint8Array,
    __moduleExports: _Uint8Array
  });

  /**
   * Converts `map` to its key-value pairs.
   *
   * @private
   * @param {Object} map The map to convert.
   * @returns {Array} Returns the key-value pairs.
   */
  function mapToArray(map) {
    var index = -1,
        result = Array(map.size);

    map.forEach(function(value, key) {
      result[++index] = [key, value];
    });
    return result;
  }

  var _mapToArray = mapToArray;

  var _mapToArray$1 = /*#__PURE__*/Object.freeze({
    default: _mapToArray,
    __moduleExports: _mapToArray
  });

  /**
   * Converts `set` to an array of its values.
   *
   * @private
   * @param {Object} set The set to convert.
   * @returns {Array} Returns the values.
   */
  function setToArray(set) {
    var index = -1,
        result = Array(set.size);

    set.forEach(function(value) {
      result[++index] = value;
    });
    return result;
  }

  var _setToArray = setToArray;

  var _setToArray$1 = /*#__PURE__*/Object.freeze({
    default: _setToArray,
    __moduleExports: _setToArray
  });

  var require$$1$e = ( _Uint8Array$1 && _Uint8Array ) || _Uint8Array$1;

  var require$$3$5 = ( _equalArrays$1 && _equalArrays ) || _equalArrays$1;

  var require$$4$4 = ( _mapToArray$1 && _mapToArray ) || _mapToArray$1;

  var require$$5$1 = ( _setToArray$1 && _setToArray ) || _setToArray$1;

  var Symbol$3 = require$$0$2,
      Uint8Array$1 = require$$1$e,
      eq$3 = require$$0$5,
      equalArrays$1 = require$$3$5,
      mapToArray$1 = require$$4$4,
      setToArray$1 = require$$5$1;

  /** Used to compose bitmasks for value comparisons. */
  var COMPARE_PARTIAL_FLAG$1 = 1,
      COMPARE_UNORDERED_FLAG$1 = 2;

  /** `Object#toString` result references. */
  var boolTag = '[object Boolean]',
      dateTag = '[object Date]',
      errorTag = '[object Error]',
      mapTag = '[object Map]',
      numberTag = '[object Number]',
      regexpTag = '[object RegExp]',
      setTag = '[object Set]',
      stringTag = '[object String]',
      symbolTag$1 = '[object Symbol]';

  var arrayBufferTag = '[object ArrayBuffer]',
      dataViewTag = '[object DataView]';

  /** Used to convert symbols to primitives and strings. */
  var symbolProto = Symbol$3 ? Symbol$3.prototype : undefined,
      symbolValueOf = symbolProto ? symbolProto.valueOf : undefined;

  /**
   * A specialized version of `baseIsEqualDeep` for comparing objects of
   * the same `toStringTag`.
   *
   * **Note:** This function only supports comparing values with tags of
   * `Boolean`, `Date`, `Error`, `Number`, `RegExp`, or `String`.
   *
   * @private
   * @param {Object} object The object to compare.
   * @param {Object} other The other object to compare.
   * @param {string} tag The `toStringTag` of the objects to compare.
   * @param {number} bitmask The bitmask flags. See `baseIsEqual` for more details.
   * @param {Function} customizer The function to customize comparisons.
   * @param {Function} equalFunc The function to determine equivalents of values.
   * @param {Object} stack Tracks traversed `object` and `other` objects.
   * @returns {boolean} Returns `true` if the objects are equivalent, else `false`.
   */
  function equalByTag(object, other, tag, bitmask, customizer, equalFunc, stack) {
    switch (tag) {
      case dataViewTag:
        if ((object.byteLength != other.byteLength) ||
            (object.byteOffset != other.byteOffset)) {
          return false;
        }
        object = object.buffer;
        other = other.buffer;

      case arrayBufferTag:
        if ((object.byteLength != other.byteLength) ||
            !equalFunc(new Uint8Array$1(object), new Uint8Array$1(other))) {
          return false;
        }
        return true;

      case boolTag:
      case dateTag:
      case numberTag:
        // Coerce booleans to `1` or `0` and dates to milliseconds.
        // Invalid dates are coerced to `NaN`.
        return eq$3(+object, +other);

      case errorTag:
        return object.name == other.name && object.message == other.message;

      case regexpTag:
      case stringTag:
        // Coerce regexes to strings and treat strings, primitives and objects,
        // as equal. See http://www.ecma-international.org/ecma-262/7.0/#sec-regexp.prototype.tostring
        // for more details.
        return object == (other + '');

      case mapTag:
        var convert = mapToArray$1;

      case setTag:
        var isPartial = bitmask & COMPARE_PARTIAL_FLAG$1;
        convert || (convert = setToArray$1);

        if (object.size != other.size && !isPartial) {
          return false;
        }
        // Assume cyclic values are equal.
        var stacked = stack.get(object);
        if (stacked) {
          return stacked == other;
        }
        bitmask |= COMPARE_UNORDERED_FLAG$1;

        // Recursively compare objects (susceptible to call stack limits).
        stack.set(object, other);
        var result = equalArrays$1(convert(object), convert(other), bitmask, customizer, equalFunc, stack);
        stack['delete'](object);
        return result;

      case symbolTag$1:
        if (symbolValueOf) {
          return symbolValueOf.call(object) == symbolValueOf.call(other);
        }
    }
    return false;
  }

  var _equalByTag = equalByTag;

  var _equalByTag$1 = /*#__PURE__*/Object.freeze({
    default: _equalByTag,
    __moduleExports: _equalByTag
  });

  /**
   * Appends the elements of `values` to `array`.
   *
   * @private
   * @param {Array} array The array to modify.
   * @param {Array} values The values to append.
   * @returns {Array} Returns `array`.
   */
  function arrayPush(array, values) {
    var index = -1,
        length = values.length,
        offset = array.length;

    while (++index < length) {
      array[offset + index] = values[index];
    }
    return array;
  }

  var _arrayPush = arrayPush;

  var _arrayPush$1 = /*#__PURE__*/Object.freeze({
    default: _arrayPush,
    __moduleExports: _arrayPush
  });

  /**
   * Checks if `value` is classified as an `Array` object.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is an array, else `false`.
   * @example
   *
   * _.isArray([1, 2, 3]);
   * // => true
   *
   * _.isArray(document.body.children);
   * // => false
   *
   * _.isArray('abc');
   * // => false
   *
   * _.isArray(_.noop);
   * // => false
   */
  var isArray = Array.isArray;

  var isArray_1 = isArray;

  var isArray$1 = /*#__PURE__*/Object.freeze({
    default: isArray_1,
    __moduleExports: isArray_1
  });

  var require$$0$k = ( _arrayPush$1 && _arrayPush ) || _arrayPush$1;

  var require$$2$9 = ( isArray$1 && isArray_1 ) || isArray$1;

  var arrayPush$1 = require$$0$k,
      isArray$2 = require$$2$9;

  /**
   * The base implementation of `getAllKeys` and `getAllKeysIn` which uses
   * `keysFunc` and `symbolsFunc` to get the enumerable property names and
   * symbols of `object`.
   *
   * @private
   * @param {Object} object The object to query.
   * @param {Function} keysFunc The function to get the keys of `object`.
   * @param {Function} symbolsFunc The function to get the symbols of `object`.
   * @returns {Array} Returns the array of property names and symbols.
   */
  function baseGetAllKeys(object, keysFunc, symbolsFunc) {
    var result = keysFunc(object);
    return isArray$2(object) ? result : arrayPush$1(result, symbolsFunc(object));
  }

  var _baseGetAllKeys = baseGetAllKeys;

  var _baseGetAllKeys$1 = /*#__PURE__*/Object.freeze({
    default: _baseGetAllKeys,
    __moduleExports: _baseGetAllKeys
  });

  /**
   * A specialized version of `_.filter` for arrays without support for
   * iteratee shorthands.
   *
   * @private
   * @param {Array} [array] The array to iterate over.
   * @param {Function} predicate The function invoked per iteration.
   * @returns {Array} Returns the new filtered array.
   */
  function arrayFilter(array, predicate) {
    var index = -1,
        length = array == null ? 0 : array.length,
        resIndex = 0,
        result = [];

    while (++index < length) {
      var value = array[index];
      if (predicate(value, index, array)) {
        result[resIndex++] = value;
      }
    }
    return result;
  }

  var _arrayFilter = arrayFilter;

  var _arrayFilter$1 = /*#__PURE__*/Object.freeze({
    default: _arrayFilter,
    __moduleExports: _arrayFilter
  });

  /**
   * This method returns a new empty array.
   *
   * @static
   * @memberOf _
   * @since 4.13.0
   * @category Util
   * @returns {Array} Returns the new empty array.
   * @example
   *
   * var arrays = _.times(2, _.stubArray);
   *
   * console.log(arrays);
   * // => [[], []]
   *
   * console.log(arrays[0] === arrays[1]);
   * // => false
   */
  function stubArray() {
    return [];
  }

  var stubArray_1 = stubArray;

  var stubArray$1 = /*#__PURE__*/Object.freeze({
    default: stubArray_1,
    __moduleExports: stubArray_1
  });

  var require$$0$l = ( _arrayFilter$1 && _arrayFilter ) || _arrayFilter$1;

  var require$$1$f = ( stubArray$1 && stubArray_1 ) || stubArray$1;

  var arrayFilter$1 = require$$0$l,
      stubArray$2 = require$$1$f;

  /** Used for built-in method references. */
  var objectProto$5 = Object.prototype;

  /** Built-in value references. */
  var propertyIsEnumerable = objectProto$5.propertyIsEnumerable;

  /* Built-in method references for those with the same name as other `lodash` methods. */
  var nativeGetSymbols = Object.getOwnPropertySymbols;

  /**
   * Creates an array of the own enumerable symbols of `object`.
   *
   * @private
   * @param {Object} object The object to query.
   * @returns {Array} Returns the array of symbols.
   */
  var getSymbols = !nativeGetSymbols ? stubArray$2 : function(object) {
    if (object == null) {
      return [];
    }
    object = Object(object);
    return arrayFilter$1(nativeGetSymbols(object), function(symbol) {
      return propertyIsEnumerable.call(object, symbol);
    });
  };

  var _getSymbols = getSymbols;

  var _getSymbols$1 = /*#__PURE__*/Object.freeze({
    default: _getSymbols,
    __moduleExports: _getSymbols
  });

  /**
   * The base implementation of `_.times` without support for iteratee shorthands
   * or max array length checks.
   *
   * @private
   * @param {number} n The number of times to invoke `iteratee`.
   * @param {Function} iteratee The function invoked per iteration.
   * @returns {Array} Returns the array of results.
   */
  function baseTimes(n, iteratee) {
    var index = -1,
        result = Array(n);

    while (++index < n) {
      result[index] = iteratee(index);
    }
    return result;
  }

  var _baseTimes = baseTimes;

  var _baseTimes$1 = /*#__PURE__*/Object.freeze({
    default: _baseTimes,
    __moduleExports: _baseTimes
  });

  var baseGetTag$3 = require$$0$3,
      isObjectLike$3 = require$$1$1;

  /** `Object#toString` result references. */
  var argsTag = '[object Arguments]';

  /**
   * The base implementation of `_.isArguments`.
   *
   * @private
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is an `arguments` object,
   */
  function baseIsArguments(value) {
    return isObjectLike$3(value) && baseGetTag$3(value) == argsTag;
  }

  var _baseIsArguments = baseIsArguments;

  var _baseIsArguments$1 = /*#__PURE__*/Object.freeze({
    default: _baseIsArguments,
    __moduleExports: _baseIsArguments
  });

  var require$$0$m = ( _baseIsArguments$1 && _baseIsArguments ) || _baseIsArguments$1;

  var baseIsArguments$1 = require$$0$m,
      isObjectLike$4 = require$$1$1;

  /** Used for built-in method references. */
  var objectProto$6 = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$5 = objectProto$6.hasOwnProperty;

  /** Built-in value references. */
  var propertyIsEnumerable$1 = objectProto$6.propertyIsEnumerable;

  /**
   * Checks if `value` is likely an `arguments` object.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is an `arguments` object,
   *  else `false`.
   * @example
   *
   * _.isArguments(function() { return arguments; }());
   * // => true
   *
   * _.isArguments([1, 2, 3]);
   * // => false
   */
  var isArguments = baseIsArguments$1(function() { return arguments; }()) ? baseIsArguments$1 : function(value) {
    return isObjectLike$4(value) && hasOwnProperty$5.call(value, 'callee') &&
      !propertyIsEnumerable$1.call(value, 'callee');
  };

  var isArguments_1 = isArguments;

  var isArguments$1 = /*#__PURE__*/Object.freeze({
    default: isArguments_1,
    __moduleExports: isArguments_1
  });

  /**
   * This method returns `false`.
   *
   * @static
   * @memberOf _
   * @since 4.13.0
   * @category Util
   * @returns {boolean} Returns `false`.
   * @example
   *
   * _.times(2, _.stubFalse);
   * // => [false, false]
   */
  function stubFalse() {
    return false;
  }

  var stubFalse_1 = stubFalse;

  var stubFalse$1 = /*#__PURE__*/Object.freeze({
    default: stubFalse_1,
    __moduleExports: stubFalse_1
  });

  var require$$1$g = ( stubFalse$1 && stubFalse_1 ) || stubFalse$1;

  var isBuffer_1 = createCommonjsModule(function (module, exports) {
  var root = require$$0$1,
      stubFalse = require$$1$g;

  /** Detect free variable `exports`. */
  var freeExports = exports && !exports.nodeType && exports;

  /** Detect free variable `module`. */
  var freeModule = freeExports && 'object' == 'object' && module && !module.nodeType && module;

  /** Detect the popular CommonJS extension `module.exports`. */
  var moduleExports = freeModule && freeModule.exports === freeExports;

  /** Built-in value references. */
  var Buffer = moduleExports ? root.Buffer : undefined;

  /* Built-in method references for those with the same name as other `lodash` methods. */
  var nativeIsBuffer = Buffer ? Buffer.isBuffer : undefined;

  /**
   * Checks if `value` is a buffer.
   *
   * @static
   * @memberOf _
   * @since 4.3.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a buffer, else `false`.
   * @example
   *
   * _.isBuffer(new Buffer(2));
   * // => true
   *
   * _.isBuffer(new Uint8Array(2));
   * // => false
   */
  var isBuffer = nativeIsBuffer || stubFalse;

  module.exports = isBuffer;
  });

  var isBuffer = /*#__PURE__*/Object.freeze({
    default: isBuffer_1,
    __moduleExports: isBuffer_1
  });

  /** Used as references for various `Number` constants. */
  var MAX_SAFE_INTEGER = 9007199254740991;

  /** Used to detect unsigned integer values. */
  var reIsUint = /^(?:0|[1-9]\d*)$/;

  /**
   * Checks if `value` is a valid array-like index.
   *
   * @private
   * @param {*} value The value to check.
   * @param {number} [length=MAX_SAFE_INTEGER] The upper bounds of a valid index.
   * @returns {boolean} Returns `true` if `value` is a valid index, else `false`.
   */
  function isIndex(value, length) {
    var type = typeof value;
    length = length == null ? MAX_SAFE_INTEGER : length;

    return !!length &&
      (type == 'number' ||
        (type != 'symbol' && reIsUint.test(value))) &&
          (value > -1 && value % 1 == 0 && value < length);
  }

  var _isIndex = isIndex;

  var _isIndex$1 = /*#__PURE__*/Object.freeze({
    default: _isIndex,
    __moduleExports: _isIndex
  });

  /** Used as references for various `Number` constants. */
  var MAX_SAFE_INTEGER$1 = 9007199254740991;

  /**
   * Checks if `value` is a valid array-like length.
   *
   * **Note:** This method is loosely based on
   * [`ToLength`](http://ecma-international.org/ecma-262/7.0/#sec-tolength).
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a valid length, else `false`.
   * @example
   *
   * _.isLength(3);
   * // => true
   *
   * _.isLength(Number.MIN_VALUE);
   * // => false
   *
   * _.isLength(Infinity);
   * // => false
   *
   * _.isLength('3');
   * // => false
   */
  function isLength(value) {
    return typeof value == 'number' &&
      value > -1 && value % 1 == 0 && value <= MAX_SAFE_INTEGER$1;
  }

  var isLength_1 = isLength;

  var isLength$1 = /*#__PURE__*/Object.freeze({
    default: isLength_1,
    __moduleExports: isLength_1
  });

  var require$$1$h = ( isLength$1 && isLength_1 ) || isLength$1;

  var baseGetTag$4 = require$$0$3,
      isLength$2 = require$$1$h,
      isObjectLike$5 = require$$1$1;

  /** `Object#toString` result references. */
  var argsTag$1 = '[object Arguments]',
      arrayTag = '[object Array]',
      boolTag$1 = '[object Boolean]',
      dateTag$1 = '[object Date]',
      errorTag$1 = '[object Error]',
      funcTag$1 = '[object Function]',
      mapTag$1 = '[object Map]',
      numberTag$1 = '[object Number]',
      objectTag = '[object Object]',
      regexpTag$1 = '[object RegExp]',
      setTag$1 = '[object Set]',
      stringTag$1 = '[object String]',
      weakMapTag = '[object WeakMap]';

  var arrayBufferTag$1 = '[object ArrayBuffer]',
      dataViewTag$1 = '[object DataView]',
      float32Tag = '[object Float32Array]',
      float64Tag = '[object Float64Array]',
      int8Tag = '[object Int8Array]',
      int16Tag = '[object Int16Array]',
      int32Tag = '[object Int32Array]',
      uint8Tag = '[object Uint8Array]',
      uint8ClampedTag = '[object Uint8ClampedArray]',
      uint16Tag = '[object Uint16Array]',
      uint32Tag = '[object Uint32Array]';

  /** Used to identify `toStringTag` values of typed arrays. */
  var typedArrayTags = {};
  typedArrayTags[float32Tag] = typedArrayTags[float64Tag] =
  typedArrayTags[int8Tag] = typedArrayTags[int16Tag] =
  typedArrayTags[int32Tag] = typedArrayTags[uint8Tag] =
  typedArrayTags[uint8ClampedTag] = typedArrayTags[uint16Tag] =
  typedArrayTags[uint32Tag] = true;
  typedArrayTags[argsTag$1] = typedArrayTags[arrayTag] =
  typedArrayTags[arrayBufferTag$1] = typedArrayTags[boolTag$1] =
  typedArrayTags[dataViewTag$1] = typedArrayTags[dateTag$1] =
  typedArrayTags[errorTag$1] = typedArrayTags[funcTag$1] =
  typedArrayTags[mapTag$1] = typedArrayTags[numberTag$1] =
  typedArrayTags[objectTag] = typedArrayTags[regexpTag$1] =
  typedArrayTags[setTag$1] = typedArrayTags[stringTag$1] =
  typedArrayTags[weakMapTag] = false;

  /**
   * The base implementation of `_.isTypedArray` without Node.js optimizations.
   *
   * @private
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a typed array, else `false`.
   */
  function baseIsTypedArray(value) {
    return isObjectLike$5(value) &&
      isLength$2(value.length) && !!typedArrayTags[baseGetTag$4(value)];
  }

  var _baseIsTypedArray = baseIsTypedArray;

  var _baseIsTypedArray$1 = /*#__PURE__*/Object.freeze({
    default: _baseIsTypedArray,
    __moduleExports: _baseIsTypedArray
  });

  /**
   * The base implementation of `_.unary` without support for storing metadata.
   *
   * @private
   * @param {Function} func The function to cap arguments for.
   * @returns {Function} Returns the new capped function.
   */
  function baseUnary(func) {
    return function(value) {
      return func(value);
    };
  }

  var _baseUnary = baseUnary;

  var _baseUnary$1 = /*#__PURE__*/Object.freeze({
    default: _baseUnary,
    __moduleExports: _baseUnary
  });

  var _nodeUtil = createCommonjsModule(function (module, exports) {
  var freeGlobal = require$$0;

  /** Detect free variable `exports`. */
  var freeExports = exports && !exports.nodeType && exports;

  /** Detect free variable `module`. */
  var freeModule = freeExports && 'object' == 'object' && module && !module.nodeType && module;

  /** Detect the popular CommonJS extension `module.exports`. */
  var moduleExports = freeModule && freeModule.exports === freeExports;

  /** Detect free variable `process` from Node.js. */
  var freeProcess = moduleExports && freeGlobal.process;

  /** Used to access faster Node.js helpers. */
  var nodeUtil = (function() {
    try {
      // Use `util.types` for Node.js 10+.
      var types = freeModule && freeModule.require && freeModule.require('util').types;

      if (types) {
        return types;
      }

      // Legacy `process.binding('util')` for Node.js < 10.
      return freeProcess && freeProcess.binding && freeProcess.binding('util');
    } catch (e) {}
  }());

  module.exports = nodeUtil;
  });

  var _nodeUtil$1 = /*#__PURE__*/Object.freeze({
    default: _nodeUtil,
    __moduleExports: _nodeUtil
  });

  var require$$0$n = ( _baseIsTypedArray$1 && _baseIsTypedArray ) || _baseIsTypedArray$1;

  var require$$1$i = ( _baseUnary$1 && _baseUnary ) || _baseUnary$1;

  var require$$2$a = ( _nodeUtil$1 && _nodeUtil ) || _nodeUtil$1;

  var baseIsTypedArray$1 = require$$0$n,
      baseUnary$1 = require$$1$i,
      nodeUtil = require$$2$a;

  /* Node.js helper references. */
  var nodeIsTypedArray = nodeUtil && nodeUtil.isTypedArray;

  /**
   * Checks if `value` is classified as a typed array.
   *
   * @static
   * @memberOf _
   * @since 3.0.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a typed array, else `false`.
   * @example
   *
   * _.isTypedArray(new Uint8Array);
   * // => true
   *
   * _.isTypedArray([]);
   * // => false
   */
  var isTypedArray = nodeIsTypedArray ? baseUnary$1(nodeIsTypedArray) : baseIsTypedArray$1;

  var isTypedArray_1 = isTypedArray;

  var isTypedArray$1 = /*#__PURE__*/Object.freeze({
    default: isTypedArray_1,
    __moduleExports: isTypedArray_1
  });

  var require$$0$o = ( _baseTimes$1 && _baseTimes ) || _baseTimes$1;

  var require$$1$j = ( isArguments$1 && isArguments_1 ) || isArguments$1;

  var require$$3$6 = ( isBuffer && isBuffer_1 ) || isBuffer;

  var require$$4$5 = ( _isIndex$1 && _isIndex ) || _isIndex$1;

  var require$$5$2 = ( isTypedArray$1 && isTypedArray_1 ) || isTypedArray$1;

  var baseTimes$1 = require$$0$o,
      isArguments$2 = require$$1$j,
      isArray$3 = require$$2$9,
      isBuffer$1 = require$$3$6,
      isIndex$1 = require$$4$5,
      isTypedArray$2 = require$$5$2;

  /** Used for built-in method references. */
  var objectProto$7 = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$6 = objectProto$7.hasOwnProperty;

  /**
   * Creates an array of the enumerable property names of the array-like `value`.
   *
   * @private
   * @param {*} value The value to query.
   * @param {boolean} inherited Specify returning inherited property names.
   * @returns {Array} Returns the array of property names.
   */
  function arrayLikeKeys(value, inherited) {
    var isArr = isArray$3(value),
        isArg = !isArr && isArguments$2(value),
        isBuff = !isArr && !isArg && isBuffer$1(value),
        isType = !isArr && !isArg && !isBuff && isTypedArray$2(value),
        skipIndexes = isArr || isArg || isBuff || isType,
        result = skipIndexes ? baseTimes$1(value.length, String) : [],
        length = result.length;

    for (var key in value) {
      if ((inherited || hasOwnProperty$6.call(value, key)) &&
          !(skipIndexes && (
             // Safari 9 has enumerable `arguments.length` in strict mode.
             key == 'length' ||
             // Node.js 0.10 has enumerable non-index properties on buffers.
             (isBuff && (key == 'offset' || key == 'parent')) ||
             // PhantomJS 2 has enumerable non-index properties on typed arrays.
             (isType && (key == 'buffer' || key == 'byteLength' || key == 'byteOffset')) ||
             // Skip index properties.
             isIndex$1(key, length)
          ))) {
        result.push(key);
      }
    }
    return result;
  }

  var _arrayLikeKeys = arrayLikeKeys;

  var _arrayLikeKeys$1 = /*#__PURE__*/Object.freeze({
    default: _arrayLikeKeys,
    __moduleExports: _arrayLikeKeys
  });

  /** Used for built-in method references. */
  var objectProto$8 = Object.prototype;

  /**
   * Checks if `value` is likely a prototype object.
   *
   * @private
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is a prototype, else `false`.
   */
  function isPrototype(value) {
    var Ctor = value && value.constructor,
        proto = (typeof Ctor == 'function' && Ctor.prototype) || objectProto$8;

    return value === proto;
  }

  var _isPrototype = isPrototype;

  var _isPrototype$1 = /*#__PURE__*/Object.freeze({
    default: _isPrototype,
    __moduleExports: _isPrototype
  });

  /**
   * Creates a unary function that invokes `func` with its argument transformed.
   *
   * @private
   * @param {Function} func The function to wrap.
   * @param {Function} transform The argument transform.
   * @returns {Function} Returns the new function.
   */
  function overArg(func, transform) {
    return function(arg) {
      return func(transform(arg));
    };
  }

  var _overArg = overArg;

  var _overArg$1 = /*#__PURE__*/Object.freeze({
    default: _overArg,
    __moduleExports: _overArg
  });

  var require$$0$p = ( _overArg$1 && _overArg ) || _overArg$1;

  var overArg$1 = require$$0$p;

  /* Built-in method references for those with the same name as other `lodash` methods. */
  var nativeKeys = overArg$1(Object.keys, Object);

  var _nativeKeys = nativeKeys;

  var _nativeKeys$1 = /*#__PURE__*/Object.freeze({
    default: _nativeKeys,
    __moduleExports: _nativeKeys
  });

  var require$$0$q = ( _isPrototype$1 && _isPrototype ) || _isPrototype$1;

  var require$$1$k = ( _nativeKeys$1 && _nativeKeys ) || _nativeKeys$1;

  var isPrototype$1 = require$$0$q,
      nativeKeys$1 = require$$1$k;

  /** Used for built-in method references. */
  var objectProto$9 = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$7 = objectProto$9.hasOwnProperty;

  /**
   * The base implementation of `_.keys` which doesn't treat sparse arrays as dense.
   *
   * @private
   * @param {Object} object The object to query.
   * @returns {Array} Returns the array of property names.
   */
  function baseKeys(object) {
    if (!isPrototype$1(object)) {
      return nativeKeys$1(object);
    }
    var result = [];
    for (var key in Object(object)) {
      if (hasOwnProperty$7.call(object, key) && key != 'constructor') {
        result.push(key);
      }
    }
    return result;
  }

  var _baseKeys = baseKeys;

  var _baseKeys$1 = /*#__PURE__*/Object.freeze({
    default: _baseKeys,
    __moduleExports: _baseKeys
  });

  var isFunction$3 = require$$0$9,
      isLength$3 = require$$1$h;

  /**
   * Checks if `value` is array-like. A value is considered array-like if it's
   * not a function and has a `value.length` that's an integer greater than or
   * equal to `0` and less than or equal to `Number.MAX_SAFE_INTEGER`.
   *
   * @static
   * @memberOf _
   * @since 4.0.0
   * @category Lang
   * @param {*} value The value to check.
   * @returns {boolean} Returns `true` if `value` is array-like, else `false`.
   * @example
   *
   * _.isArrayLike([1, 2, 3]);
   * // => true
   *
   * _.isArrayLike(document.body.children);
   * // => true
   *
   * _.isArrayLike('abc');
   * // => true
   *
   * _.isArrayLike(_.noop);
   * // => false
   */
  function isArrayLike(value) {
    return value != null && isLength$3(value.length) && !isFunction$3(value);
  }

  var isArrayLike_1 = isArrayLike;

  var isArrayLike$1 = /*#__PURE__*/Object.freeze({
    default: isArrayLike_1,
    __moduleExports: isArrayLike_1
  });

  var require$$0$r = ( _arrayLikeKeys$1 && _arrayLikeKeys ) || _arrayLikeKeys$1;

  var require$$1$l = ( _baseKeys$1 && _baseKeys ) || _baseKeys$1;

  var require$$2$b = ( isArrayLike$1 && isArrayLike_1 ) || isArrayLike$1;

  var arrayLikeKeys$1 = require$$0$r,
      baseKeys$1 = require$$1$l,
      isArrayLike$2 = require$$2$b;

  /**
   * Creates an array of the own enumerable property names of `object`.
   *
   * **Note:** Non-object values are coerced to objects. See the
   * [ES spec](http://ecma-international.org/ecma-262/7.0/#sec-object.keys)
   * for more details.
   *
   * @static
   * @since 0.1.0
   * @memberOf _
   * @category Object
   * @param {Object} object The object to query.
   * @returns {Array} Returns the array of property names.
   * @example
   *
   * function Foo() {
   *   this.a = 1;
   *   this.b = 2;
   * }
   *
   * Foo.prototype.c = 3;
   *
   * _.keys(new Foo);
   * // => ['a', 'b'] (iteration order is not guaranteed)
   *
   * _.keys('hi');
   * // => ['0', '1']
   */
  function keys(object) {
    return isArrayLike$2(object) ? arrayLikeKeys$1(object) : baseKeys$1(object);
  }

  var keys_1 = keys;

  var keys$1 = /*#__PURE__*/Object.freeze({
    default: keys_1,
    __moduleExports: keys_1
  });

  var require$$0$s = ( _baseGetAllKeys$1 && _baseGetAllKeys ) || _baseGetAllKeys$1;

  var require$$1$m = ( _getSymbols$1 && _getSymbols ) || _getSymbols$1;

  var require$$2$c = ( keys$1 && keys_1 ) || keys$1;

  var baseGetAllKeys$1 = require$$0$s,
      getSymbols$1 = require$$1$m,
      keys$2 = require$$2$c;

  /**
   * Creates an array of own enumerable property names and symbols of `object`.
   *
   * @private
   * @param {Object} object The object to query.
   * @returns {Array} Returns the array of property names and symbols.
   */
  function getAllKeys(object) {
    return baseGetAllKeys$1(object, keys$2, getSymbols$1);
  }

  var _getAllKeys = getAllKeys;

  var _getAllKeys$1 = /*#__PURE__*/Object.freeze({
    default: _getAllKeys,
    __moduleExports: _getAllKeys
  });

  var require$$0$t = ( _getAllKeys$1 && _getAllKeys ) || _getAllKeys$1;

  var getAllKeys$1 = require$$0$t;

  /** Used to compose bitmasks for value comparisons. */
  var COMPARE_PARTIAL_FLAG$2 = 1;

  /** Used for built-in method references. */
  var objectProto$a = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$8 = objectProto$a.hasOwnProperty;

  /**
   * A specialized version of `baseIsEqualDeep` for objects with support for
   * partial deep comparisons.
   *
   * @private
   * @param {Object} object The object to compare.
   * @param {Object} other The other object to compare.
   * @param {number} bitmask The bitmask flags. See `baseIsEqual` for more details.
   * @param {Function} customizer The function to customize comparisons.
   * @param {Function} equalFunc The function to determine equivalents of values.
   * @param {Object} stack Tracks traversed `object` and `other` objects.
   * @returns {boolean} Returns `true` if the objects are equivalent, else `false`.
   */
  function equalObjects(object, other, bitmask, customizer, equalFunc, stack) {
    var isPartial = bitmask & COMPARE_PARTIAL_FLAG$2,
        objProps = getAllKeys$1(object),
        objLength = objProps.length,
        othProps = getAllKeys$1(other),
        othLength = othProps.length;

    if (objLength != othLength && !isPartial) {
      return false;
    }
    var index = objLength;
    while (index--) {
      var key = objProps[index];
      if (!(isPartial ? key in other : hasOwnProperty$8.call(other, key))) {
        return false;
      }
    }
    // Assume cyclic values are equal.
    var stacked = stack.get(object);
    if (stacked && stack.get(other)) {
      return stacked == other;
    }
    var result = true;
    stack.set(object, other);
    stack.set(other, object);

    var skipCtor = isPartial;
    while (++index < objLength) {
      key = objProps[index];
      var objValue = object[key],
          othValue = other[key];

      if (customizer) {
        var compared = isPartial
          ? customizer(othValue, objValue, key, other, object, stack)
          : customizer(objValue, othValue, key, object, other, stack);
      }
      // Recursively compare objects (susceptible to call stack limits).
      if (!(compared === undefined
            ? (objValue === othValue || equalFunc(objValue, othValue, bitmask, customizer, stack))
            : compared
          )) {
        result = false;
        break;
      }
      skipCtor || (skipCtor = key == 'constructor');
    }
    if (result && !skipCtor) {
      var objCtor = object.constructor,
          othCtor = other.constructor;

      // Non `Object` object instances with different constructors are not equal.
      if (objCtor != othCtor &&
          ('constructor' in object && 'constructor' in other) &&
          !(typeof objCtor == 'function' && objCtor instanceof objCtor &&
            typeof othCtor == 'function' && othCtor instanceof othCtor)) {
        result = false;
      }
    }
    stack['delete'](object);
    stack['delete'](other);
    return result;
  }

  var _equalObjects = equalObjects;

  var _equalObjects$1 = /*#__PURE__*/Object.freeze({
    default: _equalObjects,
    __moduleExports: _equalObjects
  });

  var getNative$3 = require$$0$b,
      root$6 = require$$0$1;

  /* Built-in method references that are verified to be native. */
  var DataView = getNative$3(root$6, 'DataView');

  var _DataView = DataView;

  var _DataView$1 = /*#__PURE__*/Object.freeze({
    default: _DataView,
    __moduleExports: _DataView
  });

  var getNative$4 = require$$0$b,
      root$7 = require$$0$1;

  /* Built-in method references that are verified to be native. */
  var Promise = getNative$4(root$7, 'Promise');

  var _Promise = Promise;

  var _Promise$1 = /*#__PURE__*/Object.freeze({
    default: _Promise,
    __moduleExports: _Promise
  });

  var getNative$5 = require$$0$b,
      root$8 = require$$0$1;

  /* Built-in method references that are verified to be native. */
  var Set = getNative$5(root$8, 'Set');

  var _Set = Set;

  var _Set$1 = /*#__PURE__*/Object.freeze({
    default: _Set,
    __moduleExports: _Set
  });

  var getNative$6 = require$$0$b,
      root$9 = require$$0$1;

  /* Built-in method references that are verified to be native. */
  var WeakMap = getNative$6(root$9, 'WeakMap');

  var _WeakMap = WeakMap;

  var _WeakMap$1 = /*#__PURE__*/Object.freeze({
    default: _WeakMap,
    __moduleExports: _WeakMap
  });

  var require$$0$u = ( _DataView$1 && _DataView ) || _DataView$1;

  var require$$2$d = ( _Promise$1 && _Promise ) || _Promise$1;

  var require$$3$7 = ( _Set$1 && _Set ) || _Set$1;

  var require$$4$6 = ( _WeakMap$1 && _WeakMap ) || _WeakMap$1;

  var DataView$1 = require$$0$u,
      Map$3 = require$$2$4,
      Promise$1 = require$$2$d,
      Set$1 = require$$3$7,
      WeakMap$1 = require$$4$6,
      baseGetTag$5 = require$$0$3,
      toSource$2 = require$$3$1;

  /** `Object#toString` result references. */
  var mapTag$2 = '[object Map]',
      objectTag$1 = '[object Object]',
      promiseTag = '[object Promise]',
      setTag$2 = '[object Set]',
      weakMapTag$1 = '[object WeakMap]';

  var dataViewTag$2 = '[object DataView]';

  /** Used to detect maps, sets, and weakmaps. */
  var dataViewCtorString = toSource$2(DataView$1),
      mapCtorString = toSource$2(Map$3),
      promiseCtorString = toSource$2(Promise$1),
      setCtorString = toSource$2(Set$1),
      weakMapCtorString = toSource$2(WeakMap$1);

  /**
   * Gets the `toStringTag` of `value`.
   *
   * @private
   * @param {*} value The value to query.
   * @returns {string} Returns the `toStringTag`.
   */
  var getTag = baseGetTag$5;

  // Fallback for data views, maps, sets, and weak maps in IE 11 and promises in Node.js < 6.
  if ((DataView$1 && getTag(new DataView$1(new ArrayBuffer(1))) != dataViewTag$2) ||
      (Map$3 && getTag(new Map$3) != mapTag$2) ||
      (Promise$1 && getTag(Promise$1.resolve()) != promiseTag) ||
      (Set$1 && getTag(new Set$1) != setTag$2) ||
      (WeakMap$1 && getTag(new WeakMap$1) != weakMapTag$1)) {
    getTag = function(value) {
      var result = baseGetTag$5(value),
          Ctor = result == objectTag$1 ? value.constructor : undefined,
          ctorString = Ctor ? toSource$2(Ctor) : '';

      if (ctorString) {
        switch (ctorString) {
          case dataViewCtorString: return dataViewTag$2;
          case mapCtorString: return mapTag$2;
          case promiseCtorString: return promiseTag;
          case setCtorString: return setTag$2;
          case weakMapCtorString: return weakMapTag$1;
        }
      }
      return result;
    };
  }

  var _getTag = getTag;

  var _getTag$1 = /*#__PURE__*/Object.freeze({
    default: _getTag,
    __moduleExports: _getTag
  });

  var require$$0$v = ( _Stack$1 && _Stack ) || _Stack$1;

  var require$$2$e = ( _equalByTag$1 && _equalByTag ) || _equalByTag$1;

  var require$$3$8 = ( _equalObjects$1 && _equalObjects ) || _equalObjects$1;

  var require$$4$7 = ( _getTag$1 && _getTag ) || _getTag$1;

  var Stack$1 = require$$0$v,
      equalArrays$2 = require$$3$5,
      equalByTag$1 = require$$2$e,
      equalObjects$1 = require$$3$8,
      getTag$1 = require$$4$7,
      isArray$4 = require$$2$9,
      isBuffer$2 = require$$3$6,
      isTypedArray$3 = require$$5$2;

  /** Used to compose bitmasks for value comparisons. */
  var COMPARE_PARTIAL_FLAG$3 = 1;

  /** `Object#toString` result references. */
  var argsTag$2 = '[object Arguments]',
      arrayTag$1 = '[object Array]',
      objectTag$2 = '[object Object]';

  /** Used for built-in method references. */
  var objectProto$b = Object.prototype;

  /** Used to check objects for own properties. */
  var hasOwnProperty$9 = objectProto$b.hasOwnProperty;

  /**
   * A specialized version of `baseIsEqual` for arrays and objects which performs
   * deep comparisons and tracks traversed objects enabling objects with circular
   * references to be compared.
   *
   * @private
   * @param {Object} object The object to compare.
   * @param {Object} other The other object to compare.
   * @param {number} bitmask The bitmask flags. See `baseIsEqual` for more details.
   * @param {Function} customizer The function to customize comparisons.
   * @param {Function} equalFunc The function to determine equivalents of values.
   * @param {Object} [stack] Tracks traversed `object` and `other` objects.
   * @returns {boolean} Returns `true` if the objects are equivalent, else `false`.
   */
  function baseIsEqualDeep(object, other, bitmask, customizer, equalFunc, stack) {
    var objIsArr = isArray$4(object),
        othIsArr = isArray$4(other),
        objTag = objIsArr ? arrayTag$1 : getTag$1(object),
        othTag = othIsArr ? arrayTag$1 : getTag$1(other);

    objTag = objTag == argsTag$2 ? objectTag$2 : objTag;
    othTag = othTag == argsTag$2 ? objectTag$2 : othTag;

    var objIsObj = objTag == objectTag$2,
        othIsObj = othTag == objectTag$2,
        isSameTag = objTag == othTag;

    if (isSameTag && isBuffer$2(object)) {
      if (!isBuffer$2(other)) {
        return false;
      }
      objIsArr = true;
      objIsObj = false;
    }
    if (isSameTag && !objIsObj) {
      stack || (stack = new Stack$1);
      return (objIsArr || isTypedArray$3(object))
        ? equalArrays$2(object, other, bitmask, customizer, equalFunc, stack)
        : equalByTag$1(object, other, objTag, bitmask, customizer, equalFunc, stack);
    }
    if (!(bitmask & COMPARE_PARTIAL_FLAG$3)) {
      var objIsWrapped = objIsObj && hasOwnProperty$9.call(object, '__wrapped__'),
          othIsWrapped = othIsObj && hasOwnProperty$9.call(other, '__wrapped__');

      if (objIsWrapped || othIsWrapped) {
        var objUnwrapped = objIsWrapped ? object.value() : object,
            othUnwrapped = othIsWrapped ? other.value() : other;

        stack || (stack = new Stack$1);
        return equalFunc(objUnwrapped, othUnwrapped, bitmask, customizer, stack);
      }
    }
    if (!isSameTag) {
      return false;
    }
    stack || (stack = new Stack$1);
    return equalObjects$1(object, other, bitmask, customizer, equalFunc, stack);
  }

  var _baseIsEqualDeep = baseIsEqualDeep;

  var _baseIsEqualDeep$1 = /*#__PURE__*/Object.freeze({
    default: _baseIsEqualDeep,
    __moduleExports: _baseIsEqualDeep
  });

  var require$$0$w = ( _baseIsEqualDeep$1 && _baseIsEqualDeep ) || _baseIsEqualDeep$1;

  var baseIsEqualDeep$1 = require$$0$w,
      isObjectLike$6 = require$$1$1;

  /**
   * The base implementation of `_.isEqual` which supports partial comparisons
   * and tracks traversed objects.
   *
   * @private
   * @param {*} value The value to compare.
   * @param {*} other The other value to compare.
   * @param {boolean} bitmask The bitmask flags.
   *  1 - Unordered comparison
   *  2 - Partial comparison
   * @param {Function} [customizer] The function to customize comparisons.
   * @param {Object} [stack] Tracks traversed `value` and `other` objects.
   * @returns {boolean} Returns `true` if the values are equivalent, else `false`.
   */
  function baseIsEqual(value, other, bitmask, customizer, stack) {
    if (value === other) {
      return true;
    }
    if (value == null || other == null || (!isObjectLike$6(value) && !isObjectLike$6(other))) {
      return value !== value && other !== other;
    }
    return baseIsEqualDeep$1(value, other, bitmask, customizer, baseIsEqual, stack);
  }

  var _baseIsEqual = baseIsEqual;

  var _baseIsEqual$1 = /*#__PURE__*/Object.freeze({
    default: _baseIsEqual,
    __moduleExports: _baseIsEqual
  });

  var require$$0$x = ( _baseIsEqual$1 && _baseIsEqual ) || _baseIsEqual$1;

  var baseIsEqual$1 = require$$0$x;

  /**
   * Performs a deep comparison between two values to determine if they are
   * equivalent.
   *
   * **Note:** This method supports comparing arrays, array buffers, booleans,
   * date objects, error objects, maps, numbers, `Object` objects, regexes,
   * sets, strings, symbols, and typed arrays. `Object` objects are compared
   * by their own, not inherited, enumerable properties. Functions and DOM
   * nodes are compared by strict equality, i.e. `===`.
   *
   * @static
   * @memberOf _
   * @since 0.1.0
   * @category Lang
   * @param {*} value The value to compare.
   * @param {*} other The other value to compare.
   * @returns {boolean} Returns `true` if the values are equivalent, else `false`.
   * @example
   *
   * var object = { 'a': 1 };
   * var other = { 'a': 1 };
   *
   * _.isEqual(object, other);
   * // => true
   *
   * object === other;
   * // => false
   */
  function isEqual(value, other) {
    return baseIsEqual$1(value, other);
  }

  var isEqual_1 = isEqual;

  var toPrecision = function (num, precision) {
    var p = precision | 0;
    return p > 0 ? parseFloat(num.toFixed(p)) : num
  };

  var VueCtrlComponent = {
    name: 'v-ctrl',
    abstract: true,
    props: {
      direction: {
        type: String,
        default: 'h',
        validator: function validator (val) {
          return ['v', 'h', 'vh', 'hv'].indexOf(val) > -1
        }
      },
      throttle: {
        type: Number,
        default: 80
      },
      precision: {
        type: Number
      }
    },

    methods: {
      msdown: function msdown (e) {
        e.preventDefault();
        document.addEventListener('mousemove', this.msmove);
        document.addEventListener('mouseup', this.msup);
        this.next(e);
      },
    
      msmove: function msmove (e) {
        e.preventDefault();
        this.next(e);
      },
    
      msup: function msup (e) {
        this.next(e);
        document.removeEventListener('mousemove', this.msmove);
        document.removeEventListener('mouseup', this.msup);
      },
    
      notify: function notify (val) {
        if (isEqual_1(this.memo, val) === false) {
          this.memo = val;
          this.$emit('change', val);
        }
      },

      next: function next (ref) {
        if ( ref === void 0 ) { ref = {}; }
        var clientX = ref.clientX; if ( clientX === void 0 ) { clientX = 0; }
        var clientY = ref.clientY; if ( clientY === void 0 ) { clientY = 0; }

        var ref$1 = this;
        var direction = ref$1.direction;
        var adjust = ref$1.adjust;
        var rect = this.$el.getBoundingClientRect();

        var left = rect.left;
        var width = rect.width;
        var deltaX = clientX - left;
        var x = adjust(deltaX / width);

        if (direction === 'h') {
          return this.notify(x)
        }
    
        var top = rect.top;
        var height = rect.height;
        var deltaY = clientY - top;
        var y = adjust(deltaY / height);

        if (direction === 'v') {
          return this.notify(y)
        }

        // both direction
        this.notify([x, y]);
      },

      adjust: function adjust (num) {
        return toPrecision(clamp_1(num, 0, 1), this.precision)
      }
    },

    render: function render (h) {
      return this.$slots.default[0]
    },

    created: function created () {
      var ref = this;
      var msdown = ref.msdown;
      var msmove = ref.msmove;

      this.msdown = msdown.bind(this);
      this.msmove = throttle_1(msmove.bind(this), this.throttle);

      this.memo = null;
    },

    mounted: function mounted () {
      this.$el.addEventListener('mousedown', this.msdown);
    },

    destroyed: function destroyed () {
      this.$el.removeEventListener('mousedown', this.msdown);
    },

    install: function install () {
      Vue.component(VueCtrlComponent.name, VueCtrlComponent);
    }
  };

  if (typeof window !== 'undefined' && window.Vue) {
    Vue.use(VueCtrlComponent);
  }

  var index$2 = { VueCtrlComponent: VueCtrlComponent };

  var colorModes = Object.freeze({
    rgba: ['r', 'g', 'b', 'a'],
    hsla: ['h', 's', 'l', 'a'],
    hex: ['hex']
  });

  var VColorComponent = {render: function(){var _vm=this;var _h=_vm.$createElement;var _c=_vm._self._c||_h;return _c('div',{staticClass:"cp__wrapper"},[_c('v-ctrl',{attrs:{"direction":"vh","precision":2,"throttle":80},on:{"change":_vm.onSaturationChange}},[_c('div',{staticClass:"cp__v-ctrl cp__saturation"},[_c('div',{staticClass:"msk-hue",style:(_vm.styles.saturationPane)}),_vm._v(" "),_c('div',{staticClass:"msk-white"}),_vm._v(" "),_c('div',{staticClass:"msk-black"}),_vm._v(" "),_c('p',{staticClass:"cp__thumb",style:(_vm.styles.saturationThumb)})])]),_vm._v(" "),_c('div',{staticClass:"cp__ctrl-pane"},[_c('div',[_c('div',{staticClass:"cp__preview"},[_c('div',{style:(_vm.styles.preview)})]),_vm._v(" "),_c('div',{staticClass:"cp__tracks"},[_c('v-ctrl',{attrs:{"direction":"h","precision":2,"throttle":80},on:{"change":_vm.onHueChange}},[_c('div',{staticClass:"cp__v-ctrl cp__ctrl-bar cp__ctrl-hue"},[_c('div',{staticClass:"cp__thumb",style:(_vm.styles.hueThumb)})])]),_vm._v(" "),_c('v-ctrl',{attrs:{"direction":"h","precision":2,"throttle":80},on:{"change":_vm.onAlphaChange}},[_c('div',{staticClass:"cp__v-ctrl cp__ctrl-alpha"},[_c('div',{staticClass:"cp__thumb",style:(_vm.styles.alphaThumb)}),_vm._v(" "),_c('div',{staticClass:"cp__ctrl-bar",style:(_vm.styles.alphaTrack)})])])],1)]),_vm._v(" "),_c('div',{staticStyle:{"margin-top":"10px"}},[_c('div',{staticClass:"cp__fm-fields"},_vm._l((_vm.colorModes[_vm.currentMode]),function(k){return _c('div',{key:k},[_c('div',{staticStyle:{"position":"relative"}},[_c('input',{attrs:{"type":_vm.constrains[k].type,"maxlength":_vm.constrains[k].maxlength},domProps:{"value":_vm.colorModel[k]},on:{"change":function($event){_vm.handleInput(k, $event);}}}),_vm._v(" "),_c('span',[_vm._v(_vm._s(k))])])])})),_vm._v(" "),_c('div',{staticClass:"cp__fm-switcher"},[_c('div',{on:{"click":function($event){_vm.changecurrentMode();}}},[_c('svg',{attrs:{"viewBox":"0 0 24 24"}},[_c('path',{attrs:{"fill":"#333","d":"M12,5.83L15.17,9L16.58,7.59L12,3L7.41,7.59L8.83,9L12,5.83Z"}}),_vm._v(" "),_c('path',{attrs:{"fill":"#333","d":"M12,18.17L8.83,15L7.42,16.41L12,21L16.59,16.41L15.17,15Z"}})])])])])])],1)},staticRenderFns: [],
    name: 'color-picker',
    props: {
      color: {
        type: String,
        default: '#ff0000'
      }
    },

    components: {
      'v-ctrl': index$2.VueCtrlComponent
    },

    data: function data () {
      var ref = this;
      var color = ref.color;

      var commonNumber = {
        type: 'number',
        maxlength: 3,
      };
      var percentValue = {
        type: 'string',
        maxlength: 4
      };

      return Object.assign({}, this.digestProp(color),
        {currentMode: getColorType(color),
        colorModes: colorModes,
        colorModel: {
          hex: '',
          r: '',
          g: '',
          b: '',
          h: '',
          s: '',
          l: '',
          a: ''
        },
        constrains: {
          r: commonNumber,
          g: commonNumber,
          b: commonNumber,
          h: commonNumber,
          s: percentValue,
          l: percentValue,
          a: {
            type: 'number',
            maxlength: 4
          },
          hex: {
            type: 'string',
            maxlength: 9
          }
        }})
    },

    watch: {
      color: {
        immediate: true,
        handler: function handler (newVal, oldVal) {
          if (newVal !== oldVal) {
            index(this, this.digestProp(newVal));
          }
        }
      },
      rgba: {
        immediate: true,
        handler: function handler (newVal, oldVal) {
          if (("" + newVal) !== ("" + oldVal)) {
            this.emitChange();
          }
        }
      }
    },

    computed: {
      hsva: function hsva () {
        var ref = this;
        var hue = ref.hue;
        var alpha = ref.alpha;
        var ref_saturation = ref.saturation;
        var x = ref_saturation.x;
        var y = ref_saturation.y;
        return [
          hue * 360,
          x * 100,
          (1 - y) * 100,
          alpha
        ]
      },

      rgba: function rgba () {
        var ref = this;
        var alpha = ref.alpha;
        var hsva = ref.hsva;
        var ref$1 = hsv2rgb_1(hsva);
        var r = ref$1[0];
        var g = ref$1[1];
        var b = ref$1[2];
        return [
          Math.round(r),
          Math.round(g),
          Math.round(b),
          alpha
        ]
      },

      hsla: function hsla () {
        var ref = this;
        var alpha = ref.alpha;
        var hsva = ref.hsva;
        var ref$1 = hsv2hsl_1(hsva);
        var h = ref$1[0];
        var s = ref$1[1];
        var l = ref$1[2];
        return [
          Math.round(h),
          ((Math.round(s)) + "%"),
          ((Math.round(l)) + "%"),
          alpha
        ]
      },

      hex: function hex () {
        return rgb2hex_1(this.rgba)
      },

      previewBorderColor: function previewBorderColor () {
        var ref = this.rgba;
        var r = ref[0];
        var g = ref[1];
        var b = ref[2];
        if ((r + g + b) / 3 > 235) {
          return "rgba(160,160,160,0.8)"
        }
        return 'transparent'
      },

      styles: function styles () {
        var ref = this;
        var rgba = ref.rgba;
        var alpha = ref.alpha;
        var hue = ref.hue;
        var saturation = ref.saturation;
        var strRGB = rgba.slice(0, 3).join(', ');

        var strHueRGB = hsl2rgb_1([hue * 360, 100, 50])
          .map(function (v) { return Math.round(v); })
          .join(', ');

        return {
          preview: {
            backgroundColor: ("rgba(" + (rgba.join(', ')) + ")"),
            borderColor: this.previewBorderColor
          },
          saturationPane: {
            backgroundColor: ("rgb(" + strHueRGB + ")")
          },
          saturationThumb: {
            left: toPercent(saturation.x),
            top: toPercent(saturation.y)
          },
          alphaTrack: {
            backgroundImage: "linear-gradient(to right, " +
              "rgba(" + strRGB + ", 0) 0%, rgb(" + strRGB + ") 100%)"
          },
          alphaThumb: {
            left: toPercent(alpha)
          },
          hueThumb: {
            left: toPercent(1 - hue)
          }
        }
      }
    },

    methods: {
      digestProp: function digestProp (val) {
        var rgba = index$1(val);
        var alpha = rgba[3] == null ? 1 : rgba[3];
        var ref = rgb2hsv_1(rgba);
        var hue = ref[0];
        var saturation = ref[1];
        var value = ref[2];

        // format of alpha: `.2f`
        // according to Chrome DevTool
        var _alpha = parseFloat(alpha.toFixed(2));

        return {
          alpha: _alpha,
          hue: hue / 360,
          saturation: {
            x: saturation / 100,
            y: 1 - value / 100
          }
        }
      },
      onSaturationChange: function onSaturationChange (ref) {
        var x = ref[0];
        var y = ref[1];

        this.saturation = { x: x, y: y };
      },
      onHueChange: function onHueChange (e) {
        this.hue = 1 - e;
      },
      onAlphaChange: function onAlphaChange (e) {
        // format of alpha: `.2f`
        // according to Chrome DevTool
        this.alpha = parseFloat(e.toFixed(2));
      },

      emitChange: function emitChange () {
        var ref = this;
        var alpha = ref.alpha;
        var hex = ref.hex;
        var rgba = ref.rgba;
        var hsla = ref.hsla;
        var hexVal = simplifyHex(
          alpha === 1 ? hex.slice(0, 7) : hex
        );

        this.$emit('change', {
          rgba: rgba,
          hsla: hsla,
          hex: hexVal
        });

        // this ensures that every component in
        // our model is up to date
        var h = hsla[0];
        var s = hsla[1];
        var l = hsla[2];
        var r = rgba[0];
        var g = rgba[1];
        var b = rgba[2];
        var shortHex = index(this.colorModel, {
          r: r, g: g, b: b, h: h, s: s, l: l,
          a: alpha,
          hex: hexVal
        });
      },

      changecurrentMode: function changecurrentMode () {
        var modes = Object.keys(this.colorModes);
        var index$$1 = modes.indexOf(this.currentMode);
        this.currentMode = modes[(index$$1 + 1) % modes.length];
      },

      handleInput: function handleInput (type, event) {
        var ref = this;
        var currentMode = ref.currentMode;
        var colorModel = ref.colorModel;
        var value = event.target.value;
        var num = Number(value);
        var changed = false;

        switch (type) {
          case 'a':
            if (colorModel[type] !== num && !isNaN(num)) {
              colorModel[type] = clamp_1(num, 0, 1);
              changed = true;
            }
            break

          case 'r':
          case 'g':
          case 'b':
            if (colorModel[type] !== num && !isNaN(num)) {
              colorModel[type] = clamp_1(num, 0, 255) | 0;
              changed = true;
            }
            break

          case 'h':
            if (colorModel[type] !== num && !isNaN(num)) {
              colorModel[type] = clamp_1(num, 0, 360) | 0;
              changed = true;
            }
            break

          case 's':
          case 'l':
            if (value.slice(-1) === '%' && colorModel[type] !== value) {
              num = parseFloat(value);
              colorModel[type] = (clamp_1(num, 0, 360) | 0) + "%";
              changed = true;
            }
            break

          case 'hex':
            if (value[0] === '#') {
              if (colorModel[type] !== value && index$1(value).every(function (i) { return !isNaN(i); })) {
                colorModel[type] = simplifyHex(value);
                changed = true;
              }
            }
            break
        }

        if (changed) {
          var h = colorModel.h;
          var s = colorModel.s;
          var l = colorModel.l;
          var r = colorModel.r;
          var g = colorModel.g;
          var b = colorModel.b;
          var a = colorModel.a;
          var hex = colorModel.hex;
          var literal = hex;

          if (currentMode === 'rgba') {
            literal = "rgba(" + ([r, g, b, a]) + ")";
          } else if (currentMode === 'hsla') {
            literal = "hsla(" + ([h, s, l, a]) + ")";
          }

          index(this, this.digestProp(literal));
        }
      }
    },

    created: function created () {
      this.handleInput = debounce_1(this.handleInput.bind(this), 50);
    }
  }

  function toPercent (n, precision) {
    if ( precision === void 0 ) precision = 3;

    // eslint-disable-next-line
    var num = (n * 100).toPrecision(precision | 0);
    return (num + "%")
  }

  function getColorType (color) {
    if (color[0] === '#') {
      return 'hex'
    }

    if (color.indexOf('rgb') === 0) {
      return 'rgba'
    }

    if (color.indexOf('hsl') === 0) {
      return 'hsla'
    }

    browser(false, (color + " is not valid color value!"));
  }

  function simplifyHex (val) {
    return val.replace(/#([0-9a-f])\1([0-9a-f])\2([0-9a-f])\3([0-9a-f]?)\4$/, '#$1$2$3$4')
  }

  VColorComponent.install = function (Vue) {
    Vue.config.devtools = "production" !== 'production';
    Vue.component(VColorComponent.name, VColorComponent);
  };

  return VColorComponent;

})));
//# sourceMappingURL=index.js.map
