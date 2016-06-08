#include <stdlib.h>
#include <string>
#include <stdlib.h>
#include <map>
#include "process.hpp"
#include "defs.hpp"
#include "vm.hpp"
#include "core_image.hpp"
#include "mmc_image.hpp"
#include "mmobj.hpp"
#include "utils.hpp"
#include "ctrl.hpp"
#include <sstream>
#include <assert.h>

#ifdef MM_NO_DEBUG
  #define LOG_HEAD(log)
  #define DBG(...)
  #define LOG_REGISTERS()
  #define LOG_STACK()
  #define LOG_BODY()
  #define LOG_TRACE()
  #define LOG_ENTER_FRAME()
  #define LOG_EXIT_FRAME()
#else
#define LOG_HEAD(log) log << COLOR(log) << "[" << log_label()  << "|" << BCOLOR(log) + meme_curr_fname() << COLOR(log) << "." << __FUNCTION__ << "] " << log.normal

#define DBG(...) if(_log._enabled) { LOG_HEAD(_log) << __VA_ARGS__; }

#define LOG_REGISTERS() if(_log_registers._enabled) { LOG_HEAD(_log_registers) << _log_registers.yellow << "registers: " << endl \
                                      << _log_registers.yellow << "         SP: " << _sp << endl \
                                      << _log_registers.yellow << "         FP: " << _fp << endl \
                                      << _log_registers.yellow << "         BP: " << _bp << endl \
                                      << _log_registers.yellow << "         IP: " << _ip << endl \
                                      << _log_registers.yellow << "         MP: " << _mp << endl \
                                      << _log_registers.yellow << "         CP: " << _cp << endl \
                                      << _log_registers.normal; }

#define LOG_STACK() if (_log_stack._enabled) { LOG_HEAD(_log_stack) << "state: " << _state << " - stack:" << endl \
                         << _log_stack.yellow << dump_stack_top(_log_stack._enabled) << endl; }

#define LOG_BODY() if (_log_body._enabled) { LOG_HEAD(_log_body) << "code body: " << endl << dump_code_body(_log_body._enabled) << endl; }

#define LOG_TRACE() if (_log_stack_trace._enabled) { LOG_HEAD(_log_stack_trace) << dump_stack_trace(_log_stack_trace._enabled) << endl; }

#define LOG_ENTER_FRAME() LOG_TRACE(); LOG_REGISTERS(); LOG_STACK(); LOG_BODY();
#define LOG_EXIT_FRAME()  LOG_TRACE(); LOG_REGISTERS(); LOG_STACK(); LOG_BODY();

#endif

#define BCOLOR(log) (log.normal + (_debugger_id > 0? log.bold + log.cyan : log.bold + log.green))
#define COLOR(log) (log.normal + (_debugger_id > 0?  log.cyan : log.green))

#define WARNING() MMLog::warning() << COLOR(_log) << "[" << log_label() << "|" << BCOLOR(_log) + meme_curr_fname() << COLOR(_log) << "." << __FUNCTION__ << "] " << _log.normal
#define ERROR() MMLog::error() << COLOR(_log) << "[" << log_label() << "|" << BCOLOR(_log) + meme_curr_fname() << COLOR(_log) << "." << __FUNCTION__ << "] " << _log.normal
#define CTXNAME(ctx) _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, ctx), true)
#define TO_C_STR(str) _mmobj->mm_string_cstr(this, str, true)

//#include <boost/unordered_map.hpp>
//typedef boost::unordered_map<unsigned long, entry_t> cache_t;

#define inline_lookup(drecv, vt, selector)                              \
  while (true) {                                                        \
    if (vt == NULL) {                                                   \
      drecv = MM_NULL;                                                  \
      fun = MM_NULL;                                                     \
      break;                                                            \
    }                                                                   \
    oop dict = _mmobj->mm_behavior_get_dict(vt);                        \
                                                                        \
    fun = _mmobj->mm_dictionary_fast_get(this, dict, selector);         \
    if (fun) {                                                          \
      break;                                                            \
    } else {                                                            \
      vt = _mmobj->mm_object_delegate(vt);                              \
      drecv = _mmobj->mm_object_delegate(drecv);                        \
    }                                                                   \
  }                                                                     \



Process::Process(VM* vm, int debugger_id)
  : _log(debugger_id > 0?LOG_DBG_PROC:LOG_TARGET_PROC),
 _log_registers(debugger_id > 0?LOG_TARGET_PROC_REG:LOG_TARGET_PROC_REG),
 _log_stack(debugger_id > 0? LOG_DBG_PROC_STACK:LOG_TARGET_PROC_STACK),
 _log_stack_trace(debugger_id > 0? LOG_DBG_PROC_TRACE:LOG_TARGET_PROC_TRACE),
 _log_body(debugger_id > 0? LOG_DBG_PROC_BODY:LOG_TARGET_PROC_BODY),
 _debugger_id(debugger_id),
 _vm(vm),
 _mmobj(vm->mmobj()),
 _control(new ProcessControl()),
 _dbg_handler(NULL, MM_NULL) {

  init();
  _state = RUN_STATE;
  _unwinding_exception = false;
  _step_bp = MM_NULL;
  _current_exception = MM_NULL;
  _break_only_on_this_module = MM_NULL;
}

std::string Process::dump_stack_trace(bool enabled) {
  if (!enabled) {
    return std::string("");
  }
  std::stringstream s;
  oop bp = _bp;
  while (bp) {
    oop cp = cp_from_base(bp);
    if (!cp) break;

    oop mp = _mmobj->mm_function_get_module(this, cp, true);
    oop cmod = _mmobj->mm_module_get_cmod(mp);
    s << _mmobj->mm_string_cstr(this, _mmobj->mm_compiled_module_name(this, cmod, true), true);
    s << ":" << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, cp), true);
    s << "():" << (_mmobj->mm_function_get_line_for_instruction(
                     this, cp, ip_from_base(bp), true) + 1) << endl;
    bp = *(oop*)bp;
  }
  return s.str();
}

std::string Process::dump_code_body(bool enabled) {
  if (!enabled) {
    return std::string("");
  }
  std::stringstream s;
  if (!_cp) {
    return s.str();
  }
  bytecode* ip = _mmobj->mm_function_get_code(this, _cp, true);
  number size = _mmobj->mm_function_get_code_size(this, _cp, true);
  if (!ip) {
    return s.str();
  }
  for (number i = 0; i < size / 4; i++) {
    if (ip != _ip) {
      s << _log.normal << "*** ip: " << ip << " [" << bytecode_to_str(*ip) << "]" << endl;
    } else {
      s << _log.bold << "*** ip: " << ip << " [" << bytecode_to_str(*ip) << "]" << endl;
    }
    ip++;
  }

  // int i = 0;
  // oop bp = _bp;
  // s << "ORIGINAL _BP: " << _bp << std::endl;
  // while(true) {
  //   s << "X_BP: " << bp << endl;
  //   if (bp > (oop)10)
  //     bp = *(oop*) bp;
  //   else
  //     break;
  //   i++;
  //   if (i > 50) break;
  // }
  return s.str();
}


