#ifndef MMOBJ_HPP
#define MMOBJ_HPP

#include "defs.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <vector>
#include <boost/unordered_map.hpp>
#include <list>
#include "core_image.hpp"

//#include <gc_cpp.h>
#include "gc/gc_allocator.h"

typedef std::vector<oop, gc_allocator<oop> > oop_vector;

typedef boost::unordered_map<oop, oop, boost::hash<oop>,
                             std::equal_to<oop>,
                             gc_allocator<std::pair<oop,oop> > >
                           oop_map;


class CoreImage;
class VM;

typedef struct {} InternalError;



#define CFUN_IS_TOP_LEVEL(header) ((header & 0x10) >> 4)
#define CFUN_HAS_VAR_ARG(header)  ((header & 0x8) >> 3)
#define CFUN_HAS_ENV(header)      ((header & 0x4) >> 2)
#define CFUN_IS_PRIM(header)      ((header & 0x2) >> 1)
#define CFUN_IS_CTOR(header)      (header & 0x1)

#define CFUN_ENV_OFFSET(header)   ((header & 0xf8000) >> 15)
#define CFUN_NUM_PARAMS(header)   ((header & 0x7c00) >> 10)
#define CFUN_STORAGE_SIZE(header) ((header & 0x3e0) >> 5)


class MMObj {
public:
  MMObj(CoreImage*);
  void init();

  oop mm_process_new(Process*, Process* proc);
  Process* mm_process_get_proc(Process*, oop, bool should_assert = false);
  oop mm_frame_new(Process*, oop);
  oop mm_frame_get_bp(Process*, oop, bool should_assert = false);
  oop mm_frame_get_cp(Process*, oop, bool should_assert = false);
  oop mm_frame_get_fp(Process*, oop, bool should_assert = false);
  oop mm_frame_get_rp(Process*, oop, bool should_assert = false);
  oop mm_frame_get_dp(Process*, oop, bool should_assert = false);
  number mm_frame_get_ss(Process*, oop, bool should_assert = false);
  bytecode* mm_frame_get_ip(Process*, oop, bool should_assert = false);

  oop mm_new(oop vt, oop delegate, number payload);
  oop alloc_instance(Process*, oop klass);

  oop mm_object_new();
  oop mm_object_vt(oop);
  oop mm_object_delegate(oop);
//   inline oop mm_object_delegate(oop obj) {
//   if (is_small_int(obj) || (obj == MM_TRUE) || (obj == MM_FALSE) || (obj == MM_NULL)) {
//     return obj; //mm_object_new(); //TODO: this should probably be a dummy static object
//   } else {
//     return ((oop*) obj)[1];
//   }
// }

  inline oop mm_behavior_get_dict(oop b) { return (oop) ((oop*)b)[2]; }

  number mm_behavior_size(Process*, oop, bool should_assert = false);

  bool delegates_to(oop, oop);
  bool delegates_or_is_subclass(Process* p, oop subclass, oop superclass);
  oop delegate_for_vt(Process* p, oop obj, oop vt);

  oop mm_module_new(int num_fields, oop cmod, oop delegate);

