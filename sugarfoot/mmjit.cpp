#include "mmjit.hpp"
#include "utils.hpp"
#include "mmobj.hpp"
#include "process.hpp"
#include <jit/jit.h>
#include <stdlib.h>
#include <setjmp.h>

static
void handle_send(number args, Process* proc) {
  proc->handle_send(args);
}

static
void handle_super_ctor_send(number args, Process* proc) {
  proc->handle_super_ctor_send(args);
}

static
void handle_super_send(number args, Process* proc) {
  proc->handle_super_ctor_send(args);
}

static
void handle_call(number args, Process* proc) {
  proc->handle_call(args);
}


static
oop get_literal_by_index(MMObj* mmobj, Process* proc, oop cp, int arg, bool should_assert) {
  return mmobj->mm_function_get_literal_by_index(proc, cp, arg, should_assert);
}

static
void emit_jz(jit_label_t* mypos, jit_function_t function, jit_label_t* target) {
  jit_insn_label(function, mypos);

  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t top = jit_insn_load_relative(function, sp, 0, jit_type_void_ptr);
  jit_insn_branch_if_not(function, top, target);
}



/////////////////////////////////////


static
void emit_handler(jit_label_t* mypos, jit_function_t function, Process* proc, int code_args, void (*handler)()) {
  jit_insn_label(function, mypos);

  //to grab buf
  jit_value_t ptr_fp = jit_value_get_param(function, 1);
  jit_value_t fp = jit_insn_load_relative(function, ptr_fp, 0, jit_type_void_ptr);
  jit_value_t jbuf = jit_insn_load_relative(function, fp, 0, jit_type_void_ptr);

  jit_type_t params[2];
  params[0] = jit_type_int; //args
  params[1] = jit_type_void_ptr; //Process
  jit_type_t handle_send_signature =
    jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 2, 1);

  jit_value_t args[2];
  args[0] = jit_value_create_nint_constant(function, jit_type_uint, code_args);
  args[1] = jit_value_create_nint_constant(function, jit_type_void_ptr, (long)(void*)proc);

  jit_insn_call_native(
    function, "a_handler", (void*)handler,
    handle_send_signature, args, 2, JIT_CALL_NOTHROW);

//    if (!setjmp(frame_buf))
  jit_type_t jparams[1];
  jparams[0] = jit_type_void_ptr; //jbuf
  jit_type_t setjmp_signature =
    jit_type_create_signature(jit_abi_cdecl, jit_type_int, jparams, 1, 1);

  jit_value_t jargs[1];
  jargs[0] = jbuf;

  jit_value_t jmp_val = jit_insn_call_native(
    function, "setjmp", (void*)setjmp,
    setjmp_signature, jargs, 1, JIT_CALL_NOTHROW);

  // then ...
  jit_label_t cont = jit_label_undefined;
  jit_insn_branch_if(function, jmp_val, &cont);

  // jump back to vm
  jit_type_t jjparams[2];
  jjparams[0] = jit_type_void_ptr; //jbuf
  jjparams[1] = jit_type_int; //val
  jit_type_t longjump_signature =
    jit_type_create_signature(jit_abi_cdecl, jit_type_void, jjparams, 2, 1);

  jit_value_t jjargs[2];
  jjargs[0] = jit_value_create_nint_constant(function, jit_type_void_ptr, (long)(void*)proc->proc_setjmp_buf);
  jjargs[1] = jit_value_create_nint_constant(function, jit_type_int, 10);//FIXME

  jit_insn_call_native(
    function, "longjmp", (void*)longjmp,
    longjump_signature, jjargs, 2, JIT_CALL_NOTHROW);

  jit_insn_label(function, &cont);

  // jit_type_t done_s =
  //   jit_type_create_signature(jit_abi_cdecl, jit_type_void, NULL, 0, 1);

  // jit_insn_call_native(
  //   function, "_done_send", _done_send,
  //   done_s, NULL, 0, JIT_CALL_NOTHROW);

  //returning
  jjargs[1] = jit_value_create_nint_constant(function, jit_type_int, 20);//FIXME

  jit_insn_call_native(
    function, "longjmp", (void*)longjmp,
    longjump_signature, jjargs, 2, JIT_CALL_NOTHROW);
}


static
void emit_jump(jit_label_t* mypos, jit_function_t function, jit_label_t* target) {
  jit_insn_label(function, mypos);
  jit_insn_branch(function, target);
}

static
void emit_pop_field(jit_label_t* mypos, jit_function_t function, number arg) {
  jit_insn_label(function, mypos);

  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_dp = jit_value_get_param(function, 8);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t dp = jit_insn_load_relative(function, ptr_dp, 0, jit_type_void_ptr);

  jit_value_t top = jit_insn_load_relative(function, sp, 0, jit_type_void_ptr);


  //sp--
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_mm = jit_insn_sub(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_mm);

  jit_value_t varg = jit_value_create_nint_constant(function, jit_type_ulong, (2 + arg) * sizeof(word*));
  jit_value_t dp_plus_arg = jit_insn_add(function, dp, varg);
  jit_insn_store_relative(function, dp_plus_arg, 0, top);
}

