const { getContext, setContext, setUnhandledRejection } =
  require("bindings")("async-context");

// Implements AsyncContext according to the proposal.
//
// Makes use of V8's "Preserved Embedder Data" APIs to accomplish propagating
// the context across async pause/resumes automatically. This is the equivalent
// of the [[HostDefined]] defined on [JobCallback][] Records. However, it's not
// 100%, because V8 doesn't preserve the context across microtask Jobs. If V8
// did, this would be perfect (and simple!).
//
// [JobCallback]: https://tc39.es/ecma262/multipage/executable-code-and-execution-contexts.html#sec-jobcallback-records
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

function run(next, fn, binding, args) {
  const current = getContext();
  try {
    setContext(next);
    return fn.apply(binding, args);
  } finally {
    setContext(current);
  }
}

// The unhandledRejection event _could_ capture the context at the time of the
// rejection, but Node would need to record it in its queue of
// maybe-handled-in-the-future promises. Because Node isn't aware of our API
// yet, it's obviously not doing that.
//
// Unfortunately, it's not possible to patch node's unhandledRejection event to
// restore the proper context. So if you care to get the events, you need to
// use this to register your unhandledRejection events.
//
// **NOTE** This prevents `process.on('unhandledRejection', …)` from working.
let onUnhandledRejection;
const unhandled = new Map();
exports.setUnhandledRejection = (handler) => {
  if (typeof handler === "function") {
    onUnhandledRejection = handler;
    setUnhandledRejection(OnUnhandledRejection);
  } else {
    onUnhandledRejection = undefined;
    setUnhandledRejection(undefined);
    drain.clear();
    unhandled.clear();
  }
};

function OnUnhandledRejection(type, promise, value) {
  switch (type) {
    case "unhandledRejection":
      unhandled.set(promise, { promise, value, ctx: getContext() });
      drain();
      break;

    case "rejectionHandled":
      if (unhandled.has()) {
        unhandled.delete(promise);
      } else {
        onUnhandledRejection(type, promise);
      }
      break;
  }
}

const drain = debounce(() => {
  for (const [_, item] of unhandled) {
    run(item.ctx, onUnhandledRejection, null, [
      "unhandledRejection",
      item.promise,
      item.value,
    ]);
  }
  unhandled.clear();
});

function debounce(fn, ms = 10) {
  let timer;
  const debounced = function () {
    clearTimeout(timer);
    timer = setTimeout(() => {
      fn.apply(this, arguments);
    }, ms);
  };
  debounced.clear = () => clearTimeout(timer);
  return debounced;
}
