#include "mmobj.hpp"
#include "defs.hpp"
#include "core_image.hpp"
#include <cstdlib>
#include "vm.hpp"
#include "process.hpp"
#include "utils.hpp"
#include <assert.h>
#include <map>
#include <stdlib.h>
#include <cstring>
#include <limits.h>



#ifdef MM_NO_DEBUG
  #define DBG(...)
  #define WARNING()
  #define ERROR()
  #define TYPE_CHECK(invalid_condition, ex_type, ex_msg)
#else
  #define DBG(...) if(_log._enabled) { _log << _log.yellow + _log.bold + "[MMOBJ|" << __FUNCTION__ << "] " << _log.normal << __VA_ARGS__; }
  #define WARNING() MMLog::warning() << "[MMOBJ|" << __FUNCTION__ << "] " << _log.normal
  #define ERROR() MMLog::error() << "[MMOBJ|" << __FUNCTION__ << "] " << _log.normal
  #define TYPE_CHECK(invalid_condition, ex_type, ex_msg)       \
  if (invalid_condition) {                                   \
    if(should_assert) {                                      \
      assert(!(invalid_condition));                          \
    } else {                                                 \
      p->raise(ex_type, ex_msg);                             \
    }                                                        \
  }
#endif


MMObj::MMObj(CoreImage* core)
: _log(LOG_MMOBJ), _core_image(core) {

}

void MMObj::init() {
  _cached_context = _core_image->get_prime("Context");
  _cached_symbol = _core_image->get_prime("Symbol");
  _cached_string = _core_image->get_prime("String");
}

oop MMObj::mm_process_new(Process* p, Process* proc) {
  oop obj = alloc_instance(p, _core_image->get_prime("Process"));
  ((oop*)obj)[2] = (oop) proc;
  return obj;
}

Process* MMObj::mm_process_get_proc(Process* p, oop proc, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(proc) == _core_image->get_prime("Process")),
             "TypeError", "Expected Process")
  return (Process*) (((oop*)proc)[2]);
}

oop MMObj::mm_frame_new(Process* p, oop bp) {
  oop obj = alloc_instance(p, _core_image->get_prime("Frame"));
  ((oop*)obj)[2] = bp;
  return obj;
}

oop MMObj::mm_frame_get_bp(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError", "Expected Frame")
  return ((oop*)frame)[2];
}

oop MMObj::mm_frame_get_cp(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop bp = mm_frame_get_bp(p, frame, should_assert);
  return *((oop*)bp - 3);
}

oop MMObj::mm_frame_get_fp(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop bp = mm_frame_get_bp(p, frame, should_assert);
  return *((oop*)bp - 4);
}

number MMObj::mm_frame_get_ss(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop bp = mm_frame_get_bp(p, frame, should_assert);
  return (number) *((oop*)bp - 1);
}

oop MMObj::mm_frame_get_rp(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop fp = mm_frame_get_fp(p, frame, should_assert);
  number ss = mm_frame_get_ss(p, frame, should_assert);
  return * (oop*) (fp + ss);
}

oop MMObj::mm_frame_get_dp(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop fp = mm_frame_get_fp(p, frame, should_assert);
  number ss = mm_frame_get_ss(p, frame, should_assert);
  return * (oop*) (fp + ss + 1);
}

bytecode* MMObj::mm_frame_get_ip(Process* p, oop frame, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(frame) == _core_image->get_prime("Frame")),
             "TypeError","Expected Frame")
    oop bp = mm_frame_get_bp(p, frame, should_assert);
  return (bytecode*) *((oop*)bp - 2);
}

oop MMObj::mm_module_new(int num_fields, oop cmod, oop delegate) {
  oop imodule = (oop) calloc(4 + num_fields, sizeof(word)); //4: vt, delegate, dict, cmod

  ((word**) imodule)[0] = imodule; //imodule[vt] = imodule
  ((word**) imodule)[1] = delegate; // imodule[delegate]
  //imod[dict]
  ((word**) imodule)[3] = cmod; // imodule[cmod]
  return imodule;
}

oop MMObj::mm_compiled_module_name(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[2];
}

//license 3

oop MMObj::mm_compiled_module_params(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[4];
}


oop MMObj::mm_compiled_module_default_params(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[5];
}

oop MMObj::mm_compiled_module_aliases(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[6];
}

oop MMObj::mm_compiled_module_functions(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[7];
}

oop MMObj::mm_compiled_module_classes(Process* p, oop cmod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cmod) == _core_image->get_prime("CompiledModule")),
             "TypeError","Expected CompiledModule")
  return (oop) ((word*)cmod)[8];
}

oop MMObj::mm_boolean_new(number val) {
  return val ? MM_TRUE : MM_FALSE;
}

