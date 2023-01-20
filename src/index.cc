#include <nan.h>

using namespace v8;

template <typename T> inline void USE(T &&) {}
template <typename T, size_t N> constexpr size_t arraysize(const T (&)[N]) {
  return N;
}

static Persistent<Function> onUnhandledRejection;

void SetContext(const FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  Local<Value> value = info[0].As<Object>();
  isolate->GetCurrentContext()->SetContinuationPreservedEmbedderData(value);
}

void GetContext(const FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  info.GetReturnValue().Set(
      isolate->GetCurrentContext()->GetContinuationPreservedEmbedderData());
}

void OnUnhandledRejection(PromiseRejectMessage message) {
  if (onUnhandledRejection.IsEmpty()) {
    return;
  }

  Local<Promise> promise = message.GetPromise();
  Local<Context> context = promise->GetCreationContext().ToLocalChecked();
  Isolate *isolate = promise->GetIsolate();
  PromiseRejectEvent event = message.GetEvent();
  Local<Value> type;

  Local<Function> handler = onUnhandledRejection.Get(isolate);
  if (handler->IsUndefined()) {
    return;
  }

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
  USE(handler->Call(context, Undefined(isolate), arraysize(args), args));
}

void SetUnhandledRejection(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();
  isolate->SetPromiseRejectCallback(OnUnhandledRejection);
  onUnhandledRejection.Reset(isolate, args[0].As<Function>());
}

void InitAll(Local<Object> exports) {
  Nan::HandleScope scope;

  NODE_SET_METHOD(exports, "getContext", GetContext);
  NODE_SET_METHOD(exports, "setContext", SetContext);
  NODE_SET_METHOD(exports, "setUnhandledRejection", SetUnhandledRejection);
  NODE_DEFINE_CONSTANT(exports, kPromiseRejectWithNoHandler);
  NODE_DEFINE_CONSTANT(exports, kPromiseHandlerAddedAfterReject);
}

NODE_MODULE(AsyncContext, InitAll)