std::string Process::dump_stack_top(bool enabled) {
  if (!enabled) {
    return std::string("");
  }
  std::stringstream s;
  s << " starting with _bp: " << _bp << std::endl;
  s << "[";

  word* sp = _sp;
  oop bp = _bp;
  // oop cp = _cp;
  // oop fp = _fp;
  // oop mp = _mp;
  while (sp >= _stack) {
    if (bp == sp) {
      s <<  sp << " [BP] " << *(oop*)sp << "\n";
      number ss = *(number*)(sp-1);
      s << (sp-1) << " [SS] " << ss << "\n";
      s << (sp-2) << " [IP] " << *(bytecode*)(sp-2) << "\n";
      s << (sp-3) << " [CP] " << *(oop*)(sp-3);
      if (*(oop*)(sp-3)) {
        s << " - " << CTXNAME(*(oop*)(sp-3)) << "()\n";
      } else {
        s << "\n";
      }
      s << (sp-4) << " [FP] " << *(oop*)(sp-4) << "\n";
      s << (sp-5) << " [DP] " << *(oop*)(sp-5) << "\n";
      s << (sp-6) << " [RP] " << *(oop*)(sp-6) << "\n";
      // number j = 7;
      // for (; j < (ss+7); j++) {
      //   s << (sp-j) << " [LOCAL] " << *(oop*)(sp-j) << "\n";
      // }
      bp = *(oop*)bp;
      sp = sp-6;
    } else {
      s << sp << " " << *(oop*)sp << "\n";
      sp--;
    }
  }
  s << "]";
  return s.str();
}


oop Process::run(oop recv, oop selector_sym, int* exc) {
  //atm, we don't need to check exc
  //since ::unwind_with_exception will terminate the vm
  //if there is no more stack to unwind.
  return send_0(recv, selector_sym, exc);
}

void Process::init() {
  _stack = (word*) malloc(DEFAULT_STACK_SIZE);
  _state = INVALID_STATE;
  _sp = _stack; //stack
  _fp = NULL;
  _bp = NULL; //base
  _ip = NULL; //instruction
  _mp = NULL; //module
  _cp = NULL; //context
  _ss = 0; //local storage
}

// oop Process::get_arg(number idx) {
//   return * ((oop*)_fp + idx);
// };

oop Process::set_rp(oop rp) {
  return * (oop*) (_fp + _ss) = rp;
}

oop Process::set_dp(oop dp) {
  return * (oop*) (_fp + _ss + 1) = dp;
}

// oop Process::rp() {
//   return * (oop*) (_fp + _ss );
// }

// oop Process::dp() {
//   return * (oop*) (_fp + _ss + 1);
// }

oop Process::cp_from_base(oop bp) {
  return * ((oop*)bp - 3);
}

oop Process::fp_from_base(oop bp) {
  return * ((oop*)bp - 4);
}

bytecode* Process::ip_from_base(oop bp) {
  return (bytecode*) * ((oop*)bp - 2);
}

void Process::push_frame(oop recv, oop drecv, number arity, number storage_size) {
  DBG(" recv: " << recv << ", drecv: " << drecv << ", arity: " << arity << ", storage_size: " << storage_size << endl);

  // oop curr_sp = _sp;

  //BEGIN stack frame layout
  //---- if you change this, change pop_frame() and the frame accessor methods
  //---- as well!!

  oop fp = _fp;
  _fp = _sp - (arity - 1);  // _sp points to the last arg pushed

  DBG("before -- sp: " << _sp << " old fp: " << fp << " new fp:" << _fp << endl);

  // for (int i = 0; i < storage_size - arity; i++) {
  //   stack_push(MM_NULL);
  // }
  _sp += (storage_size - arity);

  stack_push(recv);
  stack_push(drecv);

  stack_push(fp);
  stack_push(_cp);
  stack_push(_ip);
  stack_push(_ss); //storage size
  stack_push(_bp);

  //END of frame layout

  _bp = _sp;

  _ss = storage_size;

  LOG_ENTER_FRAME();
}

void Process::pop_frame() {
  number storage_size = _ss;

  DBG("pop_frame begin SP: " << _sp << endl);


  _sp = _bp; //restore sp, ignoring frame data. push_frame/pop_frame are always in sync.

  _bp = stack_pop();
  _ss = (number) stack_pop();
  _ip = (bytecode*) stack_pop();
  _cp = stack_pop();
  _fp = stack_pop();

  _sp = _sp - (storage_size + 2); //2: fp, dp

  if (_cp) {// first frame has _cp = null
    _mp = _mmobj->mm_function_get_module(this, _cp, true);
  } else {
    DBG("MP unloaded is null! we should be in the first stack frame!" << endl);
    _state = RUN_STATE; //without this, we end up debugging the printing of
                        //failed stack trace and it sefgaults!
  }
  LOG_EXIT_FRAME();
}

void Process::unload_fun_and_return(oop retval) {
  pop_frame();
  stack_push(retval);
}

void Process::clear_exception_state() {
    _unwinding_exception = false;
    _current_exception = MM_NULL;
}

oop Process::ctor_rdp_for(oop rp, oop cp) {
  DBG("ctor_rdp_for rp: " << rp << ", cp: " << cp << endl);
  if (!rp) {
    _vm->bail("no rdp for ctor!");
  }
  oop vt = _mmobj->mm_object_vt(rp);
  oop cclass = _mmobj->mm_class_get_compiled_class(this, vt, true);

  oop other_cclass = _mmobj->mm_function_get_owner(this, cp, true);

  DBG("cclass: " << cclass << ", other cclass: " << other_cclass << endl);

  if (cclass == other_cclass) {
    return rp;
  } else {
    return ctor_rdp_for(_mmobj->mm_object_delegate(rp), cp);
  }
}

void Process::basic_new_and_load(oop klass) {
  oop rp = _mmobj->alloc_instance(this, klass);
  oop dp = ctor_rdp_for(rp, _cp);
  DBG(rp << " dp: " << dp << endl);
  set_rp(rp);
  set_dp(dp);
}

void Process::setup_fp(number params, number storage_size) {
  //ideally this should be a calloc followed by memcpy
  oop fp = (oop) calloc(sizeof(oop), storage_size + 2); //+2: space for rp, dp
  DBG("allocated fp: " << fp << " - " << " params: " << params
      << " storage_size: " << storage_size << endl);

  for (int i = 0; i < storage_size + 2; i++) {
    DBG("new_fp[" << i << " - " << (oop*)fp << "]  " << *((oop*)_fp + i) << endl);
    ((oop*)fp)[i] = *((oop*)_fp + i);
  }

  // for (int j = 0, i = params; i > 0; i--, j++) { //copying parameters
  //   ((oop*)fp)[j] = *((oop*)_fp - 2 - i); //-2: rp,dp
  // }
  // ((oop*)fp)[storage_size] =  *((oop*)_fp + storage_size); //rp
  // ((oop*)fp)[storage_size+1] =*((oop*)_fp + storage_size + 1); //dp
  // ((oop*)fp)[storage_size+2] = (oop) params;

  _fp = fp;
}

void Process::restore_fp(oop fp, number params, number env_offset) {
  DBG(fp << " " << params << " " << env_offset << endl);

  for (int i = 0; i < params; i++) {
    DBG("fp[" << env_offset + i << "] = " << * (oop*)(_fp + i) << endl);
    ((oop*)fp)[env_offset + i] = * (oop*)(_fp + i);
  }
  _fp = fp;
}

