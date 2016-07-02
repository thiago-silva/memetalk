#ifndef MMC_LOADER
#define MMC_LOADER

#include "defs.hpp"
#include "log.hpp"
#include <map>
#include <string>

class CoreImage;
class Process;
class MMObj;

class MMCImage {
  static word MAGIC_NUMBER;
  static word HEADER_SIZE;
public:
  MMCImage(Process*, CoreImage*, const std::string&);
  oop load();
  oop instantiate_module(oop);

  oop compiled_module() { return _compiled_module; }

private:
  void check_module_arity(oop module_arguments_list);
  oop instantiate_class(oop, oop, oop, std::map<std::string, oop>&, oop);
  void load_header();
  void link_external_references();

  void assign_module_arguments(oop imodule, oop module_arguments_list);
  void load_default_dependencies_and_assign_module_arguments(oop imodule);

  void load_aliases(oop imodule, oop aliases_dict, number num_params);
  void create_alias_getters(oop imodule, oop imod_dict, oop aliases_dict, number num_params);

  void  create_param_getters(oop imodule, oop imod_dict, oop params_list);

  MMLog _log;
  Process* _proc;
  MMObj* _mmobj;
  CoreImage* _core_image;

  std::string _name_or_path;
  int _data_size;
  char* _data;

  //header
  word _ot_size;
  word _st_size;
  word _er_size;
  word _names_size;

  oop _compiled_module;
  std::map<oop, int> _alias_idx_map;
};


#endif