bool MMObj::mm_boolean_cbool(Process* p, oop val, bool should_assert) {
  TYPE_CHECK(!(val == MM_TRUE || val == MM_FALSE),
             "TypeError","Expecting boolean")
  return val == MM_TRUE;
}

oop MMObj::mm_object_new() {
  oop obj = (oop) malloc(sizeof(word) * 2); // vt, delegate

  ((word**) obj)[0] = _core_image->get_prime("Object");
  ((word**) obj)[1] = 0;
  return obj;
}

oop MMObj::mm_object_vt(oop obj) {
  if (is_small_int(obj)) {
    return _core_image->get_prime("Number");
  } else if (obj == MM_TRUE) {
    return _core_image->get_prime("Boolean");
  } else if (obj == MM_FALSE) {
    return _core_image->get_prime("Boolean");
  } else if (obj == MM_NULL) {
    return _core_image->get_prime("Null");
  } else {
    return * (oop*) obj;
  }
}

oop MMObj::mm_object_delegate(oop obj) {
  if (is_small_int(obj) || (obj == MM_TRUE) || (obj == MM_FALSE) || (obj == MM_NULL)) {
    return obj; //mm_object_new(); //TODO: this should probably be a dummy static object
  } else {
    return ((oop*) obj)[1];
  }
}

oop MMObj::mm_list_new() {
  oop obj = (oop) malloc(sizeof(word) * 4); // vt, delegate, size, elements frame

  ((word**) obj)[0] = _core_image->get_prime("List");
  ((word**) obj)[1] = mm_object_new();
  ((word*)  obj)[2] = -1;
  ((std::vector<oop>**) obj)[3] = new std::vector<oop>;
  return obj;
}

number MMObj::mm_list_size(Process* p, oop list, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")
    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);
  return elements->size();
}

oop MMObj::mm_list_entry(Process* p, oop list, number idx, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")
    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);
  if (idx >= elements->size()) {
    p->raise("IndexError", "out of bounds index");
  }
  return (*elements)[idx];
}

std::vector<oop>* MMObj::mm_list_frame(Process* p, oop list, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")
  number size = (number) ((oop*)list)[2];
  if (size == -1) {
    return (std::vector<oop>*) ((oop*)list)[3];
  } else {
    ((word*)list)[2] = -1;
    std::vector<oop>* elements = new std::vector<oop>;
    oop frame = ((oop*)list)[3];
    for (int i = 0; i < size; i++) {
      elements->push_back(((oop*)frame)[i]);
    }
    ((std::vector<oop>**) list)[3] = elements;
    return elements;
  }
}

number MMObj::mm_list_index_of(Process* p, oop list, oop elem, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")
    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);


  std::vector<oop>::iterator it = elements->begin();
  for (number pos = 0; it != elements->end(); it++, pos++) {
    if (*it == elem) {
      return pos;
    //some temporary hack while we don't have a self-host compiler doing Object.==
    } else if (mm_is_string(elem) && mm_is_string(*it)) {
      DBG(mm_string_cstr(p, elem, should_assert) << " <cmp> " << mm_string_cstr(p, *it, should_assert) << endl);
      if (strcmp(mm_string_cstr(p, elem, should_assert), mm_string_cstr(p, *it, should_assert)) == 0) {
        return pos;
      }
    }
  }
  return -1;
}

void MMObj::mm_list_set(Process* p, oop list, number idx, oop element, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")
    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);
  (*elements)[idx] = element;
}


void MMObj::mm_list_prepend(Process* p, oop list, oop element, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")

    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);
  elements->insert(elements->begin(), element);
}

void MMObj::mm_list_append(Process* p, oop list, oop element, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(list) == _core_image->get_prime("List")),
             "TypeError","Expected List")

    std::vector<oop>* elements = mm_list_frame(p, list, should_assert);
  elements->push_back(element);
}

oop MMObj::mm_dictionary_new() {
  int basic = 4; //vt, delegate, size, frame
  oop obj = (oop) malloc(sizeof(word) * basic);

  ((word**) obj)[0] = _core_image->get_prime("Dictionary");
  ((word**) obj)[1] = mm_object_new();
  ((word*) obj)[2] = -1;
  ((boost::unordered_map<oop, oop>**) obj)[3] = new boost::unordered_map<oop, oop>;
  return obj;
}

boost::unordered_map<oop, oop>* MMObj::mm_dictionary_frame(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

  number size = (number) ((oop*)dict)[2];
  if (size == -1) {
    return (boost::unordered_map<oop, oop>*) ((oop*)dict)[3];
  } else {
    ((word*)dict)[2] = -1;
    boost::unordered_map<oop, oop>* elements = new boost::unordered_map<oop, oop>;
    oop frame = ((oop*)dict)[3];
    for (int i = 0, j = 0; i < size; i++, j += 2) {
      (*elements)[((oop*)frame)[j]] = ((oop*)frame)[j+1];
    }
    ((boost::unordered_map<oop, oop>**) dict)[3] = elements;
    return elements;
  }
}