static
void emit_pop(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);

  //sp--
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_mm = jit_insn_sub(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_mm);
}

static
void emit_pop_local(jit_label_t* mypos, jit_function_t function, int arg) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_fp = jit_value_get_param(function, 1);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t fp = jit_insn_load_relative(function, ptr_fp, 0, jit_type_void_ptr);

  jit_value_t top = jit_insn_load_relative(function, sp, 0, jit_type_void_ptr);

  //sp--
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_mm = jit_insn_sub(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_mm);

  jit_value_t varg = jit_value_create_nint_constant(function, jit_type_uint, arg * sizeof(word*));
  jit_value_t fp_plus_arg = jit_insn_add(function, fp, varg);
  jit_insn_store_relative(function, fp_plus_arg, 0, top);
}

static
void emit_return_top(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);

  jit_value_t top = jit_insn_load_relative(function, sp, 0, jit_type_void_ptr);

  //sp--
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_mm = jit_insn_sub(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_mm);

  jit_insn_return(function, top);
}

static
void emit_return_this(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_rp = jit_value_get_param(function, 7);
  jit_value_t rp = jit_insn_load_relative(function, ptr_rp, 0, jit_type_void_ptr);
  jit_insn_return(function, rp);
}

static
void emit_push_bin(jit_label_t* mypos, jit_function_t function, number arg) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_value_t varg = jit_value_create_nint_constant(function, jit_type_ulong, arg);
  jit_insn_store_relative(function, sp_pp, 0, varg);
}

