#include <iostream>
#include <vector>

#include <nan.h>

#include <GL/gl.h>
#include <GL/glu.h>

#define GLUTESS_SYMBOL(a, b) ( \
  Nan::ForceSet(exports, \
  Nan::New((a)).ToLocalChecked(), \
  Nan::New<v8::Uint32>((b)), attribs))

class GluTess : public Nan::ObjectWrap {

  GLUtesselator *tess_;
  GLenum prim_type_;

  unsigned int vertexSize_;
  typedef std::vector<GLdouble*> VertexData;
  VertexData  vertexData_;

  GLdouble *allocateVertexData(v8::Local<v8::Array> array);
  void releaseVertexData();

  enum {
      CALLBACK_BEGIN = 0,
      CALLBACK_VERTEX,
      CALLBACK_END,
      CALLBACK_ERROR,
      CALLBACK_EDGE_FLAG,
      CALLBACK_COMBINE,
      MAX_CALLBACK,
  };

  struct Callback {
      Nan::Persistent<v8::Function> callback_;
      void (*thunk_)();
  } callbacks_[MAX_CALLBACK];

  v8::Local<v8::Value> callback(int which, int argc, v8::Local<v8::Value> argv[]);

  static void thunkBegin(GLenum type, void *data);
  static void thunkVertex(void *vertexData, void *data);
  static void thunkEnd(void *data);
  static void thunkError(GLenum errno, void *data);
  static void thunkEdgeFlag(GLboolean flag, void *data);
  static void thunkCombine(GLdouble coords[3], void *vertex_data[4],
                              GLfloat weight[4], void **outData,
                              void *data);

public:
  explicit GluTess(unsigned int vertexSize);
  ~GluTess();

  static Nan::Persistent<v8::Function> constructor;