boost::unordered_map<oop,oop>::iterator MMObj::mm_dictionary_begin(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  return elements->begin();
}

boost::unordered_map<oop,oop>::iterator MMObj::mm_dictionary_end(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  return elements->end();
}

number MMObj::mm_dictionary_size(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  return elements->size();
}

bool MMObj::mm_dictionary_has_key(Process* p, oop dict, oop key, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  // DBG(dict << "(" << elements->size() << ") has key " << key << " ?" << endl);
  for (boost::unordered_map<oop, oop>::iterator it = elements->begin(); it != elements->end(); it++) {
    if (it->first == key) {
      return true;
    } else if (mm_is_string(key) && mm_is_string(it->first)) {
      // DBG(dict << "(" << elements->size() << ") has key " << mm_string_cstr(key) << " ?=" << mm_string_cstr(it->first) << endl);
      if (strcmp(mm_string_cstr(p, key, should_assert), mm_string_cstr(p, it->first, should_assert)) == 0) {
        return true;
      }
    }
  }
  return false;
}

oop MMObj::mm_dictionary_keys(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

  oop lst = mm_list_new();
  boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  for (boost::unordered_map<oop, oop>::iterator it = elements->begin(); it != elements->end(); it++) {
    mm_list_append(p, lst, it->first, should_assert);
  }
  return lst;
}

oop MMObj::mm_dictionary_values(Process* p, oop dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
  oop lst = mm_list_new();
  boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  for (boost::unordered_map<oop, oop>::iterator it = elements->begin(); it != elements->end(); it++) {
    mm_list_append(p, lst, it->second, should_assert);
  }
  return lst;
}


oop MMObj::mm_dictionary_get(Process* p, oop dict, oop key, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);

  DBG(dict << "(" << elements->size() << ") get " << key << endl);
  for (boost::unordered_map<oop, oop>::iterator it = elements->begin(); it != elements->end(); it++) {
    // DBG(dict << "(" << elements->size() << ") get direct? " << (it->first == key) << endl);
    if (it->first == key) {
      return it->second;
    } else {
      // DBG(dict << "(" << elements->size() << ") get string? " << (mm_is_string(key) && mm_is_string(it->first)) << endl);
      if (mm_is_string(key) && mm_is_string(it->first)) {
        // DBG(dict << "(" << elements->size() << ") get: " << mm_is_string(key) << " =?= " << mm_string_cstr(it->first) << endl);
        if (strcmp(mm_string_cstr(p, key, should_assert), mm_string_cstr(p, it->first, should_assert)) == 0) {
          return it->second;
        }
      }
    }
  }
  return MM_NULL;
}


void MMObj::mm_dictionary_set(Process* p, oop dict, oop key, oop value, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

    boost::unordered_map<oop, oop>* elements = mm_dictionary_frame(p, dict, should_assert);
  (*elements)[key] = value;
}


void MMObj::mm_module_set_dictionary(Process* p, oop imodule, oop imod_dict, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(imod_dict) == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
  ((word**) imodule)[2] = imod_dict;
}

oop MMObj::mm_module_dictionary(oop imodule) {
  return ((oop*) imodule)[2];
}

void MMObj::mm_module_set_module_argument(oop imodule, oop arg, number idx) {
  ((oop*)imodule)[idx+4] = arg; //4: vt, delegate, dict, cmod
}

oop MMObj::mm_module_get_param(oop imodule, number idx) {
  return ((oop*)imodule)[idx+4]; //4: vt, delegate, dict, cmod
}

oop MMObj::mm_module_entry(oop imodule, number idx) {
  return ((oop*)imodule)[idx+4]; //4: vt, delegate, dict, cmod
}

oop MMObj::mm_module_get_cmod(oop imodule) {
  return ((oop*)imodule)[3]; //3: vt, delegate, dict, cmod
}


oop MMObj::mm_string_new(const char* str) {
  number payload = sizeof(oop) + strlen(str) + 1; //size, <str ...>, \0

  oop oop_str = mm_new(_core_image->get_prime("String"),
                       mm_object_new(), //assuming String < Object
                       payload); //this is going to alloc more than we need as it
                                 //is payload * sizeof(oop)
  DBG("string size: " << strlen(str) << " for: [[" << str << "]]" << endl);
  ((oop*)oop_str)[2] = (oop) strlen(str);
  strcpy((char*)(oop_str + 3), str);
  return oop_str;
}


bool MMObj::mm_is_function(oop obj) {
  return *(oop*) obj == _core_image->get_prime("Function");
}

void MMObj::mm_context_set_cfun(Process* p, oop ctx, oop cfun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(ctx) == _core_image->get_prime("Context")),
             "TypeError","Expected Context")
  ((oop*)ctx)[2] = cfun;
}