bool Process::load_fun(oop recv, oop drecv, oop fun, bool should_allocate) {
  if (!(_mmobj->mm_is_function(fun) || _mmobj->mm_is_context(fun))) {
    WARNING() << "Will raise TypeError: Expecting function or context" << endl;
    raise("TypeError", "Expecting function or context");
  }

  DBG(fun << endl);

  if (_mmobj->mm_function_is_getter(this, fun, true)) {
    number idx = _mmobj->mm_function_access_field(this, fun, true);
    DBG("GETTER: idx " << idx << " on " << recv << endl);
    oop val = ((oop*)drecv)[idx];
    DBG("GETTER: pushing retval: " << val << endl);
    stack_push(val);
    return false;
  }

  number num_params = _mmobj->mm_function_get_num_params(this, fun, true);
  number storage_size = _mmobj->mm_function_get_num_locals_or_env(this, fun, true);
  push_frame(recv, drecv, num_params, storage_size);


  DBG("storage: " << _ss << " " << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, fun, true), true) << endl);

  // DBG("line mapping: " << " " << _mmobj->mm_string_cstr(_mmobj->mm_function_get_name(fun)) << endl);
  // oop mapping = _mmobj->mm_function_get_line_mapping(fun);
  // std::map<oop, oop>::iterator it = _mmobj->mm_dictionary_begin(mapping);
  // std::map<oop, oop>::iterator end = _mmobj->mm_dictionary_end(mapping);
  // for ( ; it != end; it++) {
  //   DBG(untag_small_int(it->first) << " => " << untag_small_int(it->second) << endl);
  // }
  // DBG("loc mapping: " << endl);
  // mapping = _mmobj->mm_function_get_loc_mapping(fun);
  // it = _mmobj->mm_dictionary_begin(mapping);
  // end = _mmobj->mm_dictionary_end(mapping);
  // for ( ; it != end; it++) {
  //   DBG(untag_small_int(it->first) << " => [" << _mmobj->mm_list_size(it->second) << "] " );
  //   for(int i = 0; i < _mmobj->mm_list_size(it->second); i++) {
  //     DBG(" " << untag_small_int(_mmobj->mm_list_entry(it->second, i)));
  //   }
  //   DBG(endl);
  // }

  if (_mmobj->mm_is_context(fun)) {
    restore_fp(_mmobj->mm_context_get_env(this, fun, true), num_params, _mmobj->mm_function_get_env_offset(this, fun, true));
  } else {
    if (_mmobj->mm_function_uses_env(this, fun, true)) {
      setup_fp(num_params, storage_size);
    }
  }

  _cp = fun;
  _mp = _mmobj->mm_function_get_module(this, _cp, true);
  DBG("MP for fun load: " << _mp << endl);
  DBG("_cp is " << _cp << endl);

  if (_mmobj->mm_function_is_ctor(this, fun, true) and should_allocate) {
    basic_new_and_load(recv);
  }

  if (_mmobj->mm_function_is_prim(this, fun, true)) {
    oop prim_name = _mmobj->mm_function_get_prim_name(this, fun, true);
    //std::string str_prim_name = _mmobj->mm_string_cstr(this, prim_name, true);
    int ret = execute_primitive(prim_name);
    if (ret == 0) {
      oop value = stack_pop(); //shit
      unload_fun_and_return(value);
      return false;
    } else if (ret == PRIM_HALTED) {
      pop_frame();
      halt_and_debug();
      //return value of calling "debug();"
      stack_push(MM_NULL);
      return false;
    } else if (ret == PRIM_RAISED) {
      oop ex_oop = stack_pop(); //shit
      DBG("prim returned: RAISED " << ex_oop << endl);

      //because `exception_has_handler()` (called by `maybe_debug_on_raise()`)
      //uses `_bp` to go through the stack looking for handlers by grabing the
      //`cp`'s, the first `cp grabbed is our current caller.  So, if we
      //pop_frame() before maybe_debug_on_raise(), the stack won't have the
      //first cp to check for catch() -- it is outside the stack, in our
      //register _cp. So lookup will not check our caller's cp but the caller
      //before it.  Therefore, lets pop_frame() *after* maybe_debug_on_raise()
      pop_frame();
      maybe_debug_on_raise(ex_oop);


      //we rely on compiler generating a pop instruction to bind ex_oop to the catch var
      oop exc_unwind = unwind_with_exception(ex_oop);
      if (exc_unwind != MM_NULL) {
        stack_push(exc_unwind);
      }
        // DBG("load_fun: unwind_with_exception reached primitive. c++ throwing...: " << ex_oop << endl);
      DBG("unwind_with_exception rached catch block for " << ex_oop << endl);
      // oop value = stack_pop(); //shit
      // unload_fun_and_return(value);
      return false;
    }
  }


  // DBG("load_fun: module for fun " << fun << " is " << _mp << endl);
  // DBG("load_fun: is ctor? " << _mmobj->mm_function_is_ctor(fun) << " alloc? " << should_allocate << endl);
  _ip = _mmobj->mm_function_get_code(this, fun, true);
  // DBG("first instruction " << decode_opcode(*_ip) << endl);
  _code_size = _mmobj->mm_function_get_code_size(this, fun, true);
  maybe_break_on_call();
  return true;
}

oop Process::send_0(oop recv, oop selector, int* exc) {
  return do_send(recv, selector, 0, exc);
}

oop Process::send_1(oop recv, oop selector, oop arg, int* exc) {
  stack_push(arg);
  return do_send(recv, selector, 1, exc);
}

oop Process::send(oop recv, oop selector, oop args, int* exc) {
  number num_args = _mmobj->mm_list_size(this, args);
  for (int i = 0; i < num_args; i++) {
    stack_push(_mmobj->mm_list_entry(this, args, i));
  }
  return do_send(recv, selector, num_args, exc);
}

oop Process::super_send(oop recv, oop selector, oop args, int* exc) {
  number num_args = _mmobj->mm_list_size(this, args);
  for (int i = 0; i < num_args; i++) {
    stack_push(_mmobj->mm_list_entry(this, args, i));
  }
  DBG("-- begin super_send, recv: " << recv
      << ", selector: " << _mmobj->mm_symbol_cstr(this, selector, true) << " #args: " << num_args << endl);

  *exc = 0;
  oop fun;
  oop vt = _mmobj->mm_object_delegate(_mmobj->mm_object_vt(recv));
  oop drecv = _mmobj->mm_object_delegate(recv);
  inline_lookup(drecv, vt, selector);

  if (!fun) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector, true) << " not found in object " << recv;
    DBG(s << endl);
    *exc = 1;
    oop ex = mm_exception("DoesNotUnderstand", s.str().c_str());
    WARNING() << "raising DoesNotUnderstand: " << s.str() << endl;
    DBG("-- end do_send returning exception object " << ex << endl);
    return ex;
  }

  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector, true) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    *exc = 1;
    WARNING() << "wrong arity: " << s.str() << endl;
    oop ex = mm_exception("ArityError", s.str().c_str());
    DBG("-- end do_send returning exception object " << ex << endl);
    return ex;
  }

  return protected_fetch_cycle(recv, drecv, fun, exc, true);
}

oop Process::do_send(oop recv, oop selector, int num_args, int *exc) {
  DBG("-- begin do_send, recv: " << recv
      << ", selector: " << _mmobj->mm_symbol_cstr(this, selector, true) << " #args: " << num_args << endl);

  *exc = 0;
  oop fun;
  oop vt = _mmobj->mm_object_vt(recv);
  oop drecv = recv;
  inline_lookup(drecv, vt, selector);

  if (!fun) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector, true) << " not found in object " << recv;
    DBG(s << endl);
    *exc = 1;
    oop ex = mm_exception("DoesNotUnderstand", s.str().c_str());
    WARNING() << "raising DoesNotUnderstand: " << s.str() << endl;
    DBG("-- end do_send returning exception object " << ex << endl);
    return ex;
  }

  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector, true) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    *exc = 1;
    WARNING() << "wrong arity: " << s.str() << endl;
    oop ex = mm_exception("ArityError", s.str().c_str());
    DBG("-- end do_send returning exception object " << ex << endl);
    return ex;
  }

  return protected_fetch_cycle(recv, drecv, fun, exc, true);
}

oop Process::protected_fetch_cycle(oop recv, oop drecv, oop fun, int* exc, bool should_allocate) {
  try {
    try {
      if (load_fun(recv, drecv, fun, should_allocate)) {
        fetch_cycle(_bp);
      }
    } catch(mm_frame_rewind e) {
      //Check if wether we should pop_frame() or throw (if there's a primitive call
      //or something.
      unwind_with_frame(e.mm_frame);
      _state = RUN_STATE;
      clear_exception_state();
      fetch_cycle(_bp); //resume
    }
  } catch(mm_exception_rewind e) {
    *exc = 1;
    DBG("-- end fetch cycle with excepton: " << e.mm_exception << endl);
    return e.mm_exception;
  }

  oop val = stack_pop();
  DBG("-- end fetch cycle with val: " << val << endl);
  return val;
}


