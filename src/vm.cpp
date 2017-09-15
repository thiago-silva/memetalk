#include "vm.hpp"
#include "process.hpp"
#include "core_image.hpp"
#include "mec_image.hpp"
#include "defs.hpp"
#include "mmobj.hpp"
#include "prims.hpp"
#include "utils.hpp"
#include "mec_fun.hpp"
#include <cstdlib>
#include <boost/filesystem.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#include <gc_cpp.h>
#include "gc/gc_allocator.h"

#define DBG(...) if(_log._enabled) { _log << _log.magenta + _log.bold + "[VM|" << __FUNCTION__ << "] " << _log.normal << __VA_ARGS__; }

#define WARNING() MMLog::warning() << "[VM|" << __FUNCTION__ << "] " << _log.normal
#define ERROR() MMLog::error() << "[VM|" << __FUNCTION__ << "] " << _log.normal


VM::VM(int argc, char** argv, bool online, bool profile, const std::string& core_img_filepath)
  : _log(LOG_VM), _argc(argc), _argv(argv), _online(online), _profile(profile),
    _core_image(new (GC) CoreImage(this, core_img_filepath)), _mmobj(new (GC) MMObj(_core_image)),
    _debugger_module(MM_NULL) {

  _mec_cache_directory = getenv("HOME");
  _mec_cache_directory += "/.memetalk/cache/";
  create_cache_dir(_mec_cache_directory);
  maybe_load_config();
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
  Process* proc = new (GC) Process(this, 0, _online);
  _core_image->load();
  init_primitives(this); //module initialization could execute primitives but
                         //now we are using Symbol objects as entries in the
                         //primitives dict, and without core we don't have the
                         //class Symbol loaded to link objects to.  so lets
                         //just trust core never really `send` -- and probably
                         //shouldn't.
  _mmobj->init();
  return proc;
}



int VM::start() {
  if (_argc < 2) {
    bail("usage: meme <filepath.me> | -m <module>");
  }

  Process* proc = init();

  char* module_path;
  oop imod;
  if (strcmp(_argv[1], "-m") == 0) {
    //meme -m <repo:path@version>
    module_path = _argv[2];
    _first_argv = 2; //first memetalk-land argv
    try {
      imod = instantiate_meme_module(proc, module_path, _mmobj->mm_list_new());
    } catch(mm_exception_rewind e) {
      print_error(proc, e.mm_exception);
      return 1;//* (int*) (void*) &(e.mm_exception);
    }
  } else {
    //meme <filepath.me> We might need to compile it
    imod = maybe_compile_local_source(proc, _argv[1]);
    _first_argv = 1; //first memetalk-land argv
  }

  int exc;
  oop retval = proc->run(imod, new_symbol("main"), &exc);
  if (_profile) {
    proc->report_profile();
  }
  if (!(exc == 0)) {
    proc->fail(retval);
    return 1;
  } else {
    print_retval(proc, retval);
    return 0;
  }
  //return * (int*) (void*) &retval;
}

