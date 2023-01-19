#include <nan.h>

using namespace v8;

void SetContext(const Nan::FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  Local<Value> value = info[0].As<Object>();
  isolate->GetCurrentContext()->SetContinuationPreservedEmbedderData(value);
}

void GetContext(const Nan::FunctionCallbackInfo<Value> &info) {
  Isolate *isolate = info.GetIsolate();
  info.GetReturnValue().Set(
      isolate->GetCurrentContext()->GetContinuationPreservedEmbedderData());
}

void InitAll(Local<Object> exports) {
  Local<Context> context = exports->GetCreationContext().ToLocalChecked();
  Nan::HandleScope scope;

  exports
      ->Set(context, Nan::New("getContext").ToLocalChecked(),
            Nan::New<FunctionTemplate>(GetContext)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
  exports
      ->Set(context, Nan::New("setContext").ToLocalChecked(),
            Nan::New<FunctionTemplate>(SetContext)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

NODE_MODULE(AsyncContext, InitAll)