// oop Process::do_call_protected(oop fun, int* exc) {
//   int old_state = _state;

//   _state = RUN_STATE;
//   try {
//     oop res = do_call(fun, exc);
//     _state = old_state;
//     return res;
//   } catch( ... ) {
//     _state = old_state;
//     throw;
//   }
// }

oop Process::do_call(oop fun, int* exc) {
  if (!(_mmobj->mm_is_context(fun))) { //since we pass NULL to load_fun
    WARNING() << "Will raise TypeError: Expecting Context" << endl;
    raise("TypeError", "Expecting Context");
  }

  DBG("-- begin: fun: " << fun << " sp: " << _sp << ", fp: " << _fp << endl);

  *exc = 0;
  return protected_fetch_cycle(NULL, NULL, fun, exc, false);
}

oop Process::call(oop fun, oop args, int* exc) {
  if (!(_mmobj->mm_is_context(fun))) { //since we pass NULL to load_fun
    WARNING() << "Will raise TypeError: Expecting Context" << endl;
    raise("TypeError", "expecting Context");
  }
  number num_args = _mmobj->mm_list_size(this, args);
  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, fun)) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    oop ex = mm_exception("ArityError", s.str().c_str());
    WARNING() << "returning ArityError exception object " << ex << endl;
    *exc = 1;
    return ex;
  }

  for (int i = 0; i < num_args; i++) {
    stack_push(_mmobj->mm_list_entry(this, args, i));
  }
  return do_call(fun, exc);
}

void Process::fetch_cycle(void* stop_at_bp) {
  DBG("begin fp:" << _fp <<  " stop_fp:" <<  stop_at_bp
      << " ip: " << _ip << endl);

  //at least one instructionn should be executed.  stop_at_bp is usually the
  //top mark of the stack when a function is loaded (_bp). So starting a fetch
  //cycle when this value is smaller than _bp likely means the bytecodes
  //executed more POPs than it should. Since the frame has been compromised,
  //unloading the current function will further mess things up and so on.
  //Thus, let's just do a hard crash if this happens or if _ip -- which
  //we use below -- is 0.

  assert(((_bp >= stop_at_bp) && _ip)); //"base pointer and stop_at_bp are wrong"

  while ((_bp >= stop_at_bp) && _ip) { // && ((_ip - start_ip) * sizeof(bytecode))  < _code_size) {

    //at tick we may pause for debugging interactions.
    //Because debugger may interfere with _ip,
    //we must take the code from _ip, decode and dispatch
    //*after* the tick()
    if (_vm->running_online()) tick();

    bytecode code = *_ip;


    int opcode = decode_opcode(code);
    int arg = decode_args(code);

    DBG("IP: " << _ip << " with code: " << code << " decoded as opcode " << opcode << ", arg: " << arg << endl);

    _ip++; //this can't be done after dispatch, where an ip for a different fun may be set

    // if (_state != RUN_STATE) {
    //   char* name = _mmobj->mm_string_cstr(_mmobj->mm_function_get_name(_cp));
    //   DBG(" [dispatching] " << name << " " << (_ip-1) << " " << opcode << endl);
    // }
    //dispatch(opcode, arg);
    oop val;
    switch(opcode) {
      case PUSH_LOCAL:
        DBG("PUSH_LOCAL " << arg << " " << (oop) *(_fp + arg) << endl);
        stack_push(*(_fp + arg));
        break;
      case PUSH_LITERAL:
        DBG("PUSH_LITERAL " << arg << " " << _mmobj->mm_function_get_literal_by_index(this, _cp, arg, true) << endl);
        stack_push(_mmobj->mm_function_get_literal_by_index(this, _cp, arg, true));
        break;
      case PUSH_MODULE:
        DBG("PUSH_MODULE " << arg << " " << _mp << endl);
        stack_push(_mp);
        break;
      case PUSH_FIELD:
        DBG("PUSH_FIELD " << arg << " " << (oop) *(dp() + arg + 2) <<  " dp: " << dp() << endl);
        stack_push(*(dp() + arg + 2));
        break;
      case PUSH_THIS:
        DBG("PUSH_THIS " << rp() << endl);
        stack_push(rp());
        break;
      case PUSH_FP:
        DBG("PUSH_FP " << arg << " -- " << _fp << endl);
        stack_push(_fp);
        break;

      case PUSH_CONTEXT:
        DBG("PUSH_CONTEXT " << arg << endl);
        stack_push(_cp);
        break;
      case PUSH_BIN:
        DBG("PUSH_BIN " << arg << endl);
        stack_push(arg);
        break;
      case RETURN_TOP:
        val = stack_pop();
        DBG("RETURN_TOP " << arg << " " << val << endl);
        handle_return(val);
        break;
      case RETURN_THIS:
        DBG("RETURN_THIS " << rp() << endl);
        handle_return(rp());
        break;
      case POP:
        val =stack_pop();
        DBG("POP " << arg << " = " << val << endl);
        break;
      case POP_LOCAL:
        val = stack_pop();
        DBG("POP_LOCAL " << arg << " on " << (oop) (_fp + arg) << " -- "
            << (oop) *(_fp + arg) << " = " << val << endl);
        *(_fp + arg) = (word) val;
        break;
      case POP_FIELD:
        val = stack_pop();
        DBG("POP_FIELD " << arg << " on " << (oop) (dp() + arg + 2) << " dp: " << dp() << " -- "
            << (oop) *(dp() + arg + 2) << " = " << val << endl); //2: vt, delegate
        *(dp() + arg + 2) = (word) val;
        break;
      case SEND:
        DBG("SEND " << arg << endl);
        handle_send(arg);
        break;
      case SUPER_CTOR_SEND:
        handle_super_ctor_send(arg);
        break;
      case CALL:
        DBG("CALL " << arg << endl);
        handle_call(arg);
        break;
      case JMP:
        DBG("JMP " << arg << " " << endl);
        _ip += (arg -1); //_ip already suffered a ++ in dispatch
        break;
      case JMPB:
        DBG("JMPB " << arg << " " << endl);
        _ip -= (arg+1); //_ip already suffered a ++ in dispatch
        break;

      case JZ:
        val = stack_pop();
        DBG("JZ " << arg << " " << val << endl);
        if ((val == MM_FALSE) || (val == MM_NULL)) {
          _ip += (arg -1); //_ip already suffered a ++ in dispatch
        }
        break;
      case SUPER_SEND:
        handle_super_send(arg);
        break;
      // case RETURN_THIS:
      //   break;
      default:
        ERROR() << "Unknown opcode " << opcode << endl;
        _vm->bail("opcode not implemented");
    }
    // DBG(" [end of dispatch] " << opcode << endl);
  }
  DBG("end" << endl);
}

void Process::maybe_break_on_return() {
  if (_state == STEP_INTO_STATE &&
      (_break_only_on_this_module == MM_NULL || _break_only_on_this_module == _mp)) {
    bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
    _volatile_breakpoints.push_back(bytecode_range_t(_ip,last));
  }
  // DBG("maybe_tick_return: " << _step_fp << " " << _fp << endl);
}

void Process::maybe_break_on_call() {
  if (_state == STEP_INTO_STATE &&
      (_break_only_on_this_module == MM_NULL || _break_only_on_this_module == _mp)) {
    bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
    _volatile_breakpoints.push_back(bytecode_range_t(_ip, last));
  }
}

void Process::maybe_break_on_exception() {
  if (_state == STEP_INTO_STATE || _state == STEP_OVER_STATE) {
    DBG("goint to HALT state" << endl);
    _state = HALT_STATE;
  }
}

