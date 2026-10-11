import { benchmark } from "./profile.js";

const { isArray } = Array;
const ROBJ_TEXT = 0;
const ROBJ_FRAME = 1;
const ROBJ_SOLID = 2;
const ALIGN_CENTER = 0;
const FLOW_VERTICAL = 0;
const Behaviors = Object.freeze({
  Button: 0,
});

const SCREEN_WIDTH = 1920.0;
const SCREEN_HEIGHT = 1080.0;
const COMPONENTS = 2000;
const FRAMES = 100;
const BORDERS = true;
const DEF_FLEX = {};

class Watched {
  constructor(value = null) {
    this.value = value;
  }

  get() {
    return this.value;
  }

  modify(f) {
    this.value = f(this.value);
  }
}

let seed = 42;
function rint(max) {
  seed = seed * 16807 % 2147483647;
  return seed % max;
}

const sw = (val) => Math.trunc(val / 100.0 * SCREEN_WIDTH);
const sh = (val) => Math.trunc(val / 100.0 * SCREEN_HEIGHT);
const Color = (r, g, b, a = 255) => ((a << 24) >>> 0) + (r << 16) + (g << 8) + b;
const hdpx = (val) => val * SCREEN_HEIGHT / sh(100);
const flex = (val = null) => (val === null ? DEF_FLEX : { val });

const BORDER_WIDTH = [1, 1, 1, 1];
const PADDING = [hdpx(4), hdpx(5)];
const MARGIN = [hdpx(5), hdpx(3)];
const BLACK = Color(0, 0, 0);

let withText = 0;

function simpleComponent(i, watch) {
  const pos = [sw(rint(80)), sh(rint(80))];
  const size = [sh(rint(15) + 2), sh(rint(15) + 2)];
  const color = Color(rint(256), rint(256), rint(256));
  const textCanBePlaced = size[0] > hdpx(150);
  const addText = rint(5) === 1;
  const children = [];
  if (BORDERS) {
    children.push(() => ({
      borderWidth: BORDER_WIDTH,
      rendObj: ROBJ_FRAME,
      size: flex(),
    }));
    if (textCanBePlaced) {
      withText++;
      children.push(() => ({
        watch,
        rendObj: ROBJ_TEXT,
        text: addText || !watch.get() ? `${i}${watch.get()}` : null,
      }));
    }
  }
  return () => ({
    rendObj: ROBJ_SOLID,
    size,
    key: {},
    pos,
    valign: ALIGN_CENTER,
    halign: ALIGN_CENTER,
    padding: PADDING,
    margin: MARGIN,
    color: watch.get() ? color : BLACK,
    behavior: Behaviors.Button,
    onClick: () => watch.modify((v) => !v),
    watch,
    children,
  });
}

const components = Array.from({ length: COMPONENTS }, (_, i) => simpleComponent(i, new Watched(i % 3 === 0)));

const root = () => ({
  size: flex(),
  flow: FLOW_VERTICAL,
  children: components,
});

function testUi(entry) {
  if (entry == null)
    return true;
  if (typeof entry === "function")
    entry = entry();
  if (entry !== null && typeof entry === "object" && !isArray(entry)) {
    if (!("children" in entry))
      return true;
    const { children } = entry;
    if (isArray(children)) {
      for (let i = 0, n = children.length; i < n; i++)
        if (!testUi(children[i]))
          return false;
      return true;
    }
    return testUi(children);
  }
  return false;
}

function frames() {
  for (let f = 0; f < FRAMES; f++)
    testUi(root);
  return withText;
}

benchmark("profile darg ui", 20, frames);
