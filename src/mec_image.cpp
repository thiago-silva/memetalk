#include "defs.hpp"
#include "utils.hpp"
#include "mec_image.hpp"
#include "core_image.hpp"
#include "mmobj.hpp"
#include "vm.hpp"
#include "process.hpp"
#include <sstream>

using namespace std;

#define DBG(...) if(_log._enabled) { _log << _log.blue + _log.bold + "[MECImg|" << __FUNCTION__ << "|" << _mod_path <<"] " << _log.normal << __VA_ARGS__; }
#define WARNING() MMLog::warning() << "[MECImg|" << __FUNCTION__ << "" << _mod_path << "] " << _log.normal
#define ERROR() MMLog::error() << "[MECImg|" << __FUNCTION__ << "|" << _mod_path << "] " << _log.normal

word MECImage::HEADER_SIZE = 5 * WSIZE;
word MECImage::MAGIC_NUMBER = 0x420;

MECImage::MECImage(Process* proc, CoreImage* core_image, const std::string& mod_path, int data_size, char* data)
  : _log(LOG_MECIMG), _proc(proc), _mmobj(proc->vm()->mmobj()), _core_image(core_image),
    _mod_path(mod_path), _data_size(data_size), _data(data) {
}

void MECImage::load_header() {
  word magic_number = unpack_word(_data, 0 * WSIZE);
  _ot_size = unpack_word(_data,  1 * WSIZE);
  _er_size = unpack_word(_data, 2 * WSIZE);
  _st_size = unpack_word(_data, 3 * WSIZE);
  _names_size = unpack_word(_data,  4 * WSIZE);

  DBG("Header:magic: " << magic_number << " =?= " << MECImage::MAGIC_NUMBER << endl);
  DBG("Header:ot_size: " << _ot_size << endl);
  DBG("Header:er_size: " << _er_size << endl);
  DBG("Header:st_size: " << _st_size << endl);
  DBG("Header:names_size: " << _names_size << endl);
}

void MECImage::link_external_references() {
  const char* base = _data;
  int start_external_refs = HEADER_SIZE + _names_size + _ot_size;

  for (word i = 0; i < _er_size; i += (2 * WSIZE)) {
    word name_offset = unpack_word(_data, start_external_refs + i);
    char* name = (char*) (base + name_offset);
    word obj_offset = unpack_word(_data, start_external_refs + i + WSIZE);
    // word* obj = (word*) (base + obj_offset);
    // DBG(obj_offset << " - " << *obj << " [" << name << "] -> " << _core_image->get_prime(name) << endl);
    * (word*) (base + obj_offset) = (word) _core_image->get_prime(name);
    // DBG("External refs " << obj_offset << " - " << (oop) *obj << " [" << name << "] -> " << _core_image->get_prime(name) << endl);
  }
}