void Process::tick() {
  if (!_ip) return;

  DBG("breakpoints " << _volatile_breakpoints.size() << " state: " << _state << endl);

  if (_state == RUN_STATE || _state == STEP_INTO_STATE || _state == STEP_OVER_STATE) {
    for(std::list<bytecode_range_t>::iterator it = _volatile_breakpoints.begin(); it != _volatile_breakpoints.end(); it++) {
      DBG("tick: looking for volatile breakpoint "
            << (*it).first << " <= " << _ip << " <= " << (*it).second
          << " -- opcode: " << decode_opcode(*_ip) << endl);
      if ((*it).first <= _ip && (*it).second >= _ip) {
        DBG("BREAK on "  << _ip << " " << decode_opcode(*_ip) << endl);
        _volatile_breakpoints.erase(it);
        goto do_pause;
      }
    }

    for(std::list<bytecode*>::iterator it = _breakpoints.begin(); it != _breakpoints.end(); it++) {
      DBG("tick: looking for breakpoint "
            << (*it) << " <= " << _ip << " -- opcode: " << decode_opcode(*_ip) << endl);
      if ((*it) == _ip) {
        DBG("BREAK on "  << _ip << " " << decode_opcode(*_ip) << endl);
        goto do_pause;
      }
    }

    if (_step_bp >= _bp) {
      DBG("_step_bp >= _bp, lets break" << endl);
      goto do_pause;
    }
  } else if (_state == STEP_OUT_STATE) {
    if (_step_bp >= _bp) {
      DBG("BREAK on FP "  << _fp << " " << _ip << " " << decode_opcode(*_ip) << endl);
      goto do_pause;
    }
  } else if (_state == HALT_STATE) {
    DBG("tick:HALT state. goto pause " << endl);
    goto do_pause;
  }
  return;

  do_pause:
  DBG("do_pause" << endl);
    int exc;
    oop retval = _dbg_handler.first->send_0(_dbg_handler.second,
                                            _vm->new_symbol("process_paused"), &exc);
    check_and_print_exception(_dbg_handler.first, exc, retval);
    // DBG("tick:HALT "
    //           << _mmobj->mm_string_cstr(_mmobj->mm_function_get_name(_cp))
    //           << " next opcode" <<  decode_opcode(*_ip) << endl);
    _state = HALT_STATE;
    _volatile_breakpoints.clear();
    _step_bp = MM_NULL;
    _control->pause();

    DBG("resuming! state == rewind? " << (_state == REWIND_STATE) << endl);
    if (_state == REWIND_STATE) {
      throw mm_frame_rewind(_unwind_to_bp);
    }
    // DBG("Process::tick: resuming..." << endl);
}

void Process::step_into() {
  bytecode* next = _mmobj->mm_function_next_expr(this, _cp, _ip, true);
  DBG("step into next instruction: " << next << endl);
  if (next) {
    // DBG("step_into: BP on next op: " << next << " "
    //           << decode_opcode(*next) << endl);
    bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
    _volatile_breakpoints.push_back(bytecode_range_t(next, last));
  }
  // ? _step_bp = *(oop*)_bp; //previous bp
  _state = STEP_INTO_STATE;
  _control->resume();
}

void Process::step_over() {
  bytecode* next = _mmobj->mm_function_next_expr(this, _cp, _ip, true);
  DBG("step over next instruction: " << next << endl);
  if (next) {
    DBG("step_over: BP on next op: " << next << " "
        << decode_opcode(*next) << endl);
    bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
    _volatile_breakpoints.push_back(bytecode_range_t(next, last));
  }
  _step_bp = *(oop*)_bp; //previous bp
  DBG("step_over will br on: " << _step_bp << " " << std::endl);
  _state = STEP_OVER_STATE;
  _control->resume();
}

void Process::step_over_line() {
  bytecode* next = _mmobj->mm_function_next_line_expr(this, _cp, _ip, true);
  DBG("step over line next instruction: " << next << endl);
  if (next) {
    // DBG("step_over_line: BP on next op: " << next << " "
    //           << decode_opcode(*next) << endl);
    bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
    _volatile_breakpoints.push_back(bytecode_range_t(next, last));
  }
  _step_bp = *(oop*)_bp; //previous frame
  // DBG("ste_over_line will br on FP: " << _step_bp << " " << *((oop*)_bp - 6) << std::endl);
  _state = STEP_OVER_STATE;
  _control->resume();
}

void Process::step_out() {
  // DBG("Process::step_out resuming ..." << endl);
  _step_bp = *(oop*)_bp; //previous bp
  // DBG("ste_out will br on FP: " << _step_bp << " " << std::endl);
  _state = STEP_OUT_STATE;
  _control->resume();
}

void Process::resume() {
  _state = RUN_STATE;
  //clear_exception_state(); make test && "continue" on tests/exception8 segfaults w/ this
  _control->resume();
}

void Process::reload_frame() {
  _sp = _bp; //restore sp, ignoring frame data. push_frame/pop_frame are always in sync.
  _ip = _mmobj->mm_function_get_code(this, _cp, true);

  DBG(" reloading frame back to " << _bp << ", IP: " << _ip << endl);
}

void Process::handle_super_send(number num_args) {
  oop recv = rp();
  oop selector = _vm->new_symbol(this, _mmobj->mm_function_get_name(this, _cp));


  oop fun;
  oop vt = _mmobj->mm_object_vt(recv);
  vt = _mmobj->mm_object_delegate(vt);
  oop drecv = _mmobj->mm_object_delegate(dp());
  inline_lookup(drecv, vt, selector);
  DBG("super lookup FOUND " << fun << endl);

  if (!fun) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector) << " not found in object " << drecv;
    //we rely on compiler generating a pop instruction to bind ex_oop to the catch var
    WARNING() << "will raise DoesNotUnderstand: " << s.str() << endl;
    oop oo_ex = mm_exception("DoesNotUnderstand", s.str().c_str());
    DBG("created exception object " << oo_ex << " on bp: " << _bp << endl);
    maybe_debug_on_raise(oo_ex);
    oop unwind_exc = unwind_with_exception(oo_ex);
    if (unwind_exc != MM_NULL) {
      stack_push(unwind_exc);
    }
    return;
  }

  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, fun)) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    oop ex = mm_exception("ArityError", s.str().c_str());
    WARNING() << "will raise ArityError: " << s.str() << endl;
    maybe_debug_on_raise(ex);
    oop unwind_exc = unwind_with_exception(ex);
    if (unwind_exc) {
      DBG("unwinded ex. returning exception " << ex << endl);
      stack_push(unwind_exc);
    }
    return;
  }
  load_fun(recv, drecv, fun, true);
}

void Process::handle_send(number num_args) {
  oop selector = stack_pop();
  oop recv = stack_pop(); //(oop) * _sp;

  DBG(" SEND " << selector << " name: " << _mmobj->mm_symbol_cstr(this, selector)
      << " " << "recv: " << recv << " vt: " << _mmobj->mm_object_vt(recv) << endl);

  oop fun;
  oop vt = _mmobj->mm_object_vt(recv);
  oop drecv = recv;
  inline_lookup(drecv, vt, selector);
  DBG("Lookup FOUND " << fun << endl);

  if (!fun) {
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector) << " not found in object " << recv;
    //we rely on compiler generating a pop instruction to bind ex_oop to the catch var
    WARNING() << "will raise DoesNotUnderstand: " << s.str() << endl;
    oop oo_ex = mm_exception("DoesNotUnderstand", s.str().c_str());
    DBG("created exception object " << oo_ex << " on bp: " << _bp << endl);
    maybe_debug_on_raise(oo_ex);
    oop unwind_exc = unwind_with_exception(oo_ex);
    if (unwind_exc != MM_NULL) {
      stack_push(unwind_exc);
    }
    return;
  }

  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, fun)) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    oop ex = mm_exception("ArityError", s.str().c_str());
    WARNING() << "will raise ArityError: " << s.str() << endl;
    maybe_debug_on_raise(ex);
    oop unwind_exc = unwind_with_exception(ex);
    if (unwind_exc) {
      DBG("unwinded ex. returning exception " << ex << endl);
      stack_push(unwind_exc);
    }
    return;
  }
  load_fun(recv, drecv, fun, true);
}

