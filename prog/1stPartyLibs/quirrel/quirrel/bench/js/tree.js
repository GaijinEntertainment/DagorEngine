// treap from https://github.com/frol/completely-unscientific-benchmarks
import { benchmark } from "./profile.js";

const OPS = 200000;

let seed = 42;
function random() {
  seed = seed * 16807 % 2147483647;
  return seed;
}

function merge(lower, greater) {
  if (lower === null)
    return greater;
  if (greater === null)
    return lower;
  if (lower.y < greater.y) {
    lower.right = merge(lower.right, greater);
    return lower;
  }
  greater.left = merge(lower, greater.left);
  return greater;
}

// indexed reads: QuickJS destructures through iterators
function splitBinary(orig, value) {
  if (orig === null)
    return [null, null];
  if (orig.x < value) {
    const pair = splitBinary(orig.right, value);
    orig.right = pair[0];
    return [orig, pair[1]];
  }
  const pair = splitBinary(orig.left, value);
  orig.left = pair[1];
  return [pair[0], orig];
}

function merge3(lower, equal, greater) {
  return merge(merge(lower, equal), greater);
}

function split(orig, value) {
  const lowerRest = splitBinary(orig, value);
  const equalGreater = splitBinary(lowerRest[1], value + 1);
  return [lowerRest[0], equalGreater[0], equalGreater[1]];
}

function hasValue(tree, x) {
  const parts = split(tree.root, x);
  tree.root = merge3(parts[0], parts[1], parts[2]);
  return parts[1] !== null;
}

function insert(tree, x) {
  const parts = split(tree.root, x);
  tree.root = merge3(parts[0], parts[1] ?? { x, y: random(), left: null, right: null }, parts[2]);
}

function erase(tree, x) {
  const parts = split(tree.root, x);
  tree.root = merge(parts[0], parts[2]);
}

function treeOps() {
  const tree = { root: null };
  let cur = 5;
  let found = 0;
  for (let i = 1; i <= OPS; i++) {
    cur = (cur * 57 + 43) % 10007;
    const op = i % 3;
    if (op === 0)
      insert(tree, cur);
    else if (op === 1)
      erase(tree, cur);
    else if (hasValue(tree, cur))
      found++;
  }
  return found;
}

benchmark("tree", 10, treeOps);