oop MECImage::instantiate_class(oop class_name, oop cclass, oop cclass_dict, std::map<std::string, oop>& mod_classes, oop imodule) {
  char* cname = _mmobj->mm_string_cstr(_proc, class_name);
  DBG("Instantiating class " << cname << endl);
  oop super_name = _mmobj->mm_compiled_class_super_name(_proc, cclass);
  char* super_name_str = _mmobj->mm_string_cstr(_proc, super_name);
  DBG("Super " << super_name_str << endl);

  oop cmod_params_list =   _mmobj->mm_compiled_module_params(_proc, _compiled_module);
  oop cmod_imports_dict = _mmobj->mm_compiled_module_imports(_proc, _compiled_module);
  // number num_params = _mmobj->mm_list_size(
  //   _mmobj->mm_compiled_module_params(_compiled_module));

  oop super_class = NULL;
  if (mod_classes.find(super_name_str) != mod_classes.end()) {
    DBG("Super class already instantiated" << endl);
    super_class = mod_classes.at(super_name_str);
  } else if (_mmobj->mm_dictionary_has_key(_proc, cclass_dict, _proc->vm()->new_symbol(super_name_str)) && //super class is in this module
             strcmp(cname, super_name_str) != 0) { //avoid loop when Foo < Foo
    DBG("Super class not instantiated. recursively instantiate it" << endl);
    super_class = instantiate_class(super_name, _mmobj->mm_dictionary_get(_proc, cclass_dict, _proc->vm()->new_symbol(super_name_str)), cclass_dict, mod_classes, imodule);
  } else if(_mmobj->mm_dictionary_has_key(_proc, cmod_imports_dict, _proc->vm()->new_symbol(super_name_str))) { //super class is imported
    //when loading the imports, have a _import_idx_map<oop[string], int> to indicate where it will be in the imod
    super_class = _mmobj->mm_module_get_param(
      imodule, _import_idx_map[_proc->vm()->new_symbol(super_name_str)]); //_mmobj->mm_dictionary_index_of(cmod_imports_dict, super_name) + num_params
    DBG("Super class is imported name: " << super_class << endl);
  } else  if (_mmobj->mm_list_index_of(_proc, cmod_params_list, super_name) != -1) { //super class is a module parameter
    super_class = _mmobj->mm_module_get_param(
      imodule, _mmobj->mm_list_index_of(_proc, cmod_params_list, super_name));
    DBG("Super class is module parameter: " << super_class << endl);
  }
  else if (_core_image->has_class(super_name_str)) { //superclass in core
    DBG("Super class got from super module (core)" << endl);
    super_class = _core_image->get_prime(super_name_str);
  } else {
    std::stringstream s;
    s << "Super class not found: " << super_name_str;
    _proc->raise("ImportError", s.str().c_str());
  }

  oop class_funs_dict = _mmobj->mm_cfuns_to_funs_dict(_proc, _mmobj->mm_compiled_class_own_methods(_proc, cclass), imodule);
  oop class_behavior = _mmobj->mm_class_behavior_new(_proc, super_class, class_funs_dict);

  oop funs_dict = _mmobj->mm_cfuns_to_funs_dict(_proc, _mmobj->mm_compiled_class_methods(_proc, cclass), imodule);
  number num_fields = _mmobj->mm_compiled_class_num_fields(_proc, cclass);
  oop klass = _mmobj->mm_class_new(_proc, class_behavior, super_class, funs_dict, cclass, num_fields);
  mod_classes[cname] = klass;
  DBG("User class " << cname << " = "
          << klass << " behavior: " << class_behavior
          << " super_class: " << super_class
          << " compiled_class: " << cclass
          << " dict: " << funs_dict
          << " own_dict: " << class_funs_dict
      << endl);
  return klass;
}


void MECImage::assign_module_arguments(oop imodule, oop mod_arguments_list) {
  oop cmod_params_list =   _mmobj->mm_compiled_module_params(_proc, _compiled_module);

  if (!(_mmobj->mm_list_size(_proc, cmod_params_list) >= _mmobj->mm_list_size(_proc, mod_arguments_list))) {
      _proc->raise("ImportError", "could not assign module arguments");
  }
  for (int i = 0; i < _mmobj->mm_list_size(_proc, mod_arguments_list); i++) {
    oop entry = _mmobj->mm_list_entry(_proc, mod_arguments_list, i);
    DBG(i << " " << entry << endl);
    _mmobj->mm_module_set_module_argument(imodule, entry,i);
  }
}

void MECImage::load_default_dependencies_and_assign_module_arguments(oop imodule) {
  //for each p in _cmod->default_params:
  //   imod = load it
  //   idx = index(p.name, cmod->params)
  //   imod[idx] = imod
  oop params_list = _mmobj->mm_compiled_module_params(_proc, _compiled_module);
  oop default_locations_dict = _mmobj->mm_compiled_module_default_locations(_proc, _compiled_module);
  // number dict_size = _mmobj->mm_dictionary_size(default_locations_dict);
  // for (int i = 0; i < dict_size; i++) {
  boost::unordered_map<oop, oop>::iterator it = _mmobj->mm_dictionary_begin(_proc, default_locations_dict);
  boost::unordered_map<oop, oop>::iterator end = _mmobj->mm_dictionary_end(_proc, default_locations_dict);
  for ( ; it != end; it++) {
    oop lhs_name = it->first; //_mmobj->mm_dictionary_entry_key(default_params_dict, i);
    oop mod_name = it->second; //_mmobj->mm_dictionary_entry_value(default_params_dict, i);
    char* str_mod_name = _mmobj->mm_string_cstr(_proc, mod_name);
    DBG(">>> instantiating default module: " << str_mod_name << endl);
    oop imd = _proc->vm()->instantiate_meme_module(_proc, str_mod_name,
                                      _mmobj->mm_list_new());
    DBG("<<<< DONE instantiating default module: " << str_mod_name << endl);
    number midx = _mmobj->mm_list_index_of(_proc, params_list, _mmobj->mm_symbol_to_string(_proc, lhs_name));
    if (midx == -1) {
      DBG("raising Import error on " << _mmobj->mm_symbol_cstr(_proc, lhs_name) << endl);
      _proc->raise("ImportError", "Could not bind unknown module parameter to default value");
    }
    _mmobj->mm_module_set_module_argument(imodule, imd, midx);
  }
}

void MECImage::load_imports(oop imodule, oop imports_dict, number num_params) {
  // [print] <= test;
  number num_imports = _mmobj->mm_dictionary_size(_proc, imports_dict);
  oop cmod_params_list =   _mmobj->mm_compiled_module_params(_proc, _compiled_module);
  DBG(num_imports << endl);

  boost::unordered_map<oop, oop>::iterator it = _mmobj->mm_dictionary_begin(_proc, imports_dict);
  boost::unordered_map<oop, oop>::iterator end = _mmobj->mm_dictionary_end(_proc, imports_dict);
  for (int i = 0 ; it != end; it++, i++) {
    oop import_name = it->first; //_mmobj->mm_dictionary_entry_key(imports_dict, i);
    oop module_param_name = it->second; //_mmobj->mm_dictionary_entry_value(imports_dict, i);
    number param_index = _mmobj->mm_list_index_of(_proc, cmod_params_list, module_param_name);
    oop module_param_object = _mmobj->mm_module_get_param(imodule, param_index);

    DBG("import name " << _mmobj->mm_symbol_cstr(_proc, import_name)
            << " module_param: " << _mmobj->mm_string_cstr(_proc, module_param_name)
        << " param_index: " << param_index << endl);

    int exc;
    oop entry = _proc->send_0(module_param_object, import_name, &exc);
    if (exc != 0) {
      _proc->raise("ImportError", "could not load imports");
    }

    DBG("entry: " << entry << " in imod idx: " << i + num_params << endl);
    _mmobj->mm_module_set_module_argument(imodule, entry, i + num_params);
    _import_idx_map[import_name] = i + num_params;
  }
}

void MECImage::create_import_getters(oop imodule, oop imod_dict,
                                    oop imports_dict, number num_params) {
  DBG("Creating imports: " << _mmobj->mm_dictionary_size(_proc, imports_dict) << endl);

  boost::unordered_map<oop, oop>::iterator it = _mmobj->mm_dictionary_begin(_proc, imports_dict);
  boost::unordered_map<oop, oop>::iterator end = _mmobj->mm_dictionary_end(_proc, imports_dict);
  for (int i = 0 ; it != end; it++, i++) {
    oop name = it->first; //_mmobj->mm_dictionary_entry_key(imports_dict, i);
    // oop module_param_name = _mmobj->mm_dictionary_entry_value(imports_dict, i);
    char* str = _mmobj->mm_symbol_cstr(_proc, name);
    DBG("Creating getter for import " << str << " " << i << endl);
    oop getter = _mmobj->mm_new_slot_getter(_proc, imodule,
                                            _compiled_module, _mmobj->mm_symbol_to_string(_proc, name),
                                            num_params + 4 + i); //imod: vt, delegate, dict, cmod
    _mmobj->mm_dictionary_set(_proc, imod_dict, _proc->vm()->new_symbol(str), getter);
    DBG("import dict has " << _mmobj->mm_dictionary_size(_proc, imod_dict) << endl);
  }
}

void MECImage::create_param_getters(oop imodule, oop imod_dict, oop params_list) {
  number num_params = _mmobj->mm_list_size(_proc, params_list);

  for (int i = 0; i < num_params; i++) {
    oop name = _mmobj->mm_list_entry(_proc, params_list, i);
    char* str = _mmobj->mm_string_cstr(_proc, name);
    DBG("Creating getter for param " << str << " " << i << endl);
    oop getter = _mmobj->mm_new_slot_getter(_proc, imodule, _compiled_module, name, i + 4); //imod: vt, delegate, dict, cmod
    _mmobj->mm_dictionary_set(_proc, imod_dict, _proc->vm()->new_symbol(str), getter);
  }
}


void MECImage::check_module_arity(oop module_arguments_list) {
  oop params = _mmobj->mm_compiled_module_params(_proc, _compiled_module);
  number num_params = _mmobj->mm_list_size(_proc, params);

  DBG("Compiled module arity: " << num_params << endl);

  number num_args = _mmobj->mm_list_size(_proc, module_arguments_list);
  DBG("received args: " << num_args << endl);

  oop default_params_dict = _mmobj->mm_compiled_module_default_locations(_proc, _compiled_module);
  number dict_size = _mmobj->mm_dictionary_size(_proc, default_params_dict);
  DBG("default params: " << dict_size << endl);
  if (num_params != (dict_size + num_args)) {
    DBG("module arity differ: "
            << num_params << " != " <<
        _mmobj->mm_list_size(_proc, module_arguments_list) << endl);
    _proc->raise("ImportError", "module arity differ");
  }
}

oop MECImage::instantiate_module(oop module_arguments_list) {
  DBG(" ============ instantiate module ===========" << endl);

  // word* cmod = (word*) _compiled_module;
  // DBG("CompiledModule: " << cmod << endl);
  // DBG("CompiledModule vt: " << (word*) *cmod << endl);

  oop cclass_dict = _mmobj->mm_compiled_module_classes(_proc, _compiled_module);

  DBG("CompiledModule class_dict: " << cclass_dict << endl);
  // DBG("CompiledModule class_dict vt: " << (word*) *((word*)cclass_dict) << endl);

  number num_classes = _mmobj->mm_dictionary_size(_proc, cclass_dict);
  DBG("CompiledModule num_classes: " << num_classes << endl);

  oop params = _mmobj->mm_compiled_module_params(_proc, _compiled_module);

  check_module_arity(module_arguments_list);

  number num_params = _mmobj->mm_list_size(_proc, params);

  oop imports_dict = _mmobj->mm_compiled_module_imports(_proc, _compiled_module);
  number num_imports =  _mmobj->mm_dictionary_size(_proc, imports_dict);

  oop imodule = _mmobj->mm_module_new(num_params + num_imports + num_classes,
                                      _compiled_module,
                                      _core_image->get_module_instance());

  oop name = _mmobj->mm_compiled_module_name(_proc, _compiled_module);
  DBG("imodule " << imodule << " name:" << _mmobj->mm_string_cstr(_proc, name) << " params:" << num_params
          << " imports " << num_imports
      << " classes:" << num_classes << " (size: " << (num_params + num_classes + num_imports) << ")" << endl);

  oop fun_dict = _mmobj->mm_compiled_module_functions(_proc, _compiled_module);

  // number num_funs = _mmobj->mm_dictionary_size(fun_dict);

  // DBG("CompiledModule num_functions: " << num_funs << endl);

  oop imod_dict = _mmobj->mm_dictionary_new();
  _mmobj->mm_module_set_dictionary(_proc, imodule, imod_dict);

  if (_mmobj->mm_list_size(_proc, module_arguments_list) > 0) {
    assign_module_arguments(imodule, module_arguments_list);
  }

  load_default_dependencies_and_assign_module_arguments(imodule);

  create_param_getters(imodule, imod_dict, params);

  load_imports(imodule, imports_dict, num_params);

  create_import_getters(imodule, imod_dict, imports_dict, num_params);

  int imod_dict_idx = num_params + num_imports;

  // //for each CompiledFunction:
  // // mod[dict] += Function
  // // mod[i] = Function

  DBG("total module functions: " << _mmobj->mm_dictionary_size(_proc, fun_dict) << endl);

  boost::unordered_map<oop, oop>::iterator it = _mmobj->mm_dictionary_begin(_proc, fun_dict);
  boost::unordered_map<oop, oop>::iterator end = _mmobj->mm_dictionary_end(_proc, fun_dict);
  for ( ; it != end; it++) {
    oop sym_name = it->first; //_mmobj->mm_dictionary_entry_key(fun_dict, i);
    char* str = _mmobj->mm_symbol_cstr(_proc, sym_name);
    oop cfun = it->second; //_mmobj->mm_dictionary_entry_value(fun_dict, i);
    oop fun = _mmobj->mm_function_from_cfunction(_proc, cfun, imodule);
    _mmobj->mm_dictionary_set(_proc, imod_dict, _proc->vm()->new_symbol(str), fun);
    imod_dict_idx++;
  }

  DBG("total module classes: " << _mmobj->mm_dictionary_size(_proc, cclass_dict) << endl);

  std::map<std::string, oop> mod_classes; //store each Class created here, so we do parent look up more easily

  int imod_idx = num_params + num_imports + 4; //imod: vt, delegate, dict, cmod

  it = _mmobj->mm_dictionary_begin(_proc, cclass_dict);
  end = _mmobj->mm_dictionary_end(_proc, cclass_dict);
  for ( ; it != end; it++) {
    oop sym_name = it->first; //_mmobj->mm_dictionary_entry_key(cclass_dict, i);
    oop str_name = _mmobj->mm_symbol_to_string(_proc, sym_name);
    char* cname = _mmobj->mm_symbol_cstr(_proc, sym_name);

    DBG("Found class " << cname << endl);

    oop cclass = it->second; //_mmobj->mm_dictionary_entry_value(cclass_dict, i);
    oop klass;

    if (mod_classes.find(cname) != mod_classes.end()) {
      DBG("Class " << cname << " already instantiated" << endl);
      klass = mod_classes.at(cname);
    } else {
      //recursively instantiate it
      klass = instantiate_class(str_name, cclass, cclass_dict, mod_classes, imodule);
    }

    * (oop*) & imodule[imod_idx] = klass;
    oop klass_getter = _mmobj->mm_new_slot_getter(_proc, imodule, cclass, str_name, imod_idx);

    imod_idx++;

    _mmobj->mm_dictionary_set(_proc, imod_dict, _proc->vm()->new_symbol(cname), klass_getter);
    imod_dict_idx++;
  }
  DBG(" ============ DONE instantiate module ===========" << endl);
  return imodule;
}


oop MECImage::load() {
  DBG(" ============ Load module ===========" << endl);
  load_header();
  relocate_addresses(_data, _data_size, HEADER_SIZE + _names_size + _ot_size + _er_size + _st_size);
  link_external_references();
  link_symbols(_data, _st_size, HEADER_SIZE + _names_size + _ot_size + _er_size, _proc->vm(), _core_image);
  _compiled_module = (oop) * (word*)(& _data[HEADER_SIZE + _names_size]);

  DBG(" ============ Done load module ===========" << endl);
  return _compiled_module;
}