void Process::handle_super_ctor_send(number num_args) {
  oop selector = stack_pop();
  // DBG(" SUPER: " << selector << " -- " << _mmobj->mm_symbol_cstr(selector) << endl);

  // lookup starts at the parent of rp's class
  oop instance = dp();
  oop fun;
  oop drecv = _mmobj->mm_object_delegate(instance);
  oop vt = _mmobj->mm_object_vt(_mmobj->mm_object_delegate(_mmobj->mm_object_vt(instance)));
  inline_lookup(drecv, vt, selector);
  if (!fun) {
    DBG("super Lookup failed");
    std::stringstream s;
    s << _mmobj->mm_symbol_cstr(this, selector) << " not found in child object " << instance;
    DBG(s << endl);
    //we rely on compiler generating a pop instruction to bind ex_oop to the catch var
    oop oo_ex = mm_exception("DoesNotUnderstand", s.str().c_str());
    WARNING() << "will raise DoesNotUnderstand: " << s.str() << endl;
    maybe_debug_on_raise(oo_ex);
    oop unwind_exc = unwind_with_exception(oo_ex);
    if (unwind_exc != MM_NULL) {
      stack_push(unwind_exc);
    }
    return;
  }

  DBG("Lookup FOUND " << fun << endl);


  number arity = _mmobj->mm_function_get_num_params(this, fun);
  if (num_args != arity) {
    std::stringstream s;
    s << "arity and num_args differ: " << num_args << " != " << arity;
    DBG(s << endl);
    oop oo_ex = mm_exception("ArityError", s.str().c_str());
    WARNING() << "will raise ArityError: " << s.str() << endl;
    maybe_debug_on_raise(oo_ex);
    oop unwind_exc = unwind_with_exception(oo_ex);
    if (unwind_exc != MM_NULL) {
      stack_push(unwind_exc);
    }
    return;
  }

  load_fun(rp(), drecv, fun, false);
}

void Process::handle_call(number num_args) {
  // DBG("handle_call" << endl);
  oop fun = stack_pop();
  // DBG("handle_call: fn " << fun << endl);
  number arity = _mmobj->mm_function_get_num_params(this, fun);
  // DBG("handle_call: arity " << arity << endl);
  if (num_args != arity) {
    std::stringstream s;
    s << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, fun)) << ": expects " <<  arity << " but got " << num_args;
    DBG(s << endl);
    oop oo_ex = mm_exception("ArityError", s.str().c_str());
    WARNING() << "will raise ArityError: " << s.str() << endl;
    maybe_debug_on_raise(oo_ex);
    oop unwind_exc = unwind_with_exception(oo_ex);
    if (unwind_exc != MM_NULL) {
      stack_push(unwind_exc);
    }
    return;
  }
  load_fun(NULL, NULL, fun, false);

}

void Process::handle_return(oop val) {
  unload_fun_and_return(val);
  maybe_break_on_return();
}

// void Process::dispatch(int opcode, int arg) {
//     // DBG(" executing " << opcode << " " << arg << endl);
//     oop val;
//     switch(opcode) {
//       case PUSH_LOCAL:
//         DBG("PUSH_LOCAL " << arg << " " << (oop) *(_fp + arg) << endl);
//         stack_push(*(_fp + arg));
//         break;
//       case PUSH_LITERAL:
//         DBG("PUSH_LITERAL " << arg << " " << _mmobj->mm_function_get_literal_by_index(this, _cp, arg, true) << endl);
//         stack_push(_mmobj->mm_function_get_literal_by_index(this, _cp, arg, true));
//         break;
//       case PUSH_MODULE:
//         DBG("PUSH_MODULE " << arg << " " << _mp << endl);
//         stack_push(_mp);
//         break;
//       case PUSH_FIELD:
//         DBG("PUSH_FIELD " << arg << " " << (oop) *(dp() + arg + 2) <<  " dp: " << dp() << endl);
//         stack_push(*(dp() + arg + 2));
//         break;
//       case PUSH_THIS:
//         DBG("PUSH_THIS " << rp() << endl);
//         stack_push(rp());
//         break;
//       case PUSH_FP:
//         DBG("PUSH_FP " << arg << " -- " << _fp << endl);
//         stack_push(_fp);
//         break;

//       case PUSH_CONTEXT:
//         DBG("PUSH_CONTEXT " << arg << endl);
//         stack_push(_cp);
//         break;
//       case PUSH_BIN:
//         DBG("PUSH_BIN " << arg << endl);
//         stack_push(arg);
//         break;
//       case RETURN_TOP:
//         val = stack_pop();
//         DBG("RETURN_TOP " << arg << " " << val << endl);
//         handle_return(val);
//         break;
//       case RETURN_THIS:
//         DBG("RETURN_THIS " << rp() << endl);
//         handle_return(rp());
//         break;
//       case POP:
//         val =stack_pop();
//         DBG("POP " << arg << " = " << val << endl);
//         break;
//       case POP_LOCAL:
//         val = stack_pop();
//         DBG("POP_LOCAL " << arg << " on " << (oop) (_fp + arg) << " -- "
//             << (oop) *(_fp + arg) << " = " << val << endl);
//         *(_fp + arg) = (word) val;
//         break;
//       case POP_FIELD:
//         val = stack_pop();
//         DBG("POP_FIELD " << arg << " on " << (oop) (dp() + arg + 2) << " dp: " << dp() << " -- "
//             << (oop) *(dp() + arg + 2) << " = " << val << endl); //2: vt, delegate
//         *(dp() + arg + 2) = (word) val;
//         break;
//       case SEND:
//         DBG("SEND " << arg << endl);
//         handle_send(arg);
//         break;
//       case SUPER_CTOR_SEND:
//         handle_super_ctor_send(arg);
//         break;
//       case CALL:
//         DBG("CALL " << arg << endl);
//         handle_call(arg);
//         break;
//       case JMP:
//         DBG("JMP " << arg << " " << endl);
//         _ip += (arg -1); //_ip already suffered a ++ in dispatch
//         break;
//       case JMPB:
//         DBG("JMPB " << arg << " " << endl);
//         _ip -= (arg+1); //_ip already suffered a ++ in dispatch
//         break;

//       case JZ:
//         val = stack_pop();
//         DBG("JZ " << arg << " " << val << endl);
//         if ((val == MM_FALSE) || (val == MM_NULL)) {
//           _ip += (arg -1); //_ip already suffered a ++ in dispatch
//         }
//         break;
//       case SUPER_SEND:
//         handle_super_send(arg);
//         break;
//       // case RETURN_THIS:
//       //   break;
//       default:
//         ERROR() << "Unknown opcode " << opcode << endl;
//         _vm->bail("opcode not implemented");
//     }
// }

