const { AsyncContext, setUnhandledRejection } = require("..");
const { strict: assert } = require("assert");

it("get", () => {
  const ctx = new AsyncContext();
  assert.equal(ctx.get(), undefined);
});

it("promises", () => {
  const ctx = new AsyncContext();
  ctx
    .run(1, () => {
      return Promise.resolve().then(() => {
        assert.equal(ctx.get(), 1);
      });
    })
    .then(() => {
      assert.equal(ctx.get(), undefined);
    });
});

it("async await", async () => {
  const ctx = new AsyncContext();
  await ctx.run(1, async () => {
    await Promise.resolve(1);
    assert.equal(ctx.get(), 1);
  });
  assert.equal(ctx.get(), undefined);
});

it("triggers unhandledRejection", () => {
  const ctx = new AsyncContext();

  let value;
  setUnhandledRejection(function(type) {
    switch (type) {
      case "unhandledRejection":
        value = ctx.get();
    }
  });

  ctx.run(1, () => {
    Promise.reject();
  });

  assert.equal(value, 1);
});
