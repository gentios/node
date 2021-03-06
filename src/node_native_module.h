#ifndef SRC_NODE_NATIVE_MODULE_H_
#define SRC_NODE_NATIVE_MODULE_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include <map>
#include <set>
#include <string>
#include "env.h"
#include "node_union_bytes.h"
#include "v8.h"

namespace node {
namespace native_module {

using NativeModuleRecordMap = std::map<std::string, UnionBytes>;
using NativeModuleHashMap = std::map<std::string, std::string>;

// The native (C++) side of the NativeModule in JS land, which
// handles compilation and caching of builtin modules (NativeModule)
// and bootstrappers, whose source are bundled into the binary
// as static data.
// This class should not depend on a particular isolate, context, or
// environment. Rather it should take them as arguments when necessary.
// The instances of this class are per-process.
class NativeModuleLoader {
 public:
  // kCodeCache indicates that the compilation result should be returned
  // as a Uint8Array, whereas kFunction indicates that the result should
  // be returned as a Function.
  // TODO(joyeecheung): it's possible to always produce code cache
  // on the main thread and consume them in worker threads, or just
  // share the cache among all the threads, although
  // we need to decide whether to do that even when workers are not used.
  enum class CompilationResultType { kCodeCache, kFunction };

  NativeModuleLoader();
  static void Initialize(v8::Local<v8::Object> target,
                         v8::Local<v8::Value> unused,
                         v8::Local<v8::Context> context,
                         void* priv);
  v8::Local<v8::Object> GetSourceObject(v8::Local<v8::Context> context) const;
  // Returns config.gypi as a JSON string
  v8::Local<v8::String> GetConfigString(v8::Isolate* isolate) const;

  v8::Local<v8::String> GetSource(v8::Isolate* isolate, const char* id) const;

  // Run a script with JS source bundled inside the binary as if it's wrapped
  // in a function called with a null receiver and arguments specified in C++.
  // The returned value is empty if an exception is encountered.
  // JS code run with this method can assume that their top-level
  // declarations won't affect the global scope.
  v8::MaybeLocal<v8::Value> CompileAndCall(
      v8::Local<v8::Context> context,
      const char* id,
      std::vector<v8::Local<v8::String>>* parameters,
      std::vector<v8::Local<v8::Value>>* arguments,
      Environment* optional_env);

 private:
  static void GetCacheUsage(const v8::FunctionCallbackInfo<v8::Value>& args);
  // Passing map of builtin module source code into JS land as
  // internalBinding('native_module').source
  static void SourceObjectGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  // Passing config.gypi into JS land as internalBinding('native_module').config
  static void ConfigStringGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  // Compile code cache for a specific native module
  static void CompileCodeCache(const v8::FunctionCallbackInfo<v8::Value>& args);
  // Compile a specific native module as a function
  static void CompileFunction(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Generated by tools/js2c.py as node_javascript.cc
  void LoadJavaScriptSource();  // Loads data into source_
  void LoadJavaScriptHash();    // Loads data into source_hash_
  UnionBytes GetConfig();       // Return data for config.gypi

  // Generated by tools/generate_code_cache.js as node_code_cache.cc when
  // the build is configured with --code-cache-path=.... They are noops
  // in node_code_cache_stub.cc
  void LoadCodeCache();      // Loads data into code_cache_
  void LoadCodeCacheHash();  // Loads data into code_cache_hash_

  v8::ScriptCompiler::CachedData* GetCachedData(const char* id) const;

  // Compile a script as a NativeModule that can be loaded via
  // NativeModule.p.require in JS land.
  static v8::MaybeLocal<v8::Value> CompileAsModule(
      Environment* env, const char* id, CompilationResultType result_type);

  // For bootstrappers optional_env may be a nullptr.
  // If an exception is encountered (e.g. source code contains
  // syntax error), the returned value is empty.
  v8::MaybeLocal<v8::Value> LookupAndCompile(
      v8::Local<v8::Context> context,
      const char* id,
      std::vector<v8::Local<v8::String>>* parameters,
      CompilationResultType result_type,
      Environment* optional_env);

  bool has_code_cache_ = false;
  NativeModuleRecordMap source_;
  NativeModuleRecordMap code_cache_;
  UnionBytes config_;

  NativeModuleHashMap source_hash_;
  NativeModuleHashMap code_cache_hash_;
};

}  // namespace native_module
}  // namespace node

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // SRC_NODE_NATIVE_MODULE_H_
