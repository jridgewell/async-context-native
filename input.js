const AsyncContext = require('.');
const ctx = new AsyncContext()

const queue = [];

const thenable = {
  then(onRes, _onRej) {
    queue.push("thenable: " + ctx.get());
    onRes();
  },
};

const out = ctx.run(1, () => {
  queue.push("new Promise");
  const p = new Promise(res => res(thenable));

  queue.push("p.then");
  const p2 = p.then(() => thenable);

  queue.push("p2.then");
  return p2.then(() => {
    queue.push("promise: " + ctx.get());
  });
});

queue.push("out.then");
out.then(() => {
  queue.push("done");
  //hook.disable();
  console.log(queue);
});
