#include "configuration.hpp"
#include "memory.hpp"

#include "builtin/block_environment.hpp"
#include "builtin/jit.hpp"
#include "builtin/list.hpp"

#ifdef ENABLE_LLVM
#include "llvm/state.hpp"
#endif

namespace rubinius {
  void JIT::bootstrap(STATE) {
    Module* jit = state->memory()->new_module<JIT>(state, G(rubinius), "JIT");
    GO(jit).set(jit);

    Class* cls = state->memory()->new_class<Class>(state, G(jit), "CompileRequest");
    G(jit)->compile_class(state, cls);

    G(jit)->compile_list(state, List::create(state));
  }

  void JIT::initialize(STATE, JIT* obj, Module* under, const char* name) {
    Module::initialize(state, obj, under, name);
  }

  Object* JIT::compile(STATE, Object* object, CompiledCode* code,
                       Object* block_environment, CallFrame* call_frame)
  {
#ifndef ENABLE_LLVM
    return cFalse;
#else
    if(!CBOOL(enabled())) return cFalse;

    BlockEnvironment* block_env = try_as<BlockEnvironment>(block_environment);
    if(!block_env) block_env = nil<BlockEnvironment>();

    LLVMState* ls = state->shared().llvm_state;
    ls->compile(state, code, call_frame, object->direct_class(state),
        block_env, !block_env->nil_p());

    return cTrue;
#endif
  }

  Object* JIT::compile_threshold(STATE) {
    return Integer::from(state, state->shared().config.jit_threshold_compile);
  }

  Object* JIT::sync_set(STATE, Object* flag) {
    state->shared().config.jit_sync.set(CBOOL(flag));
    return sync_get(state);
  }

  Object* JIT::sync_get(STATE) {
    return RBOOL(state->shared().config.jit_sync);
  }

  Object* JIT::enable(STATE) {
    if(!CBOOL(enabled())) return cFalse;

#ifdef ENABLE_LLVM
    state->shared().llvm_state->enable(state);
    enabled(state, cTrue);
#endif

    return cTrue;
  }

  Object* JIT::compile_soon(STATE, CompiledCode* code, CallFrame* call_frame,
      Class* receiver_class, BlockEnvironment* block_env, bool is_block)
  {
    if(!CBOOL(enabled())) return cFalse;

#ifdef ENABLE_LLVM
    LLVMState* ls = state->shared().llvm_state;
    ls->compile_soon(state, code, call_frame, receiver_class, block_env, is_block);
#endif

    return cTrue;
  }

  Object* JIT::compile_callframe(STATE, CompiledCode* code, CallFrame* call_frame,
      int primitive)
  {
    if(!CBOOL(enabled())) return cFalse;

#ifdef ENABLE_LLVM
    LLVMState* ls = state->shared().llvm_state;
    ls->compile_callframe(state, code, call_frame, primitive);
#endif

    return cTrue;
  }

  Object* JIT::start_method_update(STATE) {
    if(!CBOOL(enabled())) return cFalse;

#ifdef ENABLE_LLVM
    LLVMState* ls = state->shared().llvm_state;
    ls->start_method_update();
#endif

    return cTrue;
  }

  Object* JIT::end_method_update(STATE) {
    if(!CBOOL(enabled())) return cFalse;

#ifdef ENABLE_LLVM
    LLVMState* ls = state->shared().llvm_state;
    ls->end_method_update();
#endif

    return cTrue;
  }

  JITCompileRequest* JITCompileRequest::create(STATE, CompiledCode* code,
      Class* receiver_class, int hits, BlockEnvironment* block_env, bool is_block)
  {
    JITCompileRequest* request =
      state->memory()->new_object<JITCompileRequest>(state, G(jit)->compile_class());

    request->method(state, code);
    request->receiver_class(state, receiver_class);
    request->block_env(state, block_env);
    request->hits_ = hits;
    request->is_block_ = is_block;
    request->waiter_ = NULL;

    return request;
  }
}