std::pair<oop, oop> Process::lookup(oop drecv, oop vt, oop selector) {
  // assert( *(oop*) selector == _core_image->get_prime("Symbol"));
  if (vt == NULL) {
    return std::pair<oop, oop>(MM_NULL, MM_NULL);
  }

  // DBG("lookup selector on vt: " << vt << " whose vt is " << _mmobj->mm_object_vt(*(oop*)vt) << endl);

  oop dict = _mmobj->mm_behavior_get_dict(vt);
  DBG("dict: " << dict
      << " selector: " << _mmobj->mm_symbol_cstr(this, selector, true) << endl);

  if (_mmobj->mm_dictionary_has_key(this, dict, selector, true)) {
    oop fun = _mmobj->mm_dictionary_get(this, dict, selector, true);
    DBG("Lookup of " << selector << " found in " << vt
        << " fun: " << fun << " drecv: " << drecv << endl);
    std::pair<oop,oop> res = std::pair<oop,oop>(drecv, fun);
    return res;
  } else {
    DBG("Lookup of " << selector << " NOT found in " << vt << ", recursively looking up..." << endl);
    oop pklass = _mmobj->mm_object_delegate(vt);
    oop delegate = _mmobj->mm_object_delegate(drecv);
    return lookup(delegate, pklass, selector);
  }
}

oop Process::stack_pop() {
  oop val = * (oop*)_sp;
  _sp--;
  DBG("POP " << val << " >> " << _sp << endl);
  return val;
}

// void Process::stack_push(oop data) {
//   _sp++;
//   DBG("PUSH " << data << " -> " << _sp << endl);
//   * (word*) _sp = (word) data;
// }

// void Process::stack_push(word data) {
//   _sp++;
//   DBG("PUSH " << (oop) data << " -> " << _sp << endl);
//   * (word*) _sp = data;
// }

// void Process::stack_push(bytecode* data) {
//   _sp++;
//   DBG("PUSH " << data << " -> " << _sp << endl);
//   * (word*) _sp = (word) data;
// }


int Process::execute_primitive(oop name) {
  try {
    int val = _vm->get_primitive(this, name)(this);
    DBG("primitive " << SYM_TO_STR(name) << " returned " << val << endl);
    return val;
  } catch(mm_exception_rewind e) {
    DBG("primitive " << SYM_TO_STR(name) << " raised " << e.mm_exception << endl);
    stack_push(e.mm_exception);
    return PRIM_RAISED;
  }
}


bool Process::exception_has_handler(oop e, oop cp, bytecode* ip, oop next_bp) {
  DBG("** exception_has_handler e: " << e <<
    " on cp: " << cp <<
      ", next_bp:" << next_bp << endl);

  if (cp == NULL) {
    return false;
  }

  DBG("exception_has_handler: " << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_name(this, cp, true), true) << endl);

  if (_mmobj->mm_function_is_prim(this, cp, true)) {
    cp = cp_from_base(next_bp);
    ip = ip_from_base(next_bp);
    return exception_has_handler(e, cp, ip, *(oop*)next_bp);
  }

  bytecode* code = _mmobj->mm_function_get_code(this, cp, true);

  number exception_frames_count = _mmobj->mm_function_exception_frames_count(this, cp, true);
  DBG("exception frames: " << exception_frames_count << endl);
  if (exception_frames_count == 0) { //cp is unable to handle
    cp = cp_from_base(next_bp);
    ip = ip_from_base(next_bp);
    return exception_has_handler(e, cp, ip, *(oop*)next_bp);
  }

  //exception frames are lexically ordered,
  //so iterating normally we always reach the innermost frame first
  oop exception_frames = _mmobj->mm_function_exception_frames(this, cp, true);
  for(int i = 0; i < exception_frames_count; i++) {
    oop frame_begin = exception_frames + (EXCEPTION_FRAME_SIZE * i);

    word try_block =  *(word*) frame_begin;
    word catch_block = *(word*) (frame_begin + 1);
    oop str_type_oop = *(oop*) (frame_begin + 2);

    DBG("exception frame: " << try_block << " " << catch_block << " " << str_type_oop << endl);

    oop type_oop;
    if (str_type_oop == MM_NULL) {
      type_oop  = MM_NULL;
    } else {
      DBG("fetching exception type for name: " << str_type_oop << endl);
      oop mp = _mmobj->mm_function_get_module(this, cp, true);
      int exc;
      type_oop = send_0(mp, _vm->new_symbol(this, str_type_oop), &exc);
      if (!(exc == 0)) {
        ERROR() << "raising InternalError: Unable to get exception type from module" << endl;
        raise("InternalError", "Unable to get exception type from module");
      }
      DBG("fetching exception type got " << type_oop << endl);;
    }

    // DBG("code:  " << code << endl);
    // DBG("cp == _cp?: " << (cp == _cp) << endl);
    // DBG("_ip: " << _ip << endl);
    // DBG("ip from next_bp: " << ip_from_base(next_bp) << " " << next_bp << endl);

    unsigned long instr = ip - code;

    DBG("RAISE instr: " << instr << " at ip: " << ip << " try: " << try_block
        << " catch: " << catch_block << " type: " << type_oop << endl);

    bool delegates_to = false;
    if (type_oop) {
      delegates_to = _mmobj->delegates_or_is_subclass(this, _mmobj->mm_object_vt(e), type_oop);
    }
    DBG("delegates_to == " << delegates_to  << endl);

    DBG("::" <<  (instr >= try_block) << " " << (instr < catch_block)
        << " " << (type_oop == MM_NULL) << " " << delegates_to << endl);

    if (instr >= try_block && instr < catch_block &&
        (type_oop == MM_NULL || delegates_to)) {
      DBG("HAS CATCH " << endl);
      return true;
    }
  }

  cp = cp_from_base(next_bp);
  ip = ip_from_base(next_bp);
  return exception_has_handler(e, cp, ip, *(oop*)next_bp);
}

void Process::fail(oop e) {
    int exc;
    oop str = send_0(e, _vm->new_symbol("stack_trace"), &exc);
    assert(exc == 0);
    ERROR() << _mmobj->mm_string_cstr(this, str, true) << endl;
    _vm->bail();
}

oop Process::unwind_with_exception(oop e) {
  //This function either:
  //-Terminates the VM unwinding all the stack due to lack of catch blocks
  //-returns true in case a catcher was found (_ip is set to the block)
  // the catcher expects e to be on the stack
  //  (maybe I should put the stack_push(e) here, instead of
  //   spreading if(exc_state) stack_push(e)
  //   and also change the signature to return bool instead of oop
  //-returns false, in case execution should resume normally.
  _unwinding_exception = true;
  DBG("** unwind_with_exception e: " << e << " on cp: " << _cp << " state: " << _state << endl);

  // maybe_break_on_exception();//  why is this here? Shouldn't we check if there's no catch{} in the stack?

  DBG("ticking..." << endl);
  tick();

  if (_cp == NULL) {
    //we are already unwinding exception e.
    //if we can't get Exception.toString to work, then let's just break
    // if (!(exc == 0)) {
    //     raise("InternalError", "Unable to get string representation from exception");
    // }

    _state = RUN_STATE; //without this, we end up debugging the printing of
                        //failed stack trace and it sefgaults!

    DBG("cp is NULL!" << endl);
    throw mm_exception_rewind(e);
  }

  DBG("unwind_with_exception: " << CTXNAME(_cp) << endl);

  if (_mmobj->mm_function_is_prim(this, _cp, true)) {
    DBG("->> unwind reached primitive " << _mmobj->mm_string_cstr(this, _mmobj->mm_function_get_prim_name(this, _cp, true), true) << endl);
    throw mm_exception_rewind(e);
  }

  bytecode* code = _mmobj->mm_function_get_code(this, _cp, true);

  number exception_frames_count = _mmobj->mm_function_exception_frames_count(this, _cp, true);
  DBG("exception frames: " << exception_frames_count << endl);
  if (exception_frames_count == 0) { //_cp is unable to handle
    pop_frame();
    tick();
    if (_unwinding_exception) {
      return unwind_with_exception(e);
    } else {
      return MM_NULL;
    }
  }

  //exception frames are lexically ordered,
  //so iterating normally we always reach the innermost frame first
  oop exception_frames = _mmobj->mm_function_exception_frames(this, _cp, true);
  for(int i = 0; i < exception_frames_count; i++) {
    oop frame_begin = exception_frames + (EXCEPTION_FRAME_SIZE * i);

    word try_block =  *(word*) frame_begin;
    word catch_block = *(word*) (frame_begin + 1);
    oop str_type_oop = *(oop*) (frame_begin + 2);

    oop type_oop;
    if (str_type_oop == MM_NULL) {
      type_oop  = MM_NULL;
    } else {
      DBG("fetching exception type for name: " << str_type_oop << endl);
      int exc;
      oop mp = _mmobj->mm_function_get_module(this, _cp, true);
      type_oop = send_0(mp, _vm->new_symbol(this, str_type_oop), &exc);
      if (!(exc == 0)) {
        ERROR() << "Will raise InternalError: Unable to get exception type from module" << endl;
        raise("InternalError", "Unable to get exception type from module");
      }
      DBG("fetching exception type got " << type_oop << endl);;
    }

    word instr = _ip - code;

    DBG("RAISE instr: " << instr << " ip: " << _ip << " try: " << try_block
        << " catch: " << catch_block << " type: " << type_oop << endl);

    //we need to also account for different module instances
    //having different Class instances for the same CompiledClass

    bool delegates_to = false;
    if (type_oop) {
      delegates_to = _mmobj->delegates_or_is_subclass(this, _mmobj->mm_object_vt(e), type_oop);
    }
    DBG("delegates_to == " << delegates_to  << endl);
    if (instr >= try_block && instr < catch_block &&
        (type_oop == MM_NULL || delegates_to)) {
      DBG("CAUGHT " << endl);
      _ip = code + catch_block;
      clear_exception_state();
      return e;
    }
  }

  pop_frame();
  return unwind_with_exception(e);
}