void MMObj::mm_context_set_module(Process* p, oop ctx, oop imod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(ctx) == _core_image->get_prime("Context")),
             "TypeError","Expected Context")
  ((oop*)ctx)[3] = imod;
}

void MMObj::mm_context_set_env(Process* p, oop ctx, oop env, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(ctx) == _core_image->get_prime("Context")),
             "TypeError","Expected Context")
  ((oop*)ctx)[4] = env;
}

oop MMObj::mm_context_get_env(Process* p, oop ctx, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(ctx) == _core_image->get_prime("Context")),
             "TypeError","Expected Context")
  return ((oop*)ctx)[4];
}


bool MMObj::mm_function_uses_env(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
    oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_uses_env(p, cfun, should_assert);
}

oop MMObj::mm_function_from_cfunction(Process* p, oop cfun, oop imod, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cfun) == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")

  oop fun = (oop) malloc(sizeof(word) * 4); //vt, delegate, cfun, module

  * (oop*) fun = _core_image->get_prime("Function");
  * (oop*) &fun[1] = mm_object_new();
  * (oop*) &fun[2] = cfun;
  * (oop*) &fun[3] = imod;
  return fun;
}

oop MMObj::mm_function_get_module(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!(mm_object_vt(fun) == _core_image->get_prime("Function") ||
               mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError", "Expected Function or Context")

  return (oop) ((oop*)fun)[3];
}

oop MMObj::mm_function_get_cfun(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  return (oop) ((oop*)fun)[2];
}

bool MMObj::mm_function_is_prim(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
    oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_is_prim(p, cfun, should_assert);
}

oop MMObj::mm_function_get_name(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_name(p, cfun, should_assert);
}

oop MMObj::mm_function_get_prim_name(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_prim_name(p, cfun, should_assert);
}


bytecode* MMObj::mm_function_get_code(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_code(p, cfun, should_assert);
}

number MMObj::mm_function_get_code_size(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_code_size(p, cfun, should_assert);
}

oop MMObj::mm_function_get_literal_by_index(Process* p, oop fun, int idx, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_literal_by_index(p, cfun, idx, should_assert);
}

number MMObj::mm_function_get_num_params(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_num_params(p, cfun, should_assert);
}

number MMObj::mm_function_get_num_locals_or_env(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_num_locals_or_env(p, cfun, should_assert);
}

number MMObj::mm_function_get_env_offset(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_env_offset(p, cfun, should_assert);
}

bool MMObj::mm_function_is_getter(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_is_getter(p, cfun, should_assert);
}

number MMObj::mm_function_access_field(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_access_field(p, cfun, should_assert);
}

oop MMObj::mm_function_get_owner(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_owner(p, cfun, should_assert);
}

bool MMObj::mm_function_is_ctor(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_is_ctor(p, cfun, should_assert);
}

number MMObj::mm_function_exception_frames_count(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_exception_frames_count(p, cfun, should_assert);
}

oop MMObj::mm_function_exception_frames(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_exception_frames(p, cfun, should_assert);
}

oop MMObj::mm_function_env_table(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_env_table(p, cfun, should_assert);
}

oop MMObj::mm_function_get_text(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_text(p, cfun, should_assert);
}

oop MMObj::mm_function_get_line_mapping(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_line_mapping(p, cfun, should_assert);
}

oop MMObj::mm_function_get_loc_mapping(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_loc_mapping(p, cfun, should_assert);
}

oop MMObj::mm_function_get_closures(Process* p, oop fun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_closures(p, cfun, should_assert);
}

bytecode* MMObj::mm_function_next_expr(Process* p, oop fun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_next_expr(p, cfun, ip, should_assert);
}

bytecode* MMObj::mm_function_next_line_expr(Process* p, oop fun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_next_line_expr(p, cfun, ip, should_assert);
}

number MMObj::mm_function_get_line_for_instruction(Process* p, oop fun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(fun) == _core_image->get_prime("Function") ||
                mm_object_vt(fun) == _core_image->get_prime("Context")),
             "TypeError","Expected Function or Context")
  oop cfun = mm_function_get_cfun(p, fun, should_assert);
  return mm_compiled_function_get_line_for_instruction(p, cfun, ip, should_assert);
}


void MMObj::mm_overwrite_compiled_function(Process* p, oop target_cfun, oop origin_cfun, bool should_assert) {
  TYPE_CHECK(!(mm_object_vt(target_cfun) == _core_image->get_prime("CompiledFunction") ||
               mm_object_vt(origin_cfun) == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")

    int fields[] = {3, //params
                    5, //is_prim -
                    6, //prim_name
                    10, //num_params
                    11, //has_env
                    14, //local_size
                    15, //env_offset
                    16, //lit_frame_size
                    17, //lit_frame
                    18, //bytecode_size
                    19, //bytecode
                    20, //exc_frames_count
                    21, //exc_frames
                    22, //env_table
                    23, //text
                    24, //line_mapping
                    25, //loc_mapping
                    26, //closures
                    0};

  int i = 0;
  while (fields[i]) {
    ((oop*)target_cfun)[fields[i]] = ((oop*)origin_cfun)[fields[i]];
    i++;
  }
}

bool MMObj::mm_compiled_function_is_ctor(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( mm_object_vt(cfun) == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[4];
}

bool MMObj::mm_compiled_function_is_prim(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[5];
}

oop MMObj::mm_compiled_function_get_prim_name(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (oop) ((oop*)cfun)[6];
}

oop MMObj::mm_compiled_function_get_name(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (oop) ((oop*)cfun)[2];
}

bool MMObj::mm_compiled_function_is_getter(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((number) ((oop*)cfun)[7]) == 1;
}

number MMObj::mm_compiled_function_access_field(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[8];
}

oop MMObj::mm_compiled_function_get_owner(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[9];
}

void MMObj::mm_compiled_function_set_owner(Process* p, oop cfun, oop owner, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  ((oop*)cfun)[9] = owner;
}

number MMObj::mm_compiled_function_get_num_params(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[10];
}

bool MMObj::mm_compiled_function_uses_env(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (bool) ((oop*)cfun)[11];
}

bool MMObj::mm_compiled_function_is_top_level(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (bool) ((oop*)cfun)[12];
}

oop MMObj::mm_compiled_function_outer_cfun(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[13];
}

number MMObj::mm_compiled_function_get_num_locals_or_env(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[14];
}

number MMObj::mm_compiled_function_get_env_offset(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[15];
}

number MMObj::mm_compiled_function_get_literal_frame_size(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[16];
}

oop MMObj::mm_compiled_function_get_literal_by_index(Process* p, oop cfun, int idx, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  oop* literal_frame =  ((oop**)cfun)[17];
  if (!( (idx * WSIZE) < mm_compiled_function_get_literal_frame_size(p, cfun, should_assert))) {
    p->raise("IndexError", "index of out range");
  }
  return (oop) literal_frame[idx];
}

number MMObj::mm_compiled_function_get_code_size(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[18];
}

bytecode* MMObj::mm_compiled_function_get_code(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (bytecode*) ((oop*)cfun)[19];
}

number MMObj::mm_compiled_function_exception_frames_count(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return (number) ((oop*)cfun)[20];
}

oop MMObj::mm_compiled_function_exception_frames(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[21];
}

oop MMObj::mm_compiled_function_env_table(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[22];
}

oop MMObj::mm_compiled_function_get_text(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[23];
}

oop MMObj::mm_compiled_function_get_line_mapping(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[24];
}

oop MMObj::mm_compiled_function_get_loc_mapping(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[25];
}

oop MMObj::mm_compiled_function_get_closures(Process* p, oop cfun, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  return ((oop*)cfun)[26];
}

// oop MMObj::mm_compiled_function_get_cmod(Process* p, oop cfun, bool should_assert) {
//   if (!( *(oop*) cfun == _core_image->get_prime("CompiledFunction"))) {
//      p->raise("TypeError", "Expected CompiledFunction");
//   }
//   return ((oop*)cfun)[26];
// }

void MMObj::mm_compiled_function_set_cmod(Process* p, oop cfun, oop cmod, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  ((oop*)cfun)[26] = cmod;
}

bytecode* MMObj::mm_compiled_function_next_expr(Process* p, oop cfun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")

  bytecode* base_ip = mm_compiled_function_get_code(p, cfun, should_assert);
  word idx = ip - base_ip;

  oop mapping = mm_compiled_function_get_loc_mapping(p, cfun, should_assert);
  boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(p, mapping, should_assert);
  boost::unordered_map<oop, oop>::iterator end = mm_dictionary_end(p, mapping, should_assert);
  word next_offset = INT_MAX;
  for ( ; it != end; it++) {
    word b_offset = untag_small_int(it->first);
    if ((idx < b_offset) && (next_offset > b_offset)) {
      next_offset = b_offset;
    }
  }
  if (next_offset == INT_MAX) {
    DBG("next_expr for " << idx << " is NULL" << endl);
    return NULL;
  } else {
    return base_ip + next_offset;
  }
}

bytecode* MMObj::mm_compiled_function_next_line_expr(Process* p, oop cfun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")

  bytecode* base_ip = mm_compiled_function_get_code(p, cfun, should_assert);
  word idx = ip - base_ip;

  oop mapping = mm_compiled_function_get_line_mapping(p, cfun, should_assert);
  boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(p, mapping, should_assert);
  boost::unordered_map<oop, oop>::iterator end = mm_dictionary_end(p, mapping, should_assert);
  word current_line = 0;
  for ( ; it != end; it++) {
    word b_offset = untag_small_int(it->first);
    word line = untag_small_int(it->second);
    // DBG(" SEARCH CURR LINE "
    //           << idx << " " << b_offset << " " << line << endl);
    if ((idx >= b_offset) && (current_line < line)) {
      // DBG("GOT LINE " << line << endl);
      current_line = line;
    }
  }

  it = mm_dictionary_begin(p, mapping, should_assert);
  word next_line = INT_MAX;
  word next_offset = 0;
  for ( ; it != end; it++) {
    word line = untag_small_int(it->second);
    // DBG("line: " << line << " " << current_line << " " << next_line << endl);
    if (line > current_line && next_line > line) {
      next_line = line;
      next_offset = untag_small_int(it->first);
    }
  }

  if (next_line == INT_MAX) {
    DBG("next_line for " << idx << " is NULL" << endl);
    return NULL;
  } else {
    // DBG(" CURR LINE " << current_line << " NEXT: " << next_line
    //           << " offset: " << next_offset << endl);
    return base_ip + next_offset;
  }
}

number MMObj::mm_compiled_function_get_line_for_instruction(Process* p, oop cfun, bytecode* ip, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  bytecode* base_ip = mm_compiled_function_get_code(p, cfun, should_assert);
  word idx = ip - base_ip;

  oop mapping = mm_compiled_function_get_line_mapping(p, cfun, should_assert);
  boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(p, mapping, should_assert);
  boost::unordered_map<oop, oop>::iterator end = mm_dictionary_end(p, mapping, should_assert);
  word current_line = 0;
  for ( ; it != end; it++) {
    word b_offset = untag_small_int(it->first);
    word line = untag_small_int(it->second);
    // DBG(" SEARCH CURR LINE "
    //           << idx << " " << b_offset << " " << line << endl);
    if ((idx >= b_offset) && (current_line < line)) {
      // DBG("GOT LINE " << line << endl);
      current_line = line;
    }
  }
  return current_line;
}

bytecode* MMObj::mm_compiled_function_get_instruction_for_line(Process* p, oop cfun, number lineno, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfun == _core_image->get_prime("CompiledFunction")),
             "TypeError","Expected CompiledFunction")
  bytecode* base_ip = mm_compiled_function_get_code(p, cfun, should_assert);

  oop mapping = mm_compiled_function_get_line_mapping(p, cfun, should_assert);
  boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(p, mapping, should_assert);
  boost::unordered_map<oop, oop>::iterator end = mm_dictionary_end(p, mapping, should_assert);
  for ( ; it != end; it++) {
    word line = untag_small_int(it->second);
    DBG("line: " << line << "=" << lineno << endl);
    if (line == lineno) {
      word b_offset = untag_small_int(it->first);
      return b_offset + base_ip;
    }
  }
  return 0;
}


// bool MMObj::mm_compiled_function_loc_mapping_matches_ip(oop cfun, bytecode* ip, bool should_assert) {
//   if (!( *(oop*) cfun == _core_image->get_prime("CompiledFunction"))) {
//     p->raise("TypeError", "Expected CompiledFunction");
//   }

//   bytecode* base_ip = mm_compiled_function_get_code(cfun);
//   word idx = ip - base_ip;
//   // DBG("MMOBJ IDX : " << idx << endl);

//   oop mapping = mm_compiled_function_get_loc_mapping(cfun);
//   boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(mapping);
//   boost::unordered_map<oop, oop>::iterator end = mm_dictionary_end(mapping);
//   for ( ; it != end; it++) {
//     word b_offset = untag_small_int(it->first);
//     // DBG("LOC_MATCHES_IP? -- " << b_offset << " " <<  idx << std::endl);
//     if (idx == b_offset) {
//       return true;
//     }
//     if (b_offset > idx) {
//       break;
//     }
//   }
//   // DBG("LOC_MATCHES_IP? RET FALSE: " << idx << std::endl);
//   return false;
// }



oop MMObj::mm_compiled_class_name(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[2];
}

oop MMObj::mm_compiled_class_super_name(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[3];
}

oop MMObj::mm_compiled_class_compiled_module(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[4];
}

//
oop MMObj::mm_compiled_class_fields(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[5];
}

oop MMObj::mm_compiled_class_methods(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[6];
}

oop MMObj::mm_compiled_class_own_methods(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  return ((oop*)cclass)[7];
}

number MMObj::mm_compiled_class_num_fields(Process* p, oop cclass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cclass == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")
  oop fields_list = mm_compiled_class_fields(p, cclass, should_assert);
  return mm_list_size(p, fields_list, should_assert);
}

oop MMObj::mm_cfuns_to_funs_dict(Process* p, oop cfuns_dict, oop imod, bool should_assert) {
  TYPE_CHECK(!( *(oop*) cfuns_dict == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")

  // number size = mm_dictionary_size(cfuns_dict);
  oop funs_dict = mm_dictionary_new();

  boost::unordered_map<oop, oop>::iterator it = mm_dictionary_begin(p, cfuns_dict, should_assert);
  for ( ; it != mm_dictionary_end(p, cfuns_dict, should_assert); it++) {
    oop sym_name = it->first;
    oop cfun = it->second;
    oop fun = mm_function_from_cfunction(p, cfun, imod, should_assert);
    mm_dictionary_set(p, funs_dict, sym_name, fun, should_assert);
  }
  return funs_dict;
}

oop MMObj::mm_class_behavior_new(Process* p, oop super_class, oop funs_dict, bool should_assert) {
  TYPE_CHECK(!( *(oop*) funs_dict == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
  //vt: Behavior
  //delegate:
  //dict
  oop cbehavior = (oop) malloc(sizeof(word) * 4);
  * (oop*) cbehavior = _core_image->get_prime("Behavior");
  * (oop*) &cbehavior[1] = * (oop*) super_class; //super_class[vt]
  * (oop*) &cbehavior[2] = funs_dict;
  * (word*) &cbehavior[3] = (word) 256; //flag
  return cbehavior;
}


oop MMObj::mm_class_new(Process* p, oop class_behavior, oop super_class, oop dict, oop compiled_class, number payload, bool should_assert) {
  TYPE_CHECK(!( *(oop*) dict == _core_image->get_prime("Dictionary")),
             "TypeError","Expected Dictionary")
  TYPE_CHECK(!( *(oop*) compiled_class == _core_image->get_prime("CompiledClass")),
             "TypeError","Expected CompiledClass")

  oop klass = (oop) malloc(sizeof(word) * 5);
  * (oop*) klass = class_behavior;
  * (oop*) &klass[1] = super_class;
  * (oop*) &klass[2] = dict;
  * (word*) &klass[3] = payload;
  * (oop*) &klass[4] = compiled_class;
  return klass;
}

oop MMObj::mm_class_name(Process* p, oop klass, bool should_assert) {
  oop cclass = mm_class_get_compiled_class(p, klass, should_assert);
  return mm_compiled_class_name(p, cclass, should_assert);
}

oop MMObj::mm_class_dict(oop klass) {
  return ((oop*)klass)[2];
}

oop MMObj::mm_class_get_compiled_class(Process* p, oop klass, bool should_assert) {
  TYPE_CHECK(!( *(oop*) klass != _core_image->get_prime("Behavior")),
             "TypeError","Expected Behavior")
  return ((oop*)klass)[4];
}


oop MMObj::mm_new_slot_getter(Process* p, oop imodule, oop owner, oop name, int idx, bool should_assert) {
//    if (!( *(oop*) cclass == _core_image->get_prime("CompiledClass"))) {
//     p->raise("TypeError", "Expected CompiledClass");
//   }
  TYPE_CHECK(!( *(oop*) name == _core_image->get_prime("String")),
             "TypeError","Expected String")

  oop cfun_getter = (oop) calloc(sizeof(word), 17);

  * (oop*) cfun_getter = _core_image->get_prime("CompiledFunction");
  * (oop*) &cfun_getter[1] = mm_object_new();
  * (oop*) &cfun_getter[2] = name;
  * (oop*) &cfun_getter[3] = mm_list_new();
  * (word*) &cfun_getter[7] = 1;
  * (word*) &cfun_getter[8] = idx;
  * (oop*) &cfun_getter[9] = owner;

  return mm_function_from_cfunction(p, cfun_getter, imodule, should_assert);
}

oop MMObj::mm_symbol_new(const char* str) {
  oop symb = (oop) malloc((sizeof(word) * 3) + (strlen(str)+1));

  * (oop*) symb = _core_image->get_prime("Symbol");
  * (oop*) &symb[1] = mm_object_new();
  * (word*) &symb[2] = strlen(str);

  char* target = (char*) &symb[3];
  for (unsigned long i = 0; i <= strlen(str); i++) {
    target[i] = str[i];
  }
  return symb;
}


char* MMObj::mm_symbol_cstr(Process* p, oop sym, bool should_assert) {
  TYPE_CHECK(!( *(oop*) sym == _core_image->get_prime("Symbol")),
             "TypeError","Expected Symbol")
  //0: vt
  //1: delegate
  //2: size
  //3: <str> ...
  return (char*) &(sym[3]);
}

oop MMObj::mm_symbol_to_string(Process* p, oop sym, bool should_assert) {
  TYPE_CHECK(!( *(oop*) sym == _core_image->get_prime("Symbol")),
             "TypeError","Expected Symbol")
  return mm_string_new(mm_symbol_cstr(p, sym, should_assert));
}


oop MMObj::mm_behavior_get_dict(oop behavior) {
  //if (!( *(oop*) behavior == _core_image->get_prime("Behavior")); -- this can also be an imodul) {
//  p->raise("TypeError", "Expected Behavior");
// }
  return (oop) ((oop*)behavior)[2];
}

number MMObj::mm_behavior_size(Process* p, oop behavior, bool should_assert) {
  TYPE_CHECK(!( **(oop**) behavior == _core_image->get_prime("Behavior")),
             "TypeError","Expected Behavior")
  oop num = ((oop*)behavior)[3];
  if (is_small_int(num)) {
    DBG("behavior size is tagged and I will untag it" << endl);
    return untag_small_int(num);
  } else {
    return (number) num;
  }
}

bool MMObj::delegates_to(oop sub_type, oop super_type) {
  DBG("delegates_to? " << sub_type << " " << super_type << endl);
  if (sub_type == NULL) {
    return false;
  }
  if (sub_type == super_type) {
    return true;
  } else {
    return delegates_to(mm_object_delegate(sub_type), super_type);
  }
}

bool MMObj::delegates_or_is_subclass(Process* p, oop subclass, oop superclass) {
  if (subclass == NULL) {
    return false;
  } else if (delegates_to(subclass, superclass)) {
    return true;
  } else {
    oop super_cclass = mm_class_get_compiled_class(p, superclass);
    oop sub_cclaass = mm_class_get_compiled_class(p, subclass);
    if (super_cclass == sub_cclaass) {
      return true;
    } else {
      return delegates_or_is_subclass(p, mm_object_delegate(subclass), superclass);
    }
  }
}


oop MMObj::mm_new(oop vt, oop delegate, number payload) {
  oop obj = (oop) calloc(sizeof(word), (2 + payload)); // vt, delegate

  ((oop*) obj)[0] = vt;
  ((oop*) obj)[1] = delegate;
  return obj;
}

oop MMObj::alloc_instance(Process* p, oop klass) {
  if (klass == NULL) {
    DBG("alloc_instance: klass is null" << endl);
    return NULL;
  }

  DBG("alloc_instance for klass " << klass << endl);
  DBG("alloc_instance: class name: " << mm_string_cstr(p, mm_class_name(p, klass, true), true) << endl);

  number payload = mm_behavior_size(p, klass);
  assert(payload != INVALID_PAYLOAD);

  DBG("alloc_instance: payload " << payload << endl);

  oop instance = (oop) calloc(sizeof(word), payload + 2); //2: vt, delegate
  DBG("new instance [size: " << payload << "]: "
      << mm_string_cstr(p, mm_class_name(p, klass, true), true) << " = " << instance << endl);

  oop klass_parent =  mm_object_delegate(klass);

  ((oop*)instance)[0] = klass;
  ((oop*) instance)[1] = alloc_instance(p, klass_parent);

  DBG("Created recursive delegate " << instance << endl);
  return instance;
}


bool MMObj::mm_is_list(oop obj) {
  return *(oop*) obj == _core_image->get_prime("List");
}

bool MMObj::mm_is_dictionary(oop obj) {
  return *(oop*) obj == _core_image->get_prime("Dictionary");
}


std::list<std::string> MMObj::mm_sym_list_to_cstring_list(Process* p, oop lst, bool should_assert) {
  TYPE_CHECK(!( *(oop*) lst == _core_image->get_prime("List")),
             "TypeError","Expected List")

  std::list<std::string> res;
  for (int i = 0; i < mm_list_size(p, lst, should_assert); i++) {
    res.push_back(mm_symbol_cstr(p, mm_list_entry(p, lst, i, should_assert), should_assert));
  }
  return res;
}

void MMObj::mm_exception_set_message(Process* p, oop ex, oop msg, bool should_assert) {
  TYPE_CHECK(!( *(oop*) ex == _core_image->get_prime("Exception")),
             "TypeError","Expected Exception")

  ((oop*)ex)[2] = msg;
}

oop MMObj::mm_exception_get_message(Process* p, oop ex, bool should_assert) {
  TYPE_CHECK(!( *(oop*) ex == _core_image->get_prime("Exception")),
             "TypeError","Expected Exception")
    return ((oop*)ex)[2];
}

void MMObj::mm_exception_set_st(Process* p, oop ex, oop st, bool should_assert) {
  TYPE_CHECK(!( *(oop*) ex == _core_image->get_prime("Exception")),
             "TypeError","Expected Exception")

  ((oop*)ex)[3] = st;
}

// oop MMObj::mm_exception_get_bp(Process* p, oop ex, bool should_assert) {
//   TYPE_CHECK(!( *(oop*) ex == _core_image->get_prime("Exception")),
//              "TypeError","Expected Exception")
//     return ((oop*)ex)[3];
// }