static
void emit_push_context(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_cp = jit_value_get_param(function, 2);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t cp = jit_insn_load_relative(function, ptr_cp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_insn_store_relative(function, sp_pp, 0, cp);
}

static
void emit_push_fp(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_fp = jit_value_get_param(function, 1);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t fp = jit_insn_load_relative(function, ptr_fp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_insn_store_relative(function, sp_pp, 0, fp);
}


static
void emit_push_this(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_rp = jit_value_get_param(function, 7);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t rp = jit_insn_load_relative(function, ptr_rp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_insn_store_relative(function, sp_pp, 0, rp);
}


static
void emit_push_field(jit_label_t* mypos, jit_function_t function, number arg) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_dp = jit_value_get_param(function, 8);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t dp = jit_insn_load_relative(function, ptr_dp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_value_t varg = jit_value_create_nint_constant(function, jit_type_ulong, (2 + arg) * sizeof(word*));
  jit_value_t dp_plus_arg = jit_insn_add(function, dp, varg);
  jit_value_t dref_dp_plus_arg = jit_insn_load_relative(function, dp_plus_arg, 0, jit_type_void_ptr);
  jit_insn_store_relative(function, sp_pp, 0, dref_dp_plus_arg);
}

static
void emit_push_module(jit_label_t* mypos, jit_function_t function) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_mp = jit_value_get_param(function, 6);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t mp = jit_insn_load_relative(function, ptr_mp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_insn_store_relative(function, sp_pp, 0, mp);
}

static
void emit_push_local(jit_label_t* mypos, jit_function_t function, int arg) {
  jit_insn_label(function, mypos);
  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_fp = jit_value_get_param(function, 1);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t fp = jit_insn_load_relative(function, ptr_fp, 0, jit_type_void_ptr);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  jit_value_t varg = jit_value_create_nint_constant(function, jit_type_uint, arg * sizeof(word*));
  jit_value_t fp_plus_arg = jit_insn_add(function, fp, varg);
  jit_value_t dref_fp_plus_arg = jit_insn_load_relative(function, fp_plus_arg, 0, jit_type_void_ptr);
  jit_insn_store_relative(function, sp_pp, 0, dref_fp_plus_arg);
}

static
void emit_push_literal(jit_label_t* mypos, jit_function_t function, int arg, Process* proc, MMObj* mmobj) {
  jit_insn_label(function, mypos);

  jit_value_t ptr_sp = jit_value_get_param(function, 0);
  jit_value_t ptr_cp = jit_value_get_param(function, 2);

  jit_value_t sp = jit_insn_load_relative(function, ptr_sp, 0, jit_type_void_ptr);
  jit_value_t cp = jit_insn_load_relative(function, ptr_cp, 0, jit_type_void_ptr);

  jit_type_t params[5];
  params[0] = jit_type_void_ptr; //MMObj
  params[1] = jit_type_void_ptr; //Process
  params[2] = jit_type_void_ptr; //_cp
  params[3] = jit_type_int; //arg
  params[4] = jit_type_ubyte; //true

  jit_type_t get_literal_by_index_signature =
    jit_type_create_signature(jit_abi_cdecl, jit_type_void_ptr, params, 5, 1);

  jit_value_t args[5];
  args[0] = jit_value_create_nint_constant(function, jit_type_void_ptr, (long)(void*)mmobj);
  args[1] = jit_value_create_nint_constant(function, jit_type_void_ptr, (long)(void*)proc);
  args[2] = cp;
  args[3] = jit_value_create_nint_constant(function, jit_type_uint, arg);
  args[4] = jit_value_create_nint_constant(function, jit_type_sbyte, 1);

  jit_value_t literal = jit_insn_call_native(
    function, "get_literal_by_index", (void*)get_literal_by_index,
    get_literal_by_index_signature, args, 5, JIT_CALL_NOTHROW);

  //sp++
  jit_value_t one_word = jit_value_create_nint_constant(
    function, jit_type_ubyte, sizeof(oop));
  jit_value_t sp_pp = jit_insn_add(function, sp, one_word);
  jit_insn_store_relative(function, ptr_sp, 0, sp_pp);

  //*sp = data
  jit_insn_store_relative(function, sp_pp, 0, literal);
}


void* gnerate_code_for(bytecode* ip, number size, Process* proc, MMObj* mmobj) {
  jit_context_t context;
  context = jit_context_create();
  jit_context_build_start(context);

  jit_type_t params[9];
  params[0] = jit_type_void_ptr; //_sp
  params[1] = jit_type_void_ptr; //_fp
  params[2] = jit_type_void_ptr; //_cp
  params[3] = jit_type_void_ptr; //_ip
  params[4] = jit_type_void_ptr; //_ss
  params[5] = jit_type_void_ptr; //_bp
  params[6] = jit_type_void_ptr; //_mp
  params[7] = jit_type_void_ptr; //_rp
  params[8] = jit_type_void_ptr; //_dp

  jit_type_t signature;
  signature = jit_type_create_signature
    (jit_abi_cdecl, jit_type_void_ptr, params, 9, 1);

  jit_function_t function;
  function = jit_function_create(context, signature);
  jit_type_free(signature);

  jit_label_t *labels = (jit_label_t*) calloc(size, sizeof(jit_label_t));
  //jit_label_t labels[size];

  for (number i = 0; i < size / 4; i++) {
    labels[i] = jit_label_undefined;

    bytecode code = *ip;
    int opcode = decode_opcode(code);
    int arg = decode_args(code);

    switch(opcode) {
      case PUSH_LOCAL:
        emit_push_local(&labels[i], function, arg);
        break;
      case PUSH_LITERAL:
        emit_push_literal(&labels[i], function, arg, proc, mmobj);
        break;
      case PUSH_MODULE:
        emit_push_module(&labels[i], function);
        break;
      case PUSH_FIELD:
        emit_push_field(&labels[i], function, arg);
        break;
      case PUSH_THIS:
        emit_push_this(&labels[i], function);
        break;
      case PUSH_FP:
        emit_push_fp(&labels[i], function);
        break;
      case PUSH_CONTEXT:
        emit_push_context(&labels[i], function);
        break;
      case PUSH_BIN:
        emit_push_bin(&labels[i], function, arg);
        break;
      case RETURN_TOP:
        emit_return_top(&labels[i], function);
        break;
      case RETURN_THIS:
        emit_return_this(&labels[i], function);
        break;
      case POP:
        emit_pop(&labels[i], function);
        break;
      case POP_LOCAL:
        emit_pop_local(&labels[i], function, arg);
        break;
      case POP_FIELD:
        emit_pop_field(&labels[i], function, arg);
        break;
      case SEND:
        emit_handler(&labels[i], function, proc, arg, (void (*)())handle_send);
        break;
      case SUPER_CTOR_SEND:
        emit_handler(&labels[i], function, proc, arg,  (void (*)()) handle_super_ctor_send);
        break;
      case CALL:
        emit_handler(&labels[i], function, proc, arg,  (void (*)()) handle_call);
        break;
      case JMP:
        emit_jump(&labels[i], function, &labels[i + arg - 1]);
        break;
      case JMPB:
        emit_jump(&labels[i], function, &labels[i - (arg + 1)]);
        break;
      case JZ:
        emit_jz(&labels[i], function, &labels[i + arg - 1]);
        break;
      case SUPER_SEND:
        emit_handler(&labels[i], function, proc, arg,  (void (*)()) handle_super_send);
      default:
        proc->bail("opcode not implemented");
    }
    ip++;
  }

  /* Compile the function */
  jit_function_compile(function);

  /* Unlock the context */
  jit_context_build_end(context);

  return jit_function_to_closure(function);
}
