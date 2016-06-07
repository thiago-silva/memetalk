.license
.endlicense

.preamble()

.code

  object Behavior
    _vt: Behavior; //self-referencing
    _delegate: null;
    dict: {};
//    size: 0;
  end

  object Object_CompiledClass
    _vt: CompiledClass; //declared below
    _delegate: null;
    name: "Object";
    super_class_name: "";
    compiled_module: CoreCompiledModule;
    fields: [];
    methods: {};
    own_methods: {};
  end

  object Object_Behavior
    _vt: Behavior;
    _delegate: null;
    size: 0;
    functions {
      init new: fun() {
        return this;
      }
       // id: fun() {
       //   <primitive "id">
       // }
       toString: fun() {
         <primitive "behavior_to_string">
       }
       toSource: fun() {
         <primitive "behavior_to_source">
       }
       ==: fun(other) {
         <primitive "equal">
       }
       !=: fun(other) {
         return !(this == other);
       }
       fullName: fun() {
         return "core:Behavior";
       }
    }
  end

  object Object
    _vt: Object_Behavior;
    _delegate: null;
    size: 0;
    compiled_class: Object_CompiledClass;
    functions {
       // id: fun() {
       //   <primitive "id">
       // }
       toString: fun() {
         <primitive "object_to_string">
       }
       toSource: fun() {
         <primitive "object_to_source">
       }
       !: fun() {
         <primitive "object_not">
       }
       ==: fun(other) {
         <primitive "equal">
       }
       !=: fun(other) {
         return !(this == other);
       }
       send: fun(name, args) {
         <primitive "object_send">
       }
       super_send: fun(name, args) {
         <primitive "object_super_send">
       }
       id: fun() {
         <primitive "object_id">
       }
       fullName: fun() {
         return "core:Object";
       }
       or: fun(other) {
         if (this) {
           return this;
         } elif (other) {
           return other;
         } else {
           return false;
         }
      }
    }
  end

  class CompiledClass
    fields: name, super_class_name, compiled_module,
            fields, methods, own_methods;
    instance_method name: fun() {
      return @name;
    }
    instance_method super_class_name: fun() {
      return @super_class_name;
    }
    instance_method compiled_module: fun() {
      return @compiled_module;
    }
    instance_method fields: fun() {
      return @fields;
    }
    instance_method methods: fun() {
      return @methods;
    }
    instance_method own_methods: fun() {
      return @own_methods;
    }
    instance_method fullName: fun() {
      if (@compiled_module) {
        return @compiled_module.fullName() + ":" + @name;
      } else {
        //TODO: actually get thisModule.compiledModile or something
        return "core:" + @name;
      }
    }
  end

  class CompiledFunction
   fields: name, params, is_ctor, is_prim, prim_name, accessor_flags,
           accessor_field, owner, len_params, has_env, is_top_level,
           outer_cfun, storage_size, env_offset, lit_frame_size,
           literal_frame_addr, bytecode_size, bytecode_addr,
           exception_frames_len, exception_frame_addr,
           env_table, text, line_mappings,
           loc_mappings, closures;
    init withEnv: fun(text, scope_names, cmod) {
      <primitive "compiled_function_with_env">
    }
    init withFrame: fun(text, frame, cmod) {
      <primitive "compiled_function_with_frame">
    }
    instance_method new_context: fun(fp, module) {
      <primitive "compiled_function_new_context">
      //return Context.new(this, fp, module);
    }
    instance_method asContextWithVars: fun(imod, vars_dict) {
      <primitive "compiled_function_as_context_with_vars">
    }
    instance_method text: fun() {
      <primitive "compiled_function_get_text">
    }
    // instance_method line_mapping: fun() {
    //   <primitive "compiled_function_get_line_mapping">
    // }
    // instance_method loc_mapping: fun() {
    //   <primitive "compiled_function_get_loc_mapping">
    // }
    instance_method source_location_for: fun(frame) {
      <primitive "compiled_function_loc_for">
    }
    instance_method lineFor: fun(frame) {
      if (@is_prim) {
        return 0;
      } else {
        return this.source_location_for(frame)[0];
      }
    }
    instance_method name: fun() {
      return @name;
    }
    instance_method owner: fun() {
      return @owner;
    }
    instance_method fullName: fun() {
      return @owner.fullName + "/" + @name;
    }
    instance_method isTopLevel: fun() {
      return @is_top_level;
    }
    instance_method outerCompiledFunction: fun() {
      return @outer_cfun;
    }
    instance_method env_table: fun() {
      return @env_table;
    }
    instance_method bytecode_addr: fun() {
      return @bytecode_addr;
    }
    instance_method line_mappings: fun() {
      return @line_mappings;
    }
    instance_method closures: fun() {
      return @closures;
    }
    instance_method recompile: fun(text) {
      <primitive "compiled_function_recompile">
    }
  end

  class CompiledModule
   fields: name, license, params, default_params, aliases, functions, classes,
           parent_module;
  instance_method fullName: fun() {
    return @name;
  }
  instance_method functions: fun() {
    return @functions;
  }
  instance_method classes: fun() {
    return @classes;
  }
  end

  class Null
    instance_method toString: fun() {
      return "null";
    }
    instance_method toSource: fun() {
      return "null";
    }
  end

  class Boolean
  instance_method toString: fun() {
    if (this == true) {
      return "true";
    } else {
      return "false";
    }
  }
  instance_method and: fun(other) {
    if (this) {
      if (other) {
         return true;
      }
    }
    return false;
  }
  instance_method or: fun(other) {
    if (this) {
       return this;
    } elif (other) {
       return other;
    }
    return false;
  }
  end

  class String
  instance_method toInteger: fun() {
    <primitive "string_to_integer">
  }
  instance_method toString: fun() {
    return this;
  }
  instance_method toSource: fun() {
    return "\"" + this.replace_all("\\", "\\\\").replace_all("\"", "\\\"").replace_all("\n", "\\n") + "\"";
  }
  instance_method toSymbol: fun() {
    <primitive "string_to_symbol">
  }
  instance_method +: fun(other) {
    <primitive "string_append">
  }
  instance_method count: fun(other) {
    <primitive "string_count">
  }
  instance_method size: fun() {
    <primitive "string_size">
  }
  instance_method ==: fun(other) {
    <primitive "string_equal">
  }
  instance_method find: fun(arg) {
    <primitive "string_find">
  }
  instance_method each: fun(fn) {
    <primitive "string_each">
  }
  instance_method index: fun(arg) {
    <primitive "string_index">
  }
  instance_method rindex: fun(arg) {
    <primitive "string_rindex">
  }
  instance_method from: fun(idx) {
    <primitive "string_from">
  }
  instance_method substr: fun(from, max) {
    <primitive "string_substr">
  }
  instance_method b64encode: fun() {
    <primitive "string_b64encode">
  }
  instance_method b64decode: fun() {
    <primitive "string_b64decode">
  }
  instance_method replace_all: fun(what, value) {
    <primitive "string_replace_all">
  }
  instance_method onlySpaces: fun() {
    <primitive "string_only_spaces">
  }
  instance_method onlyDigits: fun() {
    <primitive "string_only_digits">
  }
  instance_method isLower: fun() {
    <primitive "string_is_lower">
  }
  instance_method isUpper: fun() {
    <primitive "string_is_upper">
  }
  instance_method split: fun(sep) {
    <primitive "string_split">
  }
  instance_method trim: fun() {
    <primitive "string_trim">
  }
  end

  class Symbol
  instance_method toString: fun() {
    <primitive "symbol_to_string">
  }
  instance_method toSource: fun() {
    return ":" + this.toString;
  }
  end

  class Dictionary
  instance_method toString: fun() {
    <primitive "dictionary_to_string">
  }
  instance_method toSource: fun() {
    <primitive "dictionary_to_source">
  }
  instance_method set: fun(key, value) {
    <primitive "dictionary_set">
  }
  instance_method index: fun(idx) {
    <primitive "dictionary_index">
  }
  instance_method +: fun(other) {
    <primitive "dictionary_plus">
  }
  instance_method has: fun(key) {
    <primitive "dictionary_has">
  }
  instance_method keys: fun() {
    <primitive "dictionary_keys">
  }
  instance_method values: fun() {
    <primitive "dictionary_values">
  }
  instance_method size: fun() {
    <primitive "dictionary_size">
  }
  class_method new: fun() {
    <primitive "dictionary_new">
  }
  end

  class List
  instance_method append: fun(x) {
    <primitive "list_append">
  }
  instance_method prepend: fun(x) {
    <primitive "list_prepend">
  }
  instance_method index: fun(idx) {
    <primitive "list_index">
  }
  instance_method each: fun(fn) {
    <primitive "list_each">
  }
  instance_method map: fun(fn) {
    <primitive "list_map">
  }
  instance_method filter: fun(fn) {
    <primitive "list_filter">
  }
  instance_method detect: fun(fn) {
    <primitive "list_detect">
  }
  instance_method has: fun(value) {
    <primitive "list_has">
  }
  instance_method last: fun() {
    <primitive "list_last">
  }
  instance_method from: fun(idx) {
    <primitive "list_from">
  }
  instance_method toString: fun() {
    <primitive "list_to_string">
  }
  instance_method toSource: fun() {
    <primitive "list_to_source">
  }
  instance_method size: fun() {
    <primitive "list_size">
  }
  instance_method reverse: fun() {
    <primitive "list_reverse">
  }
  instance_method join: fun(sep) {
    <primitive "list_join">
  }
  instance_method +: fun(other) {
    <primitive "list_plus">
  }
  class_method new: fun() {
    <primitive "list_new">
  }
  end

  class Number
    instance_method +: fun(arg) {
      <primitive "number_sum">
    }
    instance_method -: fun(arg) {
      <primitive "number_sub">
    }
    instance_method *: fun(arg) {
      <primitive "number_mul">
    }
    instance_method <: fun(arg) {
      <primitive "number_lt">
    }
    instance_method >: fun(arg) {
      <primitive "number_gt">
    }
    instance_method >=: fun(arg) {
      <primitive "number_gteq">
    }
    instance_method toString: fun() {
      <primitive "number_to_string">
    }
    instance_method toSource: fun() {
      <primitive "number_to_source">
    }
  end


  class Function
    fields: compiled_function, module;
    instance_method compiledFunction: fun() {
      return @compiled_function;
    }
    instance_method getEnv: fun() {
      <primitive "function_get_env">
    }
  end

  class Context
  fields: compiled_function, module, env;
  init new: fun(cfun, env, module) {
    <primitive "context_new">
    // @compiled_function = cfun;
    // @module = module;
    // @env = env;
  }
  instance_method compiledFunction: fun() {
    return @compiled_function;
  }
  instance_method getEnv: fun() {
    <primitive "context_get_env">
  }
  class_method withVars: fun(code, vars, imod) {
    var cmod = get_compiled_module(imod);
    var cfn = CompiledFunction.withEnv(code, vars.keys(), cmod);
    return cfn.asContextWithVars(imod, vars);
  }
  class_method withFrame: fun(code, frame, imod) {
    <primitive "context_with_frame">
    // var cmod = get_compiled_module(imod);
    // var cfn = CompiledFunction.withFrame(code, frame, cmod);
    // return cfn.new_context(frame.fp, imod);
  }
  end

