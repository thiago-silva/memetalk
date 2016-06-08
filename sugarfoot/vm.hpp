#ifndef VM_HPP
#define VM_HPP

#include "defs.hpp"
#include "log.hpp"
#include <map>
#include <boost/unordered_map.hpp>
#include <string>
#include <list>
#include "process.hpp"
#include "mmobj.hpp"
#include "core_image.hpp"

class CoreImage;
class MMCImage;
class MMObj;
class Process;

class VM {

public:
  VM(int argc, char** argv, bool online, const char* core_img_filepath);

  int& argc() { return _argc; };
  char** argv() { return _argv; };

  inline bool running_online() { return _online; };

  inline MMObj* mmobj() { return _mmobj; };


  int start();

  oop new_symbol(const char*);
  oop new_symbol(Process*, oop);

  void register_primitive(std::string, prim_function_t);

  // prim_function_t get_primitive(Process*, oop);
  inline prim_function_t get_primitive(Process* proc, oop name) {
    return _primitives.at(name);
  }

  oop instantiate_module(Process*, const char* name, oop module_args_list);

//  oop get_prime(const char* name);
  inline oop get_prime(const char* name) {
    return _core_image->get_prime(name);
  }

  CoreImage* core() { return _core_image; };

  // Process* process() { return _process; };

  oop compile_fun(Process*, const char* text, std::list<std::string>, oop cmod, int*);
  oop recompile_fun(Process*, oop cfun, const char* text, int* exc);

  std::pair<Process*, oop> start_debugger(Process* target);

  void bail(const std::string& msg);
  void bail();

  void print_error(Process* proc, oop retval);

  oop get_compiled_module(Process* proc, std::string name);

private:
  // void dump_prime_info();
  // void dictionary_dump(oop dict);
  void print_retval(Process*, oop retval);
  Process* init();

  MMLog _log;
  int _argc;
  char** _argv;
  bool _online;
  CoreImage* _core_image;
  MMObj* _mmobj;
//  std::list<Process*> _processes;

  boost::unordered_map<std::string, oop> _symbols;

  boost::unordered_map<oop, prim_function_t> _primitives;
  std::map<std::string, MMCImage*> _modules;
};

#endif
