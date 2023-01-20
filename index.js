const { getContext, setContext, setUnhandledRejection } = require("bindings")("async-context");

exports.AsyncContext = class AsyncContext {
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

// It's not possible to patch node's unhandledRejection event to restore the
// proper context. So if you care to get the events, you need to use this to
// register your unhandledRejection events.
//
// **NOTE** This prevents `process.on('unhandledRejection', …)` from working.
exports.setUnhandledRejection = function(handler) {
  if (typeof handler !== 'function') handler = undefined;
  setUnhandledRejection(handler);
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
