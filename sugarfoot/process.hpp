#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <string>
#include <map>
#include <list>
#include <fstream>

#include "defs.hpp"
#include "log.hpp"

class VM;
class MMObj;
class ProcessControl;

typedef std::pair<bytecode*,bytecode*> bytecode_range_t;

class mm_exception_rewind {
public:
  mm_exception_rewind(oop ex) : mm_exception(ex) {}
  oop mm_exception;
};

class mm_frame_rewind {
public:
  mm_frame_rewind(oop fp) : mm_frame(fp) {}
  oop mm_frame;
};


class Process {
  enum {
    INVALID_STATE,
    RUN_STATE,
    STEP_INTO_STATE,
    STEP_OVER_STATE,
    STEP_OUT_STATE,
    REWIND_STATE,
    HALT_STATE
  };


public:
  Process(VM*, int debugger_id = 0);

  oop run(oop, oop, int*);


  bool is_running() { return _state == RUN_STATE; }

  void fail(oop e);

  inline VM* vm() { return _vm; }
  inline oop sp() { return _sp; }
  inline oop fp() { return _fp; }
  inline oop cp() { return _cp; }
  inline oop mp() { return _mp; }
  inline oop bp() { return _bp; }
  inline bytecode* ip() { return _ip; }

  inline oop rp() { return * (oop*) (_fp + _ss ); }
  inline oop dp() { return * (oop*) (_fp + _ss + 1); }

  oop set_rp(oop);
  oop set_dp(oop);

  inline oop get_arg(number idx) { return * ((oop*)_fp + idx); }
  oop cp_from_base(oop bp);
  oop fp_from_base(oop bp);
  bytecode* ip_from_base(oop bp);

  oop top_frame() { return (oop) &_bp; };
  // oop get_rp() { return _rp; };
  // oop get_dp() { return _dp; };

  inline MMObj* mmobj() { return _mmobj; }

  inline void stack_push(oop data) {
      _sp++;
    * (word*) _sp = (word) data;
  }
  inline void stack_push(word data) {
  _sp++;
  * (word*) _sp = data;
  }
  inline void stack_push(bytecode* data) {
    _sp++;
  * (word*) _sp = (word) data;
  }

  oop current_exception() { return _current_exception; }

  oop unwind_with_exception(oop);

  // oop do_call_protected(oop, int*);
  oop call(oop, oop, int*);
  oop do_call(oop, int*);

  oop send_0(oop recv, oop selector, int* exc);
  oop send_1(oop recv, oop selector, oop arg, int* exc);
  oop send(oop recv, oop selector, oop args, int* exc);
  oop do_send(oop recv, oop selector, int num_args, int *exc);

  oop super_send(oop recv, oop selector, oop args, int* exc);
  std::pair<oop,oop> lookup(oop, oop, oop);

  void raise(const char*, const char*);
  oop mm_exception(const char*, const char*);

  void maybe_debug_on_raise(oop);
  void halt_and_debug();
  void step_into();
  void step_over();
  void step_over_line();
  void step_out();
  void resume();
  void reload_frame();

  bool toggle_module_break_mode();

  word* stack() { return _stack; };

  void bail(const std::string& msg);
  void bail();

  void unload_fun_and_return(oop retval);

  bool has_debugger_attached();

  void clear_exception_state();

  void break_at_addr(bytecode*);
  void run_until(oop, number line);
  void add_breakpoint(oop, number line);

  void rewind_to_frame_and_continue(oop frame);

  oop protected_fetch_cycle(oop recv, oop drecv, oop fun, int* exc, bool should_allocate);
  void unwind_with_frame(oop frame);

  std::string dump_code_body(bool);
  std::string dump_stack_top(bool);
  std::string dump_stack_trace(bool);

private:
  std::string log_label();
  const char* meme_curr_fname();

  void init();

  bool load_fun(oop, oop, oop, bool);

  void** create_threaded_code(void** table, bytecode* ip, number size);


  oop stack_pop();
  int execute_primitive(oop);
  void fetch_cycle(void*);

  void push_frame(oop,oop,number, number);
  void pop_frame();

  oop ctor_rdp_for(oop rp, oop cp);

  void setup_fp(number, number);
  void copy_params_to_env(oop, number, number);
  void restore_fp(oop, number, number);

  void dispatch(int, int);
  void handle_send(number);
  void handle_super_send(number num_arg);
  void handle_super_ctor_send(number);
  void handle_call(number);
  void handle_return(oop);
  void basic_new_and_load(oop);

  bool exception_has_handler(oop e, oop cp, bytecode* ip, oop bp);

  void tick();
  void maybe_break_on_call();
  void maybe_break_on_return();
  void maybe_break_on_exception();

  MMLog _log;
  MMLog _log_registers;
  MMLog _log_stack;
  MMLog _log_stack_trace;
  MMLog _log_body;

  int _debugger_id;
  VM* _vm;
  MMObj* _mmobj;

  int _state;
  bool _unwinding_exception;
  ProcessControl* _control;
  std::pair<Process*, oop> _dbg_handler;

  //this order is important: it reflects the order of registers
  //in the stack, and is used by bp_at()
  oop _fp;
  oop _cp;
  bytecode* _ip;
  void** _tcode;
  number _ss;
  oop _bp;
  //
  oop _mp;
  word* _sp;

  word* _stack;
  std::list<bytecode_range_t> _volatile_breakpoints;
  std::list<bytecode*> _breakpoints;
  oop _break_only_on_this_module;
  oop _step_bp;
  oop _unwind_to_bp;
  oop _current_exception;
  void ** _dispatch_table;
};

#endif