class Exception
  fields: message, stack_trace;
  init new: fun(message) {
    <primitive "exception_constructor">
  }
  instance_method message: fun() {
    return @message;
  }
  instance_method stack_trace: fun() {
    return @stack_trace;
  }
  instance_method throw: fun() {
    <primitive "exception_throw">
  }
  instance_method message: fun() {
    return @message;
  }
  instance_method toString: fun() {
    return "Exception: " + this.message.toString();
  }
  class_method throw: fun(msg) {
    this.new(msg).throw;
  }
end

class NonLocalReturn
  fields: value;
  init new: fun(value) {
    @value = value;
  }
  instance_method throw: fun() {
    <primitive "exception_throw">
  }
  instance_method value: fun() {
    return @value;
  }
  class_method throw: fun(value) {
    this.new(value).throw;
  }
end

class ImportError < Exception
  instance_method toString: fun() {
    return "ImportError: " + this.message.toString();
  }
end

class TypeError < Exception
  instance_method toString: fun() {
    return "TypeError: " + this.message.toString();
  }
end

class IndexError < Exception
  instance_method toString: fun() {
    return "IndexError: " + this.message.toString();
  }
end

class KeyError < Exception
  instance_method toString: fun() {
    return "KeyError: " + this.message.toString();
  }
end