  oop mm_compiled_module_name(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_params(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_meta_vars(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_default_locations(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_imports(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_classes(Process*, oop, bool should_assert = false);
  oop mm_compiled_module_functions(Process*, oop, bool should_assert = false);

  oop mm_boolean_new(number val);
  bool mm_boolean_cbool(Process*, oop val, bool should_assert = false);

  oop mm_list_new();
  void mm_list_init(oop obj);

  number mm_list_size(Process*, oop list, bool should_assert = false);
  number mm_list_index_of(Process*, oop list, oop elem, bool should_assert = false);
  oop mm_list_entry(Process*, oop list, number idx, bool should_assert = false);
  oop_vector* mm_list_frame(Process*, oop, bool should_assert = false);
  void mm_list_prepend(Process*, oop list, oop element, bool should_assert = false);
  void mm_list_append(Process*, oop list, oop element, bool should_assert = false);
  void mm_list_set(Process*, oop list, number idx, oop element, bool should_assert = false);

  oop mm_dictionary_new();
  void mm_dictionary_init(oop obj);
  number mm_dictionary_size(Process*, oop dict, bool should_assert = false);
  // oop mm_dictionary_entry_key(Process*, oop dict, int idx, bool should_assert = false);
  // oop mm_dictionary_entry_value(Process*, oop dict, int idx, bool should_assert = false);
  void mm_dictionary_set(Process*, oop dict, oop key, oop value, bool should_assert = false);
  bool mm_dictionary_has_key(Process*, oop dict, oop key, bool should_assert = false);
  oop mm_dictionary_keys(Process*, oop dict, bool should_assert = false);
  oop mm_dictionary_values(Process*, oop dict, bool should_assert = false);

 oop mm_dictionary_get(Process*, oop dict, oop key, bool should_assert = false);
 oop mm_dictionary_fast_get(Process*, oop dict, oop key);

  oop_map* mm_dictionary_frame(Process*, oop, bool should_assert = false);
  oop_map::iterator mm_dictionary_begin(Process*, oop, bool should_assert = false);
  oop_map::iterator mm_dictionary_end(Process*, oop, bool should_assert = false);

  void mm_module_set_dictionary(Process*, oop imodule, oop imod_dict, bool should_assert = false);
  void mm_module_set_module_argument(oop imodule, oop arg, number idx);
  oop mm_module_entry(oop imodule, number idx);
  oop mm_module_dictionary(oop imodule);
  oop mm_module_get_cmod(oop imodule);
  oop mm_module_get_param(oop imodule, number idx);

  inline bool mm_is_string(oop obj) {
    if (is_small_int(obj) ||
        (obj == MM_TRUE)  ||
        (obj == MM_FALSE) ||
        (obj == MM_NULL)) {
      return false;
    } else {
      return *(oop*) obj == _cached_string;
    }
  }

  oop mm_string_new(const std::string& str);
  oop mm_string_new(const char*);
  number mm_string_size(Process* p, oop str, bool should_assert = false);
  std::string mm_string_stl_str(Process* p, oop str, bool should_assert = false);
  inline char* mm_string_cstr(Process* p, oop str, bool should_assert = false) {
    // TYPE_CHECK(!( mm_object_vt(str) == _core_image->get_prime("String")),
    //          "TypeError","Expected String")
    //0: vt
    //1: delegate
    //2: size
    //3: <str> ...
    return (char*) &(str[3]);
  }


  bool mm_is_list(oop);
  bool mm_is_dictionary(oop);

  oop mm_symbol_new(const char* str);

  inline bool mm_is_symbol(oop sym) {
    return *(oop*) sym == _cached_context;
  }

  char* mm_symbol_cstr(Process*, oop, bool should_assert = false);
  oop mm_symbol_to_string(Process*, oop, bool should_assert = false);

  inline bool mm_is_function(oop obj) {
    static oop fun_prime = _core_image->get_prime("Function");
    return *(oop*) obj == fun_prime;
  }

  inline bool mm_is_context(oop obj) {
    return *(oop*) obj == _cached_context;
  }

  oop mm_context_new(Process* p, oop cfun, oop imod, oop env, bool should_assert = false);
  void mm_context_set_cfun(Process*, oop, oop, bool should_assert = false);
  void mm_context_set_env(Process*, oop, oop, bool should_assert = false);
  void mm_context_set_module(Process*, oop, oop, bool should_assert = false);
  oop mm_context_get_env(Process*, oop, bool should_assert = false);

  oop mm_function_from_cfunction(Process*, oop cfun, oop imod, bool should_assert = false);


  long mm_function_get_header(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_name(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_prim_name(Process*, oop fun, bool should_assert = false);
  bool mm_function_is_getter(Process*, oop fun, bool should_assert = false);
  number mm_function_access_field(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_owner(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_literal_by_index(Process*, oop fun, int idx, bool should_assert = false);
  number mm_function_get_code_size(Process*, oop fun, bool should_assert = false);
  bytecode* mm_function_get_code(Process*, oop fun, bool should_assert = false);
  number mm_function_exception_frames_count(Process*, oop fun, bool should_assert = false);
  oop mm_function_exception_frames(Process*, oop fun, bool should_assert = false);
  oop mm_function_env_table(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_text(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_line_mapping(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_loc_mapping(Process*, oop fun, bool should_assert = false);
  oop mm_function_get_closures(Process*, oop fun, bool should_assert = false);
  bytecode* mm_function_next_expr(Process*, oop fun, bytecode* ip, bool should_assert = false);
  bytecode* mm_function_next_line_expr(Process*, oop fun, bytecode* ip, bool should_assert = false);
  number mm_function_get_line_for_instruction(Process*, oop fun, bytecode* ip, bool should_assert = false);

  oop mm_function_get_module(Process*, oop fun, bool should_assert = false);

  inline oop mm_function_get_cfun(Process* p, oop fun, bool should_assert = false) {
    return (oop) ((oop*)fun)[2];
  }

  void mm_overwrite_compiled_function(Process*, oop target_cfun, oop origin_cfun, bool should_assert = false);

  //
  long mm_compiled_function_get_header(Process*, oop fun, bool should_assert = false);
  oop mm_compiled_function_get_name(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_params(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_prim_name(Process*, oop cfun, bool should_assert = false);
  bool mm_compiled_function_is_getter(Process*, oop cfun, bool should_assert = false);
  number mm_compiled_function_access_field(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_owner(Process*, oop cfun, bool should_assert = false);
  void mm_compiled_function_set_owner(Process*, oop cfun, oop owner, bool should_assert = false);
  oop mm_compiled_function_outer_cfun(Process*, oop cfun, bool should_assert = false);
  number mm_compiled_function_get_literal_frame_size(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_literal_by_index(Process*, oop cfun, int idx, bool should_assert = false);
  number mm_compiled_function_get_code_size(Process*, oop cfun, bool should_assert = false);
  bytecode* mm_compiled_function_get_code(Process*, oop cfun, bool should_assert = false);
  number mm_compiled_function_exception_frames_count(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_exception_frames(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_env_table(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_text(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_line_mapping(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_loc_mapping(Process*, oop cfun, bool should_assert = false);
  oop mm_compiled_function_get_closures(Process*, oop cfun, bool should_assert = false);

  number mm_compiled_function_get_line_for_instruction(Process*, oop fun, bytecode* ip, bool should_assert = false);
  bytecode* mm_compiled_function_get_instruction_for_line(Process* p, oop cfun, number lineno, bool should_assert = false);
  bytecode* mm_compiled_function_next_expr(Process*, oop cfun, bytecode* ip, bool should_assert = false);
  bytecode* mm_compiled_function_next_line_expr(Process*, oop cfun, bytecode* ip, bool should_assert = false);

  // oop mm_compiled_function_get_cmod(Process*, oop cfun, bool should_assert = false);
  // void mm_compiled_function_set_cmod(Process*, oop cfun, oop cmod, bool should_assert = false);

  // bool mm_compiled_function_loc_mapping_matches_ip(Process*, oop, bytecode*, bool should_assert = false);

  oop mm_compiled_class_name(Process*, oop cclass, bool should_assert = false);
  oop mm_compiled_class_super_name(Process*, oop cclass, bool should_assert = false);
  oop mm_compiled_class_compiled_module(Process* p, oop cclass, bool should_assert);
  oop mm_compiled_class_own_methods(Process*, oop cclass, bool should_assert = false);
  oop mm_compiled_class_methods(Process*, oop cclass, bool should_assert = false);
  oop mm_compiled_class_fields(Process*, oop cclass, bool should_assert = false);
  number mm_compiled_class_num_fields(Process*, oop cclass, bool should_assert = false);



  oop mm_class_behavior_new(Process*, oop super_class, oop funs_dict, bool should_assert = false);
  oop mm_class_new(Process*, oop class_behavior, oop super_class, oop dict, oop compiled_class, number payload, bool should_assert = false);
  oop mm_class_name(Process*, oop klass, bool should_assert = false);
  oop mm_class_get_compiled_class(Process*, oop klass, bool should_assert = false);
  oop mm_class_dict(oop);

  oop mm_cfuns_to_funs_dict(Process*, oop cfuns_dict, oop imod, bool should_assert = false);
  oop mm_new_slot_getter(Process*, oop imodule, oop owner, oop name, int idx, bool should_assert = false);

  std::list<std::string> mm_sym_list_to_cstring_list(Process*, oop, bool should_assert = false);

  void mm_exception_set_message(Process* proc, oop ex, oop msg, bool should_assert = false);
  oop mm_exception_get_message(Process* proc, oop ex, bool should_assert = false);
  void mm_exception_set_st(Process* proc, oop ex, oop st, bool should_assert = false);
  // oop mm_exception_get_bp(Process* proc, oop ex, bool should_assert = false);

  oop mm_integer_or_longnum_new(Process* proc, number n, bool should_assert = false);

  oop mm_longnum_new(Process* proc, number n, bool should_assert = false);
  number mm_longnum_get(Process* proc, oop n, bool should_assert = false);

  oop mm_float_new(Process* proc, float n, bool should_assert = false);
  float mm_float_get(Process* proc, oop n, bool should_assert = false);

  oop mm_cclass_or_cmodule_name(Process* p, oop obj, bool should_assert = false);

  CoreImage* core() { return _core_image; };

  void** mm_function_get_thread_code(Process*, oop fun, bool should_assert = false);
  void mm_function_set_thread_code(Process*, oop fun, void*, bool should_assert = false);
  void** mm_compiled_function_get_thread_code(Process*, oop cfun, bool should_assert = false);
  void mm_compiled_function_set_thread_code(Process*, oop cfun, void*, bool should_assert = false);

private:
  MMLog _log;
  // VM* _vm;
  CoreImage* _core_image;
  oop _cached_context;
  oop _cached_symbol;
  oop _cached_string;
};

#endif
