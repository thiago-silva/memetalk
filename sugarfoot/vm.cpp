#include "vm.hpp"
#include "process.hpp"
#include "core_image.hpp"
#include "mmc_image.hpp"
#include "defs.hpp"
#include "mmobj.hpp"
#include "prims.hpp"
#include "utils.hpp"
#include "mmc_fun.hpp"
#include <cstdlib>


#define DBG(...) if(_log._enabled) { _log << _log.magenta + _log.bold + "[VM|" << __FUNCTION__ << "] " << _log.normal << __VA_ARGS__; }

#define WARNING() MMLog::warning() << "[VM|" << __FUNCTION__ << "] " << _log.normal
#define ERROR() MMLog::error() << "[VM|" << __FUNCTION__ << "] " << _log.normal


VM::VM(int argc, char** argv, bool online, const char* core_img_filepath)
  : _log(LOG_VM), _argc(argc), _argv(argv), _online(online),
    _core_image(new CoreImage(this, core_img_filepath)), _mmobj(new MMObj(_core_image)) {
}

// MMObj* VM::mmobj() {
//   return _mmobj;
// }

// void VM::dictionary_dump(oop dict) {
//   number size = _mmobj->mm_dictionary_size(dict);

//   DBG() << "  dict vt: " << _mmobj->mm_object_vt(dict) << endl;
//   DBG() << "  dict size: " << size << endl;
//   for (int i = 3; i < (3 + (size*2)); i++) {
//     DBG() << "  [" << i << "]: & " << &((oop*)dict)[i];
//     DBG() << endl;
//     // DBG() << " vt:" << * ((oop**)dict)[i]  << endl;
//   }
// }

// void VM::dump_prime_info() {
//   std::map<std::string, oop> primes = _core_image->get_primes();
//   std::map<std::string, oop>::iterator it;
//   for (it = primes.begin(); it != primes.end(); it++) {
//     if (it->first == "Behavior" || it->first == "Object_Behavior") continue;
//     oop klass = it->second;
//     DBG() << "PRIME " << klass << " name: " << _mmobj->mm_string_cstr(_mmobj->mm_class_name(klass)) << " vt: "
//             << _mmobj->mm_object_vt(klass) << " dict:" << _mmobj->mm_class_dict(klass) << endl;
//     dictionary_dump(_mmobj->mm_class_dict(klass));
//   }
// }

void VM::print_retval(Process* proc, oop retval) {
  int exc;
  DBG(" ======== the end ======= " << endl)
  oop retval_str = proc->send_0(retval, new_symbol("toString"), &exc);
  DBG("RETVAL: " << retval << " => " << _mmobj->mm_string_cstr(proc, retval_str) << endl)
}

void VM::print_error(Process* proc, oop retval) {
  int exc;
  oop retval_str = proc->send_0(retval, new_symbol("toString"), &exc);
  ERROR() << "uncaught exception: " << retval << " => " << _mmobj->mm_string_cstr(proc, retval_str) << endl;
}

Process* VM::init() {
  Process* proc = new Process(this);
  init_primitives(this); //module initialization could execute primitives
  _core_image->load();
  _mmobj->init();
  return proc;
}



int VM::start() {
  if (_argc != 2) {
    bail("usage: sf-vm <file.mmc>");
  }

  Process* proc = init();

  char* filepath = _argv[1];


  // dump_prime_info();

  oop imod;
  try {
    imod = instantiate_module(proc, filepath, _mmobj->mm_list_new());
  } catch(mm_exception_rewind e) {
    print_error(proc, e.mm_exception);
    return * (int*) (void*) &(e.mm_exception);
  }

  int exc;
  oop retval = proc->run(imod, new_symbol("main"), &exc);
  if (!(exc == 0)) {
    proc->fail(retval);
  } else {
    print_retval(proc, retval);
  }
  return * (int*) (void*) &retval;
}

std::pair<Process*, oop> VM::start_debugger(Process* target) {
  DBG("debugger for target: " << target << endl)

  static int debugger_id = 0;

  oop imod;
  Process* dbg_proc = new Process(this, ++debugger_id);
  try {
    imod = instantiate_module(dbg_proc, "remote_repl", _mmobj->mm_list_new());
  } catch(mm_exception_rewind e) {
    ERROR() << "uncaught exception while instantiating debugger module :(" << endl;
    dbg_proc->fail(e.mm_exception);
    // print_retval(e.mm_exception);
    // return * (int*) (void*) &(e.mm_exception);
  }

  oop oop_target_proc = _mmobj->mm_process_new(target, target);
  int exc;
  oop handler = dbg_proc->send_1(imod, new_symbol("debug"), oop_target_proc, &exc);
  if (exc != 0) {
    dbg_proc->fail(handler);
  }
  return std::pair<Process*, oop>(dbg_proc, handler);
}

oop VM::new_symbol(const char* cstr) {
  std::string s = cstr;
  boost::unordered_map<std::string, oop>::iterator it = _symbols.find(s);
  if (it == _symbols.end()) {
    oop sym = _mmobj->mm_symbol_new(cstr);
    _symbols[s] = sym;
    return sym;
  } else {
    return _symbols[s];
  }
}