void Process::raise(const char* ex_type_name, const char* msg) {
  WARNING() << ex_type_name << " -- " << msg << endl;
  //TODO: this is used by mmc_image. I think we should just return the
  // exception and let that class deal with it.
  throw mm_exception_rewind(mm_exception(ex_type_name, msg));
}

oop Process::mm_exception(const char* ex_type_name, const char* msg) {
  DBG(ex_type_name << " -- " << msg << endl);
  oop ex_type = _vm->get_prime(ex_type_name);

  int exc;
  oop exobj = send_1(ex_type, _vm->new_symbol("new"), _mmobj->mm_string_new(msg), &exc);
  //if this fails, can't call raise() because it calls mm_exception again
  //recursively. So let's just bail.
  assert(exc == 0);
  DBG("returning ex: " << exobj << endl);
  return exobj;
}

bool Process::has_debugger_attached() {
  return _dbg_handler.first != NULL;
}

void Process::halt_and_debug() {
  DBG("starting new debugger? " << has_debugger_attached() << endl);
  if (!has_debugger_attached()) {
    WARNING() << "starting debugger" << endl;
    _dbg_handler = _vm->start_debugger(this);
    DBG("got a _dbg_handler " << endl);
  }
  DBG("pausing... " << endl);
  step_over();
}

void Process::maybe_debug_on_raise(oop ex_oop) {
  if (!_vm->running_online()) return;

  if (has_debugger_attached() && !exception_has_handler(ex_oop, _cp, _ip, _bp)) {
    DBG("we have a debugger and this exception does not have handler. state=halt" << endl);
    _state = HALT_STATE;
  } else if (!exception_has_handler(ex_oop, _cp, _ip, _bp)) {

    DBG("running online and there's no handler for exception "
        << ex_oop << "; calling halt_and_debug()" << endl);

    _current_exception = ex_oop;
    halt_and_debug();
  } else {
      DBG("maybe_debug_on_raise: NOT. state" << _state << endl);
  }
}


// oop Process::bp_at(unsigned int idx) { //backwards 0 is current
//   if (idx == 0) {
//     // DBG("Process::bp_at BP[" << idx << "] ret&: " << &_bp << endl);
//     return (oop) &_bp;
//   }
//   oop bp = _bp;
//   for (unsigned int i = 0; i < idx-1; i++) {
//     bp = *(oop*) bp;
//     // DBG("Process::bp_at " << i << " = " << bp << endl);
//   }
//   // DBG("Process::bp_at BP[" << idx << "] ret: " << bp << endl);
//   return bp;
// }

// void Process::eval_in_frame(const char* text, number frame_idx) {
//   // oop bp = bt_at(frame_idx);
//   // oop cp = *((oop*)bp - 5);
//   // oop mp = _mmobj->mm_function_get_module(cp);
//   // oop cmod = _mmobj->mm_module_get_cmod(mp);

//   // // get the env table from cp. It should have all names / offsets. See it the order of the names
//   // // matches their _ep offsets, If not, build `vars` from the table.
//   // std::list<std::string> lst = proc->mmobj()->mm_list_to_cstring_list();

//   // int exc;
//   // oop cfun = _vm->compile_fun(text, , cmod, &exc);

//   // if (exc != 0) {
//   //   proc->stack_push(cfun);
//   //   return PRIM_RAISED;
//   // }
// }


void Process::bail(const std::string& msg) {
  _vm->bail(msg);
}

void Process::bail() {
  _vm->bail();
}

const char* Process::meme_curr_fname() {
  if (_cp) {
    return CTXNAME(_cp);
  } else {
    return "?";
  }
}

void Process::break_at_addr(bytecode* addr) {
  DBG("break_at_addr: " << addr << endl);
  bytecode* last = _ip + _mmobj->mm_function_get_code_size(this, _cp, true);
  _volatile_breakpoints.push_back(bytecode_range_t(addr, last));
}

void Process::run_until(oop cfun, number lineno) {
  DBG("run_until: " << cfun << " " << lineno);
  bytecode* b = _mmobj->mm_compiled_function_get_instruction_for_line(this, cfun, lineno);
  if (b) {
    number size = _mmobj->mm_compiled_function_get_code_size(this, cfun);
    _volatile_breakpoints.push_back(bytecode_range_t(b, b+size));
    resume();
  } else {
    WARNING() << "could not bytecode for line " << lineno << endl;
  }
}

void Process::add_breakpoint(oop cfun, number lineno) {
  DBG("add breakpoint: " << cfun << " " << lineno);
  bytecode* b = _mmobj->mm_compiled_function_get_instruction_for_line(this, cfun, lineno);
  if (b) {
    _breakpoints.push_back(b);
  } else {
    WARNING() << "could not bytecode for line " << lineno << endl;
  }
}

bool Process::toggle_module_break_mode() {
  if (_break_only_on_this_module == MM_NULL) {
    _break_only_on_this_module = _mp;
    return true;
  } else {
    _break_only_on_this_module = MM_NULL;
    return false;
  }
}

void Process::rewind_to_frame_and_continue(oop bp) {
  DBG("rewind to bp: " << bp << endl);
  _state = REWIND_STATE;
  _unwind_to_bp = bp;
  _control->resume();
}

// oop Process::unwind_to_frame(oop frame) {
//   _state = REWIND_STATE;
//   _unwind_to_frame = frame;
//   _control->resume();
// }

void Process::unwind_with_frame(oop bp) {
  DBG("unwind from _bp: " << _bp << " to bp: " <<  bp
      << " current _cp: " << _cp << " = " << CTXNAME(_cp) << endl);
  if (_bp == bp) {
    DBG("found bp. stop unwinding" << endl);
    reload_frame();
    return;
  } else if (_mmobj->mm_function_is_prim(this, _cp, true)) {
    DBG("prim, lets throw" << endl);
      throw mm_frame_rewind(bp);
  } else {
    pop_frame();
    unwind_with_frame(bp);
  }
}

std::string Process::log_label() {
  if (_debugger_id == 0) {
    return "Target";
  } else {
    std::stringstream s;
    s << "DBGProc{" << _debugger_id << "}";
    return s.str();
  }
}
