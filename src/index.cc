#include <nan.h>

using namespace v8;

template <typename T> inline void USE(T &&) {}
template <typename T, size_t N> constexpr size_t arraysize(const T (&)[N]) {
  return N;
}

// If the user has invoked setUnhandledRejection, then this stores their
// handler.
static Persistent<Function> onUnhandledRejection;

// Exposes V8's SetContinuationPreservedEmbedderData, which is preserved across
// promise chains.
void SetContext(const FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  Local<Value> value = info[0].As<Object>();
  isolate->GetCurrentContext()->SetContinuationPreservedEmbedderData(value);
}

// Exposes V8's GetContinuationPreservedEmbedderData, which reads the value
// preserved by a promise chain.
void GetContext(const FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  info.GetReturnValue().Set(
      isolate->GetCurrentContext()->GetContinuationPreservedEmbedderData());
}

// It's not possible to patch node's unhandledRejection event to restore
// the proper context. So this implements basic handling so we can
// preserve the promise context. This would also be fixed by preserving the
// embedder data across microtasks.
//
// Mostly a copy paste of Node's handler:
// https://github.com/nodejs/node/blob/babe6d7c84b9cefe0b9bf9f1929d19c80cc30c7e/src/node_task_queue.cc#L68-L160
void OnUnhandledRejection(PromiseRejectMessage message) {
  if (onUnhandledRejection.IsEmpty()) {
    return;
  }

  Local<Promise> promise = message.GetPromise();
  Local<Context> context = promise->GetCreationContext().ToLocalChecked();
  Isolate *isolate = promise->GetIsolate();
  PromiseRejectEvent event = message.GetEvent();
  Local<Value> type;

  Local<Value> value;
  switch (event) {
  case kPromiseRejectWithNoHandler:
    value = message.GetValue();
    type = String::NewFromUtf8(isolate, "unhandledRejection",
                               NewStringType::kInternalized)
               .ToLocalChecked();
    break;
  case kPromiseHandlerAddedAfterReject:
    value = Undefined(isolate);
    type = String::NewFromUtf8(isolate, "rejectionHandled",
                               NewStringType::kInternalized)
               .ToLocalChecked();
    break;
  default:
    return;
  }

  if (value.IsEmpty()) {
    value = Undefined(isolate);
  }

  Local<Value> args[] = {type, promise, value};
  Local<Function> handler = onUnhandledRejection.Get(isolate);
  USE(handler->Call(context, Undefined(isolate), arraysize(args), args));
}

void SetUnhandledRejection(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();
  isolate->SetPromiseRejectCallback(OnUnhandledRejection);

  if (args[0]->IsFunction()) {
    onUnhandledRejection.Reset(isolate, args[0].As<Function>());
  } else {
    onUnhandledRejection.Reset();
  }
}

void InitAll(Local<Object> exports) {
  Nan::HandleScope scope;

  NODE_SET_METHOD(exports, "getContext", GetContext);
  NODE_SET_METHOD(exports, "setContext", SetContext);
  NODE_SET_METHOD(exports, "setUnhandledRejection", SetUnhandledRejection);
}

NODE_MODULE(AsyncContext, InitAll)
