import { compToString, RGBAToHex } from './common.js';

const options = {
  moduleCache: {
    vue: Vue
  },
  async getFile(url) {
    const res = await fetch(url);
    if (!res.ok)
      throw Object.assign(new Error(res.statusText + ' ' + url), { res });
    return {
      getContentData: asBinary => asBinary ? res.arrayBuffer() : res.text(),
    }
  },
  addStyle(textContent) {
    const style = Object.assign(document.createElement('style'), { textContent });
    const ref = document.head.getElementsByTagName('style')[0] || null;
    document.head.insertBefore(style, ref);
  },
}

const { loadModule } = window['vue3-sfc-loader'];

function now() {
  return (Date.now || function () { return new Date().getTime(); })();
}

function restArguments(func, startIndex) {
  startIndex = startIndex == null ? func.length - 1 : +startIndex;
  return function () {
    var length = Math.max(arguments.length - startIndex, 0),
      rest = Array(length),
      index = 0;
    for (; index < length; index++) {
      rest[index] = arguments[index + startIndex];
    }
    switch (startIndex) {
      case 0: return func.call(this, rest);
      case 1: return func.call(this, arguments[0], rest);
      case 2: return func.call(this, arguments[0], arguments[1], rest);
    }
    var args = Array(startIndex + 1);
    for (index = 0; index < startIndex; index++) {
      args[index] = arguments[index];
    }
    args[startIndex] = rest;
    return func.apply(this, args);
  };
}

function shouldOpenEditor(comp) {
  return [
    'ecs::Object',
    'ecs::Array',
    'ecs::IntList',
    'ecs::UInt8List',
    'ecs::UInt16List',
    'ecs::UInt32List',
    'ecs::UInt64List',
    'ecs::StringList',
    'ecs::EidList',
    'ecs::FloatList',
    'ecs::Point2List',
    'ecs::Point3List',
    'ecs::Point4List',
    'ecs::IPoint2List',
    'ecs::IPoint3List',
    'ecs::IPoint4List',
    'ecs::BoolList',
    'ecs::ColorList',
    'ecs::TMatrixList',
    'ecs::Int64List',
  ].indexOf(comp.type) >= 0;
}

function getAttrValue(attr) {
  if (attr.type === 'E3DCOLOR') {
    const toh = v => ('0' + v.toString(16)).substr(-2);
    return '#' + toh(attr.value.r) + toh(attr.value.g) + toh(attr.value.b);
  }
  return attr.value;
}

function startEdit(attr) {
  console.log('startEdit', attr);

  attr._edit = true;
  attr._new_value = null;
}

function cancelEdit(attr) {
  attr._edit = false;
  attr._new_value = null;
}

function saveEdit(row, attr) {
  if (row.eid === undefined) {
    return;
  }

  console.log('saveEdit', attr, attr._new_value);

  attr._edit = false;
  if (attr._new_value !== null && attr._new_value !== undefined) {
    const nv = attr.type === 'TMatrix' ? Array.from(attr._new_value) : attr._new_value;
    this.ecsService.setEntityAttribute(row.eid, attr, nv);
  }
}

function updateValue(ev, row, val, attr, key = null) {
  console.log('updateValue', ev, row, val, attr, key);

  const setByKey = v => {
    let res = attr.type === 'TMatrix' ? [] : {};
    let curVal = attr._new_value === null ? attr.value : attr._new_value;
    for (let k in curVal) {
      res[k] = curVal[k];
    }
    res[key] = +val;
    return res;
  };

  const convert =
  {
    E3DCOLOR: v => {
      const toi = (v, s) => parseInt(v.substring(1 + s * 2, 3 + s * 2), 16);
      return {
        r: toi(attr._color, 0), g: toi(attr._color, 1), b: toi(attr._color, 2), a: toi(attr._color, 3)
      }
    },
    int: v => setByKey(v),
    float: v => setByKey(v),
    Point2: v => setByKey(v),
    Point3: v => setByKey(v),
    Point4: v => setByKey(v),
    TMatrix: v => setByKey(v),
  };

  if (attr.type === 'E3DCOLOR' && Array.isArray(val)) {
    attr._color = RGBAToHex(val);
  }

  const valueToSet = convert[attr.type] ? convert[attr.type](val) : val;

  console.log('updateValue', attr, val, valueToSet);

  attr._new_value = valueToSet;

  console.log(ev);
  if (ev.code === "Enter" || ev.type === "change") {
    this.saveEdit(row, attr);
  }
  else if (ev.code === "Escape") {
    this.cancelEdit(attr);
  }
}

function sortValues(values, key, getter = null) {
  if (values._sortedKey === undefined) {
    values._sortedKey = {};
  }

  if (values._sortedKey[key] === undefined) {
    values._sortedKey[key] = {up: false, down: false};
  }

  const getValue = getter ? getter : v => v[key];

  if (values._sortedKey[key].up) {
    values._sortedKey[key].up = false;
    values._sortedKey[key].down = true;
    values.sort((a, b) => {
      if (getValue(a) < getValue(b)) return 1;
      if (getValue(a) > getValue(b)) return -1;
      return 0;
    });
  }
  else if (values._sortedKey[key].down) {
    values._sortedKey[key].up = false;
    values._sortedKey[key].down = false;
    values.sort((a, b) => {
      if (a.id < b.id) return -1;
      if (a.id > b.id) return 1;
      return 0;
    });
  }
  else {
    values._sortedKey[key].up = true;
    values._sortedKey[key].down = false;
    values.sort((a, b) => {
      if (getValue(a) < getValue(b)) return -1;
      if (getValue(a) > getValue(b)) return 1;
      return 0;
    });
  }
}

export default {
  loadModule(path) {
    return loadModule(path, options)
  },
  debounce(func, wait, immediate) {
    var timeout, previous, args, result, context;

    var later = function () {
      var passed = now() - previous;
      if (wait > passed) {
        timeout = setTimeout(later, wait - passed);
      } else {
        timeout = null;
        if (!immediate) result = func.apply(context, args);
        if (!timeout) args = context = null;
      }
    };

    var debounced = restArguments(function (_args) {
      context = this;
      args = _args;
      previous = now();
      if (!timeout) {
        timeout = setTimeout(later, wait);
        if (immediate) result = func.apply(context, args);
      }
      return result;
    });

    debounced.cancel = function () {
      clearTimeout(timeout);
      timeout = args = context = null;
    };

    return debounced;
  },
  formatValue: compToString,
  compToString,
  RGBAToHex,
  shouldOpenEditor,
  getAttrValue,
  startEdit,
  cancelEdit,
  saveEdit,
  updateValue,
  sortValues,
}