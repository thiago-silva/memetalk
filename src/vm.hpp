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
class MECImage;
class MMObj;
class Process;


typedef boost::unordered_map<std::string, oop, boost::hash<std::string>,
                             std::equal_to<std::string>,
                             gc_allocator<std::pair<std::string,oop> > >
                            symbol_map_t;

typedef boost::unordered_map<oop, prim_function_t,
                             boost::hash<oop>,
                             std::equal_to<oop>,
                             gc_allocator<std::pair<oop,prim_function_t> > >
                            prim_map_t;

typedef boost::unordered_map<std::string, MECImage*,
                             boost::hash<std::string>,
                             std::equal_to<std::string>,
                             gc_allocator<std::pair<std::string,MECImage*> > >
                            modules_map_t;


class VM {

public:
  VM(int argc, char** argv, bool online, bool profile, const std::string& core_img_filepath);

  int& argc() { return _argc; };
  char** argv() { return _argv; };

  inline MMObj* mmobj() { return _mmobj; };


  int start();

  oop new_symbol(const char*);
  oop new_symbol(Process*, oop);

  void register_primitive(std::string, prim_function_t);

  // prim_function_t get_primitive(Process*, oop);
  inline prim_function_t get_primitive(Process* proc, oop name) {
    return _primitives.at(name);
  }

  oop instantiate_meme_module(Process*, const char* name, oop module_args_list);
  oop instantiate_local_module(Process*, const char* path, oop module_args_list);

//  oop get_prime(const char* name);
  inline oop get_prime(const char* name) {
    return _core_image->get_prime(name);
  }

  CoreImage* core() { return _core_image; };

  // Process* process() { return _process; };

  oop compile_fun(Process*, const char* text, std::list<std::string>, oop cmod, int*);
  oop recompile_fun(Process*, oop cfun, int line, const char* text, int* exc);

  std::pair<Process*, oop> start_debugger(Process* target);

  void bail(const std::string& msg);
  void bail();

  void print_error(Process* proc, oop retval);

  oop get_compiled_module(Process* proc, std::string name);

  void set_debugger_module(oop module) { _debugger_module = module; };

  char* get_argv(int);

  char* fetch_module(const std::string& filepath, int* file_size);
private:
  // void dump_prime_info();
  // void dictionary_dump(oop dict);
  // void parse_repository_path();
  void maybe_load_primitives_file(const std::string& file_path_without_extension);
  oop maybe_compile_local_source(Process* proc, std::string filepath);
  bool is_mec_file_older_then_source(std::string src_file_path);
  void maybe_load_config();
  void print_retval(Process*, oop retval);
  Process* init();

  MMLog _log;
  int _argc;
  char** _argv;
  bool _online;
  bool _profile;
  CoreImage* _core_image;
  MMObj* _mmobj;
  oop _debugger_module;
  int _first_argv;
//  std::list<Process*> _processes;

  symbol_map_t _symbols;

  prim_map_t _primitives;
  modules_map_t _modules;
  std::string _mec_cache_directory;

  //path configurations
  boost::unordered_map<std::string, std::string> _repo_locations;
  boost::unordered_map<std::string, std::string> _repo_override;
};

#endif
