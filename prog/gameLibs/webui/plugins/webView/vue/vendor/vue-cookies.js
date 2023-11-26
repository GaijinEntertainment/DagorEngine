/**
 * Vue3 Cookies v1.0.0
 *
 * Forked from
 * https://github.com/cmp-cc/vue-cookies
 * And changed format to support Vue.js 3
 *
 */
const { reactive } = Vue;
var defaultConfig = {
    expireTimes: "1d",
    path: "; path=/",
    domain: "",
    secure: false,
    sameSite: "; SameSite=Lax",
};
var VueCookiesManager = /** @class */ (function () {
    function VueCookiesManager() {
        this.current_default_config = defaultConfig;
    }
    VueCookiesManager.prototype.config = function (config) {
        for (var propertyName in this.current_default_config) {
            this.current_default_config[propertyName] = config[propertyName]
                ? config[propertyName]
                : defaultConfig[propertyName];
        }
    };
    VueCookiesManager.prototype.get = function (keyName) {
        var value = decodeURIComponent(document.cookie.replace(new RegExp("(?:(?:^|.*;)\\s*" +
            encodeURIComponent(keyName).replace(/[\-\.\+\*]/g, "\\$&") +
            "\\s*\\=\\s*([^;]*).*$)|^.*$"), "$1")) || null;
        if (value &&
            value.substring(0, 1) === "{" &&
            value.substring(value.length - 1, value.length) === "}") {
            try {
                value = JSON.parse(value);
            }
            catch (e) {
                return value;
            }
        }
        return value;
    };
    VueCookiesManager.prototype.set = function (keyName, value, expireTimes, path, domain, secure, sameSite) {
        if (!keyName) {
            throw new Error("Cookie name is not found in the first argument.");
        }
        else if (/^(?:expires|max-age|path|domain|secure|SameSite)$/i.test(keyName)) {
            throw new Error('Cookie name illegality. Cannot be set to ["expires","max-age","path","domain","secure","SameSite"]\t current key name: ' +
                keyName);
        }
        // support json object
        if (value && value.constructor === Object) {
            value = JSON.stringify(value);
        }
        var _expires = "";
        if (expireTimes == undefined) {
            expireTimes = this.current_default_config.expireTimes
                ? this.current_default_config.expireTimes
                : "";
        }
        if (expireTimes && expireTimes != 0) {
            switch (expireTimes.constructor) {
                case Number:
                    if (expireTimes === Infinity || expireTimes === -1)
                        _expires = "; expires=Fri, 31 Dec 9999 23:59:59 GMT";
                    else
                        _expires = "; max-age=" + expireTimes;
                    break;
                case String:
                    if (/^(?:\d+(y|m|d|h|min|s))$/i.test(expireTimes)) {
                        // get capture number group
                        var _expireTime = expireTimes.replace(/^(\d+)(?:y|m|d|h|min|s)$/i, "$1");
                        // get capture type group , to lower case
                        switch (expireTimes
                            .replace(/^(?:\d+)(y|m|d|h|min|s)$/i, "$1")
                            .toLowerCase()) {
                            // Frequency sorting
                            case "m":
                                _expires = "; max-age=" + +_expireTime * 2592000;
                                break; // 60 * 60 * 24 * 30
                            case "d":
                                _expires = "; max-age=" + +_expireTime * 86400;
                                break; // 60 * 60 * 24
                            case "h":
                                _expires = "; max-age=" + +_expireTime * 3600;
                                break; // 60 * 60
                            case "min":
                                _expires = "; max-age=" + +_expireTime * 60;
                                break; // 60
                            case "s":
                                _expires = "; max-age=" + _expireTime;
                                break;
                            case "y":
                                _expires = "; max-age=" + +_expireTime * 31104000;
                                break; // 60 * 60 * 24 * 30 * 12
                            default:
                                new Error('unknown exception of "set operation"');
                        }
                    }
                    else {
                        _expires = "; expires=" + expireTimes;
                    }
                    break;
                case Date:
                    _expires = "; expires=" + expireTimes.toUTCString();
                    break;
            }
        }
        document.cookie =
            encodeURIComponent(keyName) +
                "=" +
                encodeURIComponent(value) +
                _expires +
                (domain
                    ? "; domain=" + domain
                    : this.current_default_config.domain
                        ? this.current_default_config.domain
                        : "") +
                (path
                    ? "; path=" + path
                    : this.current_default_config.path
                        ? this.current_default_config.path
                        : "; path=/") +
                (secure == undefined
                    ? this.current_default_config.secure
                        ? "; Secure"
                        : ""
                    : secure
                        ? "; Secure"
                        : "") +
                (sameSite == undefined
                    ? this.current_default_config.sameSite
                        ? "; SameSute=" + this.current_default_config.sameSite
                        : ""
                    : sameSite
                        ? "; SameSite=" + sameSite
                        : "");
        return this;
    };
    VueCookiesManager.prototype.remove = function (keyName, path, domain) {
        if (!keyName || !this.isKey(keyName)) {
            return false;
        }
        document.cookie =
            encodeURIComponent(keyName) +
                "=; expires=Thu, 01 Jan 1970 00:00:00 GMT" +
                (domain
                    ? "; domain=" + domain
                    : this.current_default_config.domain
                        ? this.current_default_config.domain
                        : "") +
                (path
                    ? "; path=" + path
                    : this.current_default_config.path
                        ? this.current_default_config.path
                        : "; path=/") +
                "; SameSite=Lax";
        return true;
    };
    VueCookiesManager.prototype.isKey = function (keyName) {
        return new RegExp("(?:^|;\\s*)" +
            encodeURIComponent(keyName).replace(/[\-\.\+\*]/g, "\\$&") +
            "\\s*\\=").test(document.cookie);
    };
    VueCookiesManager.prototype.keys = function () {
        if (!document.cookie)
            return [];
        var _keys = document.cookie
            .replace(/((?:^|\s*;)[^\=]+)(?=;|$)|^\s*|\s*(?:\=[^;]*)?(?:\1|$)/g, "")
            .split(/\s*(?:\=[^;]*)?;\s*/);
        for (var _index = 0; _index < _keys.length; _index++) {
            _keys[_index] = decodeURIComponent(_keys[_index]);
        }
        return _keys;
    };
    return VueCookiesManager;
}());
export default {
    install: function (app, options) {
        app.config.globalProperties.$cookies = new VueCookiesManager();
        if (options) {
            app.config.globalProperties.$cookies.config(options);
        }
    },
};
var GLOBAL_COOKIES_MANAGER = null;
function globalCookiesConfig(options) {
    if (GLOBAL_COOKIES_MANAGER == null) {
        GLOBAL_COOKIES_MANAGER = new VueCookiesManager();
    }
    GLOBAL_COOKIES_MANAGER.config(options);
}
function useCookies() {
    if (GLOBAL_COOKIES_MANAGER == null) {
        GLOBAL_COOKIES_MANAGER = new VueCookiesManager();
    }
    var cookies = reactive(GLOBAL_COOKIES_MANAGER);
    return { cookies: cookies };
}
export { globalCookiesConfig, useCookies };