  static void Initialize(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE exports);
  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);

  static void BeginPolygon(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void EndPolygon(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void BeginContour(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void EndContour(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Callback(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Normal(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Property(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Vertex(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

Nan::Persistent<v8::Function> GluTess::constructor;

GluTess::GluTess (unsigned int vertexSize) :
  vertexSize_(vertexSize) {

  tess_ = gluNewTess();

  callbacks_[CALLBACK_BEGIN].thunk_ = (void (*)())thunkBegin;
  callbacks_[CALLBACK_VERTEX].thunk_ = (void (*)())thunkVertex;
  callbacks_[CALLBACK_END].thunk_ = (void (*)())thunkEnd;
  callbacks_[CALLBACK_ERROR].thunk_ = (void (*)())thunkError;
  callbacks_[CALLBACK_EDGE_FLAG].thunk_ = (void (*)())thunkEdgeFlag;
  callbacks_[CALLBACK_COMBINE].thunk_ = (void (*)())thunkCombine;
}

GluTess::~GluTess () {
  for(int i=0; i < MAX_CALLBACK; ++i) {
    if(!callbacks_[i].callback_.IsEmpty()) {
      callbacks_[i].callback_.Reset();
    }
  }

  releaseVertexData();
  gluDeleteTess(tess_);
  tess_ = 0;
}

GLdouble *GluTess::allocateVertexData(v8::Local<v8::Array> array) {
  GLdouble *v = new GLdouble [vertexSize_];
  for(uint32_t i=0; i < vertexSize_; ++i)
    v[i]= array->Get(i)->NumberValue();
  vertexData_.push_back(v);
  return v;
}

void GluTess::releaseVertexData() {
  for(VertexData::const_iterator i = vertexData_.begin(); i != vertexData_.end(); ++i)
    delete [] *i;
  vertexData_.clear();
}

// Thunks to connect gluTess callbacks to v8
//
v8::Local<v8::Value> GluTess::callback(int which, int argc, v8::Local<v8::Value> argv[]) {
  Nan::TryCatch try_catch;
  v8::Local<v8::Function> cb(Nan::New(callbacks_[which].callback_));
  v8::Local<v8::Value> ret = Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
  if (try_catch.HasCaught()) {
    Nan::FatalException(try_catch);
    return Nan::New(0);
  }
  return ret;
}

void GluTess::thunkBegin(GLenum type, void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  v8::Local<v8::Value> argv[1];
  argv[0] = Nan::New<v8::Number>(type);
  self->callback(CALLBACK_BEGIN, 1, argv);
}

void GluTess::thunkVertex(void *vertexData, void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  v8::Local<v8::Array> a = Nan::New<v8::Array>(self->vertexSize_);
  for(unsigned int i=0; i < self->vertexSize_; ++i)
      a->Set(i, Nan::New(((GLdouble *)vertexData)[i]));

  v8::Local<v8::Value> arg = a;
  self->callback(CALLBACK_VERTEX, 1, &arg);
}

void GluTess::thunkEnd(void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  self->callback(CALLBACK_END, 0, 0);
}

void GluTess::thunkError(GLenum errno, void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  v8::Local<v8::Value> arg = Nan::New(errno);
  self->callback(CALLBACK_ERROR, 1, &arg);
}

void GluTess::thunkEdgeFlag(GLboolean flag, void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  v8::Local<v8::Value> arg = Nan::New<v8::Boolean>(flag);
  self->callback(CALLBACK_EDGE_FLAG, 1, &arg);
}

void GluTess::thunkCombine(GLdouble coords[3], void *vertex_data[4],
                           GLfloat weight[4], void **outData, void *data) {
  GluTess* self = (GluTess*)data;
  Nan::HandleScope scope;

  // Position
  v8::Local<v8::Array> coords_cb = Nan::New<v8::Array>(3);
  for(unsigned int i=0; i < 3; ++i)
    coords_cb->Set(i, Nan::New(coords[i]));

  // Surrounding vertices
  v8::Local<v8::Array> vertices_cb = Nan::New<v8::Array>(3);
  for(unsigned int i=0; i < 4; ++i) {
    v8::Local<v8::Array> a = Nan::New<v8::Array>(self->vertexSize_);
    if(vertex_data[i])
      for(unsigned int j=0; j < self->vertexSize_; ++j)
        a->Set(j, Nan::New(((GLdouble *)vertex_data[i])[j]));
    else
      for(unsigned int j=0; j < self->vertexSize_; ++j)
        a->Set(j, Nan::New(0));

    vertices_cb->Set(i, a);
  }

  // Weights
  v8::Local<v8::Array> weight_cb = Nan::New<v8::Array>(4);
  for(int i=0; i < 4; ++i)
    weight_cb->Set(i, Nan::New(weight[i]));

  v8::Local<v8::Value> argv[4];
  argv[0] = coords_cb;
  argv[1] = vertices_cb;
  argv[2] = weight_cb;
  argv[3] = Nan::New(self->vertexSize_);

  v8::Local<v8::Value> out = self->callback(CALLBACK_COMBINE, 4, argv);

  if(out.IsEmpty() || !out->IsArray()) {
    Nan::ThrowError("combine callback should return array");
    *outData = vertex_data[0];
    return;
  }

  v8::Local<v8::Array> outArray = v8::Local<v8::Array>::Cast(out);

  if(outArray->Length() < self->vertexSize_) {
    Nan::ThrowError("combine callback did not return enough values");
    *outData = vertex_data[0];
    return;
  }

  *outData = self->allocateVertexData(outArray);
}

void GluTess::BeginPolygon(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() > 0)
    Nan::ThrowError("no arguments");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  self->vertexData_.clear();
  gluTessBeginPolygon(self->tess_, (void *)self);

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::EndPolygon(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() > 0)
    Nan::ThrowError("no arguments");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  gluTessEndPolygon(self->tess_);
  self->releaseVertexData();

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::BeginContour(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() > 0)
    Nan::ThrowError("no arguments");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  gluTessBeginContour(self->tess_);

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::EndContour(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() > 0)
    Nan::ThrowError("no arguments");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  gluTessEndContour(self->tess_);

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::Callback(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() != 2 || !info[0]->IsUint32() ||
      !(info[1]->IsFunction() || info[1]->IsUndefined()))
      Nan::ThrowError("Two arguments, which & callback");

  if(info[0]->Uint32Value() < GLU_TESS_BEGIN_DATA || info[0]->Uint32Value() > GLU_TESS_COMBINE_DATA)
    Nan::ThrowError("Not a callback");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  int which = info[0]->Uint32Value() - GLU_TESS_BEGIN_DATA;

  if(info[1]->IsUndefined()) {
    // Clear the callback
    gluTessCallback(self->tess_, info[0]->Uint32Value(), 0);
    self->callbacks_[which].callback_.Reset();
  } else {
    // Connect the callback via the appropriate thunk
    self->callbacks_[which].callback_.Reset(info[1].As<v8::Function>());
    gluTessCallback(self->tess_, info[0]->Uint32Value(), self->callbacks_[which].thunk_);
  }

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::Normal(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() != 3 || !info[0]->IsNumber() || !info[1]->IsNumber()  || !info[2]->IsNumber())
    Nan::ThrowError("3 numbers");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  gluTessNormal(self->tess_, info[0]->NumberValue(), info[1]->NumberValue(), info[2]->NumberValue());

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::Property(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() != 2 || !info[0]->IsUint32() || !info[1]->IsNumber())
    Nan::ThrowError("two arguments, int, int");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  gluTessProperty(self->tess_, info[0]->Uint32Value(), info[1]->NumberValue());

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::Vertex(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;

  if (info.Length() != 1 || !info[0]->IsArray())
    Nan::ThrowError("1 array argument");

  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(info[0]);

  if(array->Length() < 3)
    Nan::ThrowError("1 array argument");

  GluTess *self = Nan::ObjectWrap::Unwrap<GluTess>(info.This());

  if (array->Length() != self->vertexSize_) {
    //DEBUG("!! %d %d", array->Length(), self->vertexSize_);
    Nan::ThrowError("wrong number of vertex values");
  }

  GLdouble *v = self->allocateVertexData(array);
  gluTessVertex(self->tess_, v, v);

  info.GetReturnValue().Set(Nan::Undefined());
}

void GluTess::Initialize (Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE exports) {
  Nan::HandleScope scope;

  // prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("GluTess").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "beginPolygon", BeginPolygon);
  Nan::SetPrototypeMethod(tpl, "endPolygon", EndPolygon);
  Nan::SetPrototypeMethod(tpl, "beginContour", BeginContour);
  Nan::SetPrototypeMethod(tpl, "endContour", EndContour);
  Nan::SetPrototypeMethod(tpl, "callback", Callback);
  Nan::SetPrototypeMethod(tpl, "normal", Normal);
  Nan::SetPrototypeMethod(tpl, "property", Property);
  Nan::SetPrototypeMethod(tpl, "vertex", Vertex);

  constructor.Reset(tpl->GetFunction());

  const v8::PropertyAttribute attribs = (v8::PropertyAttribute) (v8::ReadOnly | v8::DontDelete);

  GLUTESS_SYMBOL("WINDING_RULE", GLU_TESS_WINDING_RULE);
  GLUTESS_SYMBOL("BOUNDARY_ONLY", GLU_TESS_BOUNDARY_ONLY);
  GLUTESS_SYMBOL("TOLERANCE", GLU_TESS_TOLERANCE);

  GLUTESS_SYMBOL("WINDING_ODD", GLU_TESS_WINDING_ODD);
  GLUTESS_SYMBOL("WINDING_NONZERO", GLU_TESS_WINDING_NONZERO);
  GLUTESS_SYMBOL("WINDING_POSITIVE", GLU_TESS_WINDING_POSITIVE);
  GLUTESS_SYMBOL("WINDING_NEGATIVE", GLU_TESS_WINDING_NEGATIVE);
  GLUTESS_SYMBOL("WINDING_ABS_GEQ_TWO", GLU_TESS_WINDING_ABS_GEQ_TWO);

  GLUTESS_SYMBOL("BEGIN", GLU_TESS_BEGIN_DATA);
  GLUTESS_SYMBOL("VERTEX", GLU_TESS_VERTEX_DATA);
  GLUTESS_SYMBOL("END", GLU_TESS_END_DATA);
  GLUTESS_SYMBOL("ERROR", GLU_TESS_ERROR_DATA);
  GLUTESS_SYMBOL("EDGE_FLAG", GLU_TESS_EDGE_FLAG_DATA);
  GLUTESS_SYMBOL("COMBINE", GLU_TESS_COMBINE_DATA);

  GLUTESS_SYMBOL("LINE_LOOP", GL_LINE_LOOP);
  GLUTESS_SYMBOL("TRIANGLES", GL_TRIANGLES);
  GLUTESS_SYMBOL("TRIANGLE_STRIP", GL_TRIANGLE_STRIP);
  GLUTESS_SYMBOL("TRIANGLE_FAN", GL_TRIANGLE_FAN);

  Nan::Set(exports, Nan::New("GluTess").ToLocalChecked(), tpl->GetFunction());
}

void GluTess::New (const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Nan::HandleScope scope;
  if (info.IsConstructCall()) {
    GluTess* self = new GluTess(info[0]->Uint32Value());
    self->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
}

NAN_MODULE_INIT(init) {
  GluTess::Initialize(target);
}

NODE_MODULE(glutess, init)