oop VM::new_symbol(Process* p, oop str) {
  if (!(_mmobj->mm_object_vt(str) == _core_image->get_prime("String"))) {
    WARNING() << " expected String object, got:" << str << endl;
    p->raise("TypeError", "Expecting String");
  }
  return new_symbol(_mmobj->mm_string_cstr(p, str));
}

void VM::register_primitive(std::string name, prim_function_t fun) {
  _primitives[new_symbol(name.c_str())] = fun;
}

// prim_function_t VM::get_primitive(Process* proc, oop name) {
//   // DBG("VM::get_primitive " << name << endl)
//   // boost::unordered_map<std::string, prim_function_t>::iterator it = _primitives.find(name);
//   // if (it == _primitives.end()) {
//   //   ERROR() << "did not find primitive with name:" << name << endl;
//   //   proc->raise("InternalError", (std::string("primitive not found: ") + name).c_str());
//   // }
//   // return it->second;;
//   return _primitives.at(name);
// }

oop VM::instantiate_module(Process* proc, const char* name_or_path, oop module_args_list) {
  DBG( "instantiating module " << name_or_path << endl);
  MMCImage* mmc;
  std::map<std::string, MMCImage*>::iterator it = _modules.find(name_or_path);
  if (it == _modules.end()) {
    DBG("loading new module " << name_or_path << endl);
    mmc = new MMCImage(proc, _core_image, name_or_path);
    _modules[name_or_path] = mmc;
    mmc->load();
  } else {
    DBG("module already loaded " << name_or_path << endl);
    mmc = it->second;
  }
  return mmc->instantiate_module(module_args_list);
}

// oop VM::get_prime(const char* name) {
//   return _core_image->get_prime(name);
// }

#include <iostream>
#include <cstdio>
#include <sstream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

static
std::string get_compile_fun_json(const char* text, std::list<std::string> vars) {
  ptree pt;
  ptree env;

  for (std::list<std::string>::iterator it = vars.begin(); it != vars.end(); it++) {
    env.push_back(std::make_pair("", *it));
  }

  pt.put ("text", text);
  pt.add_child("env_names",  env);
  std::ostringstream buf;
  write_json (buf, pt, false);
  return buf.str();
}

oop VM::recompile_fun(Process* proc, oop cfun, const char* text, int* exc) {
  std::list<std::string> vars;
  std::stringstream s;
  std::string json = get_compile_fun_json(text, vars);

  s << "python -m pycompiler.compiler recompile-fun '" << json << "'";
  DBG("Executing python compiler: " << s.str() << endl);
  *exc = 0;
  if (FILE* p = popen(s.str().c_str(), "r")) {
    boost::iostreams::file_descriptor_source d(fileno(p), boost::iostreams::close_handle);
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> pstream(d);
    std::stringstream out;
    out << &pstream;
    std::string data = out.str();

    char* c_data = (char*) calloc(sizeof(char), data.size());
    data.copy(c_data, data.size());

    DBG("Done executing python compiler" << endl);
    if (pclose(p) == 0) {
      MMCFunction* mmcf = new MMCFunction(this, _core_image, c_data, data.size());
      oop new_cfun = mmcf->load(proc);
      _mmobj->mm_overwrite_compiled_function(proc, cfun, new_cfun);
      return cfun;
    } else {
      *exc = 1;
      return proc->mm_exception("CompileError", c_data);
    }
  }
  *exc = 1;
  ERROR() << "Could not call python compiler :(" << endl;
  return proc->mm_exception("CompileError", "pipe error: Unable to call compiler");
}

oop VM::compile_fun(Process* proc, const char* text, std::list<std::string> vars, oop cmod, int* exc) {
  std::stringstream s;
  std::string json = get_compile_fun_json(text, vars);

  s << "python -m pycompiler.compiler compile-lambda '" << json << "'";
  DBG("Executing python compiler: " << s.str() << endl);

  *exc = 0;
  if (FILE* p = popen(s.str().c_str(), "r")) {
    boost::iostreams::file_descriptor_source d(fileno(p), boost::iostreams::close_handle);
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> pstream(d);
    std::stringstream out;
    out << &pstream;
    std::string data = out.str();

    char* c_data = (char*) calloc(sizeof(char), data.size()+1);
    data.copy(c_data, data.size());
    if (pclose(p) == 0) {
      MMCFunction* mmcf = new MMCFunction(this, _core_image, c_data, data.size());
      oop cfun = mmcf->load(proc);
      _mmobj->mm_compiled_function_set_cmod(proc, cfun, cmod);
      return cfun;
    } else {
      *exc = 1;
      return proc->mm_exception("CompileError", c_data);
    }
  }
  *exc = 1;
  ERROR() << "Could not call python compiler :(" << endl;
  return proc->mm_exception("CompileError", "pipe error: Unable to call compiler");
}

void VM::bail(const std::string& msg) {
  std::cerr << msg << std::endl;
  exit(1);
}

void VM::bail() {
  exit(1);
}

oop VM::get_compiled_module(Process* proc, std::string name) {
  if (_modules.find(name) == _modules.end()) {
    proc->raise("KeyError", "module not found");
  }
  return _modules[name]->compiled_module();
}
