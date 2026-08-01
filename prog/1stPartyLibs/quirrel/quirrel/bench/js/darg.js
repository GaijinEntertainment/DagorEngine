const rand = Math.random;
const RAND_MAX = Math.RAND_MAX;

const CompsToUpdateEachFrame = 30;
const ROBJ_TEXT = 0;
const ROBJ_FRAME = 1;
const ROBJ_SOLID = 2;
const ALIGN_CENTER = 0;

const Behaviors = {
  Button: 0,
};

const FLOW_VERTICAL = 0;

class Watched {
  constructor(value = null) {
    this.value = value;
  }

  get() {
    return this.value;
  }

  modify(callback) {
    this.value = callback(this.value);
  }
}

function rint(max) {
  return Math.floor((rand() / RAND_MAX) * max);
}

const screenWidth = 1920.0;
const screenHeight = 1080.0;

function sw(val) {
  return Math.floor((val / 100.0) * screenWidth);
}

function sh(val) {
  return Math.floor((val / 100.0) * screenHeight);
}

function Color(r, g, b, a = 255) {
  return ((a << 24) >>> 0) + (r << 16) + (g << 8) + b;
}

function hdpx(val) {
  return (val / sh(100)) * screenHeight;
}

const defflex = {};

function flex(val = null) {
  return val == null ? defflex : { val: val };
}

const componentsNum = 2000; // amount of components
const borders = true; // show borders on each element

function simpleComponent(i, watch) {
  const pos = [sw(rand() * 80 / RAND_MAX), sh(rand() * 80 / RAND_MAX)];
  const size = [sh(rand() * 15 / RAND_MAX + 2), sh(rand() * 15 / RAND_MAX + 2)];
  const color = Color(rand() * 255 / RAND_MAX, rand() * 255 / RAND_MAX, rand() * 255 / RAND_MAX);
  const textCanBePlaced = size[0] > hdpx(150);
  const addText = rint(5) === 1;
  const text = textCanBePlaced ? () => ({
    watch: watch,
    rendObj: ROBJ_TEXT,
    text: addText || !watch.get() ? ("" + i + watch.get()) : null
  }) : null;

  const children = [];

  if (borders) {
    children.push(() => ({
      borderWidth: [1, 1, 1, 1],
      rendObj: ROBJ_FRAME,
      size: flex()
    }));
    if (text) children.push(text);
  }

  return () => ({
    rendObj: ROBJ_SOLID,
    size: size,
    key: {},
    pos: pos,
    valign: ALIGN_CENTER,
    halign: ALIGN_CENTER,
    padding: [hdpx(4), hdpx(5)],
    margin: [hdpx(5), hdpx(3)],
    color: watch.get() ? color : Color(0, 0, 0),
    behavior: Behaviors.Button,
    onClick: () => watch.modify((v) => !v),
    watch: watch,
    children: children,
  });
}

let num = 0;
const children = [];
for (let i = 0; i < componentsNum; ++i) {
  children.push(simpleComponent(i, new Watched(i % 3 === 0)));
}

function benchmark() {
  return {
    size: flex(),
    flow: FLOW_VERTICAL,
    children: children,
  };
}

function testUi(entry) {
  if (entry == null) return true;
  if (typeof entry === "function") entry = entry();
  const t = typeof entry;
  if (t === "object") {
    if (!("children" in entry)) return true;
    if (Array.isArray(entry.children)) {
      for (const child of entry.children) {
        if (!testUi(child)) return false;
      }
      return true;
    }
    return testUi(entry.children);
  }
  return false;
}


///tests
const Frames = 100; // offline only
const testsNum = 10; // offline only

function timeStamp() {
   return Date.now();
}

function profile(tname,cnt,testFn) {
	var t = 100500;
	var count = cnt
	while ( count>0 ) {
		var t0 = timeStamp();
		testFn();
		var t1 = timeStamp();
		var dt = t1 - t0;
		t = Math.min(dt, t);
		count --;
	}
	t /= 1000.0;
	var msg = '"'+tname+'", '+t+', '+cnt;
   print(msg)
}

const testf = function () {
  let frames = Frames;
  while (--frames) {
    testUi(benchmark);
  }
};

function performance_tests() {
	{
		profile("profile darg ui", testsNum, testf);
	}
	timeStamp();
}

performance_tests();