class InternalError < Exception
  instance_method toString: fun() {
    return "InternalError: " + this.message.toString();
  }
end


class DoesNotUnderstand < Exception
  instance_method toString: fun() {
    return "DoesNotUnderstand: " + this.message.toString();
  }
end

class ArityError < Exception
  instance_method toString: fun() {
    return "ArityError: " + this.message.toString();
  }
end

class CompileError < Exception
  instance_method toString: fun() {
    return "CompileError: " + this.message.toString();
  }
end

  class Mirror
    fields: mirrored;
    init new: fun(mirrored) {
      @mirrored = mirrored;
    }
    //instance_method fields: fun() { //strings only
    //  <primitive "mirror_fields">
    //}
    instance_method entries: fun() { //can be a list of anything
      <primitive "mirror_entries">
    }
    instance_method valueFor: fun(entry) { //requirement: entry \in self.entries()
      <primitive "mirror_value_for">
    }
    instance_method setValueFor: fun(entry, value) { //requirement: entry \in self.entries()
      <primitive "mirror_set_value_for">
    }
    class_method vtFor: fun(obj) {
      <primitive "mirror_vt_for">
    }
  end


// temps
  class Process
    fields: self;
  instance_method stepInto: fun() {
    <primitive "process_step_into">
  }
  instance_method stepOver: fun() {
    <primitive "process_step_over">
  }
  instance_method stepOverLine: fun() {
    <primitive "process_step_over_line">
  }
  instance_method stepOut: fun() {
    <primitive "process_step_out">
  }
  instance_method resume: fun() {
    <primitive "process_resume">
  }
  instance_method reloadFrame: fun() {
    <primitive "process_reload_frame">
  }
  instance_method returnFromFrame: fun(val) {
    <primitive "process_return_from_frame">
  }
  instance_method breakAtAddr: fun(fn) {
    <primitive "process_break_at_addr">
  }
  instance_method rewindAndContinue: fun(frame) {
    <primitive "process_rewind_and_continue">
  }
  instance_method runUntil: fun(cfun, line) {
    <primitive "process_run_until">
  }
  instance_method add_breakpoint: fun(cfun, line) {
    <primitive "process_add_breakpoint">
  }
  instance_method currentException: fun() {
    <primitive "process_current_exception">
  }
  instance_method toggle_module_break_mode: fun() {
    <primitive "process_toggle_module_break_mode">
  }
  instance_method fp: fun() {
     <primitive "process_fp">
  }
  instance_method cp: fun() {
     <primitive "process_cp">
  }
  instance_method mp: fun() {
     <primitive "process_mp">
  }
  // instance_method ip: fun() {
  //   <primitive "process_ip">
  // }
  instance_method frames: fun() {
    <primitive "process_frames">
  }
  // instance_method apply: fun(fn) {
  //   <primitive "process_apply">
  // }
  // instance_method evalInFrame: fun(text, frame_idx) {
  //   <primitive "process_eval_in_frame">
  // }
  end

  class Frame
  fields: self;
  instance_method cp: fun() {
    <primitive "frame_cp">
  }
  instance_method ip: fun() {
    <primitive "frame_ip">
  }
  instance_method fp: fun() {
    <primitive "frame_fp">
  }
  instance_method rp: fun() {
    <primitive "frame_rp">
  }
  instance_method dp: fun() {
    <primitive "frame_dp">
  }
  instance_method get_local_value: fun(idx) {
    <primitive "frame_get_local_value">
  }
  end

  get_current_process: fun() {
    <primitive "get_current_process">
  }

  get_current_frame: fun() {
    <primitive "get_current_frame">
  }

  get_compiled_module: fun(module) {
    <primitive "get_compiled_module">
  }

  get_compiled_module_by_name: fun(name) {
    <primitive "get_compiled_module_by_name">
  }

  get_module: fun(name) {
    <primitive "get_module">
  }

  modules_path: fun() {
     <primitive "modules_path">
  }

  debug: fun() {
     <primitive "test_debug">
  }

  compile_module: fun(mm_name) {
     <primitive "remote_repl_compile_module">
  }
  instantiate_module: fun(mm_name) {
     <primitive "remote_repl_instantiate_module">
  }
.end
