const { getContext, setContext } = require("bindings")("async-context");

module.exports = class AsyncContext {
  static wrap(fn) {
    const snapshot = getContext();
    return function (...args) {
      return run(snapshot, fn, this, args);
    };
  }

  run(val, fn, ...args) {
    const next = new Map(getContext());
    next.set(this, val);
    return run(next, fn, null, args);
  }

  get() {
    return getContext()?.get(this);
  }
};

function run(next, fn, binding, args) {
  const current = getContext();
  try {
    setContext(next);
    return fn.apply(binding, args);
  } finally {
    setContext(current);
  }
}
