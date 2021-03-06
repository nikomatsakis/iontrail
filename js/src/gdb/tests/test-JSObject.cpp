#include "gdb-tests.h"

FRAGMENT(JSObject, simple) {
  js::Rooted<JSObject *> glob(cx, JS_GetGlobalObject(cx));
  js::Rooted<JSObject *> plain(cx, JS_NewObject(cx, 0, 0, 0));
  js::Rooted<JSObject *> func(cx, (JSObject *) JS_NewFunction(cx, (JSNative) 1, 0, 0,
                                                              JS_GetGlobalObject(cx), "dys"));
  js::Rooted<JSObject *> anon(cx, (JSObject *) JS_NewFunction(cx, (JSNative) 1, 0, 0,
                                                              JS_GetGlobalObject(cx), 0));
  js::Rooted<JSFunction *> funcPtr(cx, JS_NewFunction(cx, (JSNative) 1, 0, 0,
                                                      JS_GetGlobalObject(cx), "formFollows"));

  JSObject &plainRef = *plain;
  JSFunction &funcRef = *funcPtr;
  js::RawObject plainRaw = plain;
  js::RawObject funcRaw = func;

  breakpoint();

  (void) glob;
  (void) plain;
  (void) func;
  (void) anon;
  (void) funcPtr;
  (void) &plainRef;
  (void) &funcRef;
  (void) plainRaw;
  (void) funcRaw;
}

FRAGMENT(JSObject, null) {
  js::Rooted<JSObject *> null(cx, NULL);
  js::RawObject nullRaw = null;

  breakpoint();

  (void) null;
  (void) nullRaw;
}