std::pair<Process*, oop> VM::start_debugger(Process* target) {
  DBG("debugger for target: " << target << endl)

  static int debugger_id = 0;

  oop imod;
  Process* dbg_proc = new (GC) Process(this, ++debugger_id);
  if (_debugger_module != MM_NULL) {
    imod = _debugger_module;
  } else {
    try {
      imod = instantiate_meme_module(dbg_proc, "central:stdlib/remote_repl", _mmobj->mm_list_new());
    } catch(mm_exception_rewind e) {
      ERROR() << "uncaught exception while instantiating debugger module :(" << endl;
      dbg_proc->fail(e.mm_exception);
      // print_retval(e.mm_exception);
      // return * (int*) (void*) &(e.mm_exception);
    }
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
  symbol_map_t::iterator it = _symbols.find(s);
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

oop VM::instantiate_meme_module(Process* proc, const char* mod_path, oop module_args_list) {
  DBG( "instantiating module " << mod_path << endl);
  MECImage* mec;
  modules_map_t::iterator it = _modules.find(mod_path);
  if (it == _modules.end()) {
    DBG("loading new module " << mod_path << endl);
    int file_size;
    char* data = fetch_module(mod_path, &file_size);
    mec = new (GC) MECImage(proc, _core_image, mod_path, file_size, data);
    _modules[mod_path] = mec;
    mec->load();
  } else {
    DBG("module already loaded " << mod_path << endl);
    mec = it->second;
  }
  return mec->instantiate_module(module_args_list);
}

oop VM::instantiate_local_module(Process* proc, const char* mec_file_path, oop module_args_list) {
  DBG( "instantiating module in file " << mec_file_path << endl);
  MECImage* mec;
  modules_map_t::iterator it = _modules.find(mec_file_path);
  if (it == _modules.end()) {
    DBG("loading new module " << mec_file_path << endl);
    int file_size;
    char* data = read_mec_file(mec_file_path, &file_size);
    mec = new (GC) MECImage(proc, _core_image, mec_file_path, file_size, data);
    _modules[mec_file_path] = mec;
    mec->load();
  } else {
    DBG("module already loaded " << mec_file_path << endl);
    mec = it->second;
  }
  return mec->instantiate_module(module_args_list);
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
std::string get_compile_fun_json(const char* text, int line, std::list<std::string> vars) {
  ptree pt;
  ptree env;

  for (std::list<std::string>::iterator it = vars.begin(); it != vars.end(); it++) {
    env.push_back(std::make_pair("", ptree(*it)));
  }

  pt.put ("text", text);
  pt.put("start_line", line);
  pt.add_child("env_names",  env);
  std::ostringstream buf;
  write_json (buf, pt, false);
  return buf.str();
}

oop VM::recompile_fun(Process* proc, oop cfun, int line, const char* text, int* exc) {
  std::list<std::string> vars;
  std::stringstream s;
  DBG("recompiling line " << line << " code: " << text << endl);
  std::string json = get_compile_fun_json(text, line, vars);

  s << "python -m pycompiler.compiler recompile-fun '" << json << "'";
  DBG("Executing python compiler: " << s.str() << endl);
  *exc = 0;
  if (FILE* p = popen(s.str().c_str(), "r")) {
    boost::iostreams::file_descriptor_source d(fileno(p), boost::iostreams::close_handle);
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> pstream(d);
    std::stringstream out;
    out << &pstream;
    std::string data = out.str();

    char* c_data = (char*) GC_MALLOC(sizeof(char) * data.size());
    data.copy(c_data, data.size());

    DBG("Done executing python compiler" << endl);
    if (pclose(p) == 0) {
      MECFunction* mecf = new (GC) MECFunction(this, _core_image, c_data, data.size());
      oop new_cfun = mecf->load(proc);
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
  std::string json = get_compile_fun_json(text, 0, vars);

  s << "python -m pycompiler.compiler compile-lambda '" << json << "'";
  DBG("Executing python compiler: " << s.str() << endl);

  *exc = 0;
  if (FILE* p = popen(s.str().c_str(), "r")) {
    boost::iostreams::file_descriptor_source d(fileno(p), boost::iostreams::close_handle);
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> pstream(d);
    std::stringstream out;
    out << &pstream;
    std::string data = out.str();

    char* c_data = (char*) GC_MALLOC(sizeof(char) * data.size()+1);
    data.copy(c_data, data.size());
    if (pclose(p) == 0) {
      MECFunction* mecf = new (GC) MECFunction(this, _core_image, c_data, data.size());
      oop cfun = mecf->load(proc);
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

char* VM::get_argv(int i) {
  i += _first_argv; //skips vm and vm flags
  if (i < _argc) {
    return _argv[i];
  }
  return NULL;
}

void VM::maybe_load_config() {
  ptree pt;
  std::string home = std::string(getenv("HOME"));
  char pwdc[PATH_MAX];
  getcwd(pwdc, PATH_MAX);

  try {
    boost::property_tree::read_json("meme.config", pt);
  } catch(...) {
    DBG("Unable to read meme.config in current directory" << endl);
    try {
      boost::property_tree::read_json(home + "/.meme.config", pt);
    } catch(...) {
      std::cerr << "fatal error:\n\tUnable to read configuration file\n";
      std::cerr << "\tDirectories searched:\n";
      std::cerr << "\t* " << pwdc << "/meme.config\n";
      std::cerr << "\t* " << home << "/.meme.config\n";
      bail();
    }
  }

  try {
    ptree node = pt.get_child("repositories");
    boost::property_tree::ptree::iterator iter;
    for(iter = node.begin(); iter != node.end(); iter++) {
      DBG("config.repositories " << iter->first << " -> " << iter->second.data() << endl);
      _repo_locations[iter->first] = iter->second.data();
    }

    node = pt.get_child("override_to_local");
    for(iter = node.begin(); iter != node.end(); iter++) {
      DBG("override_to_local: " << iter->first << " -> " << iter->second.data() << std::endl);
      _repo_override[iter->first] = iter->second.data();
    }
  } catch(...) {
    DBG("Could not read data in meme.config");
  }
}

char* VM::fetch_module(const std::string& mod_path, int* file_size) {
  DBG("fetch module '" << mod_path << "'" << std::endl);
  //1-check for overriding rules
  boost::unordered_map<std::string, std::string>::iterator it;
  for (it = _repo_override.begin(); it != _repo_override.end(); it++) {
    DBG("looking for overriding rules: '" << it->first << "'" << endl);
    if (mod_path.find(it->first) == 0) {
      std::string local_path =it->second + mod_path.substr(it->first.length());
      DBG("translating " << mod_path << " to local path " << local_path << std::endl);
      maybe_load_primitives_file(local_path);
      return read_mec_file(local_path + ".mec", file_size);
    }
  }
  for (it = _repo_locations.begin(); it != _repo_locations.end(); it++) {
    if (mod_path.find(it->first) == 0) {
      std::cerr << "TODO: fetching " << mod_path << " on " << it->first << " = " << it->second << std::endl;
      bail();
    }
  }
  bail(std::string("fatal error:\n\tmodule not found: ") + mod_path);
  return 0;
}

void VM::maybe_load_primitives_file(const std::string& file_path_without_extension) {
  struct stat st;

  std::string file_name = file_path_without_extension + "_prims.so";
  DBG("Looking for prim file " << file_name << std::endl);

  if (stat(file_name.c_str(), &st) == -1) {
    // Can't access the file. Get out.
    DBG("No sign of primitives file " << file_name << std::endl);
    return;
  }

  void *handle = dlopen(file_name.c_str(), RTLD_LAZY);
  if (!handle) {
    // Can't open up dynamically linked library
    ERROR() << dlerror() << endl;
    return;
  }

  // Find and execute func that inits primitives from the .so in VM
  void (*init)(VM*);
  *(void **) (&init) = dlsym(handle, "init_primitives");
  if (init == NULL) {
    ERROR() << dlerror() << endl;
  } else {
    DBG("Register primitives in file " << file_name << std::endl);
    init(this);
  }
}

bool VM::is_mec_file_older_then_source(std::string src_file_path) {
  std::string mec_file_path = src_file_path + "c";

  boost::filesystem::path src_p(src_file_path);
  boost::filesystem::path mec_p(mec_file_path);
  if (boost::filesystem::exists(mec_file_path)) {
    std::time_t t_mec = boost::filesystem::last_write_time(mec_p) ;
    std::time_t t_src = boost::filesystem::last_write_time(src_p) ;
    return (t_mec < t_src);
  } else {
    return true;
  }
}

oop VM::maybe_compile_local_source(Process* proc, std::string filepath) {
  std::string line;
  std::fstream file;
  if (is_mec_file_older_then_source(filepath)) {
    DBG("we need to compile " << filepath << endl);
    file.open(filepath.c_str(), std::fstream::in | std::fstream::binary);
    if (!file.is_open()) {
      bail(std::string("file not found: ") + filepath);
    }

    std::getline(file, line);
    if (line.find("meme") != 0) {
      bail(std::string("file is not a memetalk source: ") + filepath);
    }
    std::string compiler_module_path = line.substr(5);
    oop imod;
    try {
      DBG("instantiating compiler module : " << compiler_module_path << endl);
      imod = instantiate_meme_module(proc, compiler_module_path.c_str(), _mmobj->mm_list_new());
    } catch(mm_exception_rewind e) {
      DBG("uops\n");
      print_error(proc, e.mm_exception);
      bail();
    }
    DBG("compiling source with compiler: " << compiler_module_path << endl);
    int exc;
    oop res = proc->send_1(imod, new_symbol("compile"), _mmobj->mm_string_new(filepath), &exc);
    DBG("finished source with compiler: " << compiler_module_path << endl);
    if (exc) {
      DBG("compiler failed: " << compiler_module_path << endl);
      proc->fail(res);
      bail();
    }
  }
  return instantiate_local_module(proc, (filepath + "c").c_str(), _mmobj->mm_list_new());
}
