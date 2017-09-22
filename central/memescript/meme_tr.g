meme central:memescript/compiler
requires ometa_base
where
  ometa_base = central:memescript/ometa_base
import OMetaBase from ometa_base

class MemeScriptTranslator < OMetaBase
fields: proc;
init new: fun(proc, input) {
  super.new(input);
  @proc = proc;
}
<ometa>
start = [:module
         _                             //ignore compiler node
         !{@proc.new_module()}:modobj
         meta_section(modobj)
         requirements_section(modobj)
         code_section(modobj)];

meta_section :modobj = [:meta [meta_var(modobj)*]];

meta_var :modobj = [_:key _:val] => modobj.add_meta(key, val);

requirements_section :modobj = [:requirements module_params:params !{modobj.set_params(params)}
                                [:default-locations [default_location(modobj)*]]
                                [:imports [module_import(modobj)*]]];

module_params = [_*:xs] => xs;

default_location :modobj = [_:mod _:path] => modobj.add_default_location(mod, path);

module_import :modobj = [_:name _:from] => modobj.add_import(name, from);

code_section :modobj = [:code ~~[load_top_level_name(modobj)*] [definition(modobj)*]];

load_top_level_name :modobj = [:class [_:name _] _*] => modobj.add_top_level_name(name)
                            | [:object _:name _*] => modobj.add_top_level_name(name)
                            | [:fun _:name _*] => modobj.add_top_level_name(name)
                            ;

definition :modobj =  function_definition(modobj)
                   |  class_definition(modobj)
                   |  object_definition(modobj)
                   ;

function_definition :modobj =  !{this.input.head()}:ast
                               [:fun _:name  !{modobj.new_function(name)}:fnobj fparams(fobj):p !{fnobj.set_params(p)}
                                     !{fnobj.set_line(ast)}
                                     _:uses_env !{fnobj.uses_env(uses_env)}
                                     !{fnobj.body_processor}:bproc
                                     [:body body(bproc)]] !{fnobj.set_text(ast.text)};


class_definition :modobj =  [:class [_:name _:parent]
                              [:fields _:fields_list]
                                !{modobj.new_class(name, parent, fields_list)}:klass
                              constructors(klass)
                              [instance_method(klass)*]
                              [class_method(klass)*]];

constructor :klass =  !{this.input.head()}:ast
                        [:ctor _:name !{klass.new_ctor(name)}:fnobj fparams(fnobj):p !{fnobj.set_params(p)}
                             !{fnobj.set_line(ast)}
                             _:uses_env !{fnobj.uses_env(uses_env)}
                             !{fnobj.body_processor}:bproc
                             [:body body(bproc)]]  !{fnobj.set_text(ast.text)};

constructors :klass = [:ctors [constructor(klass)*]]
                     | [:ctors []];

instance_method :klass = !{this.input.head()}:ast
                           [:fun _:name !{klass.new_instance_method(name)}:fnobj fparams(fnobj):p !{fnobj.set_params(p)}
                                  !{fnobj.set_line(ast)}
                                  _:uses_env !{fnobj.uses_env(uses_env)}
                                  !{fnobj.body_processor}:bproc
                                  [:body body(bproc)]]  !{fnobj.set_text(ast.text)};

class_method :klass = !{this.input.head()}:ast
                         [:fun _:name  !{klass.new_class_method(name)}:fnobj fparams(fnobj):p !{fnobj.set_params(p)}
                          !{fnobj.set_line(ast)}
                          _:uses_env !{fnobj.uses_env(uses_env)}
                          !{fnobj.body_processor}:bproc
                          [:body body(bproc)]]  !{fnobj.set_text(ast.text)};

fparams :obj = [:params [fparam(obj)*:x]] => x;

fparam :obj  = [:var-arg !{obj.set_vararg()}] => "*"
               | _
               ;

object_definition :modobj = [:object _:name !{modobj.new_object(name)}:obj
                            [obj_slot(obj)+] [obj_function(obj)*]];

obj_slot :obj = [:slot obj_slot_value(obj)];

obj_slot_value :obj  =  _:name [:literal-number _:x]       => obj.add_slot_literal_num(name,x)
                     |  _:name [:literal-string _:x]       => obj.add_slot_literal_string(name,x)
                     |  _:name [:literal :null]            => obj.add_slot_literal_null(name)
                     |  _:name [:literal-array [anything*]:x] => obj.add_slot_literal_array(name, x)
                     |  _:name [:literal-dict anything*:x]  => obj.add_slot_literal_dict(name, x)
                     |  _:name _:x                          => obj.add_slot_ref(name, x)
                     ;

obj_function :obj = constructor(obj)
                  | function_definition(obj);


body :fnobj = [expr(fnobj)*]
            | [[:primitive [:literal-string _:name]] anything*]   => fnobj.set_primitive(name)
            ;

exprs :fnobj = [expr(fnobj)*];


expr_elif :fnobj = [:elif expr(fnobj) !{fnobj.emit_jz(null)}:lb_next [expr(fnobj)* !{fnobj.emit_jmp(null)}:lb_jmp]] !{lb_next.as_current()} => lb_jmp;


stm :fnobj :ast = :var-def _:id expr(fnobj) =>  fnobj.emit_var_decl(ast, id)
               | :end-body => fnobj.emit_end_body(ast)
               | :return expr(fnobj)       =>  fnobj.emit_return_top(ast)
               | :return-null => fnobj.emit_return_null(ast)
               | :return-top => fnobj.emit_return_top(ast)
               | :non-local-return expr(fnobj) =>  fnobj.emit_non_local_return(ast)
               | :super-ctor-send _:s args(fnobj):arity => fnobj.emit_super_ctor_send(ast, s, arity)
               | :send-or-local-call _:name args(fnobj):arity => fnobj.emit_send_or_local_call(ast, name, arity)
               | :super-send args(fnobj):arity => fnobj.emit_super_send(ast, arity)
               | :send _:e `send` [:args [[:id _]:v [:literal-array [expr(fnobj)*:arity]]]] expr(fnobj, e) expr(fnobj, v) => fnobj.emit_opt_send(ast, arity.size)
               | :send _:e _:s args(fnobj):arity expr(fnobj, e) => fnobj.emit_send(ast, s, arity)
               | :call _:e args(fnobj):arity expr(fnobj, e) => fnobj.emit_call(ast, arity)
               | :expression expr(fnobj) => fnobj.emit_pop(ast)
               | :not expr(fnobj) => fnobj.emit_unary(ast, "!")
               | :negative expr(fnobj) => fnobj.emit_unary(ast, "neg")
               | :get-var-arg expr(fnobj) => fnobj.emit_push_var_arg(ast)
               | :bit-neg expr(fnobj) => fnobj.emit_unary(ast, "~")
               | :and _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "and")
               | :or _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "or")
               | :+  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "+")
               | :-  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "-")
               | :*  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "*")
               | :/  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "/")
               | :&  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "&")
               | :|  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "|")
               | :<  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "<")
               | :<<  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "<<")
               | :>>  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, ">>")
               | :<= _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "<=")
               | :>  _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, ">")
               | :>= _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, ">=")
               | :== _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "==")
               | :!= _:e expr(fnobj) expr(fnobj, e) => fnobj.emit_binary(ast, "!=")
               | :if expr(fnobj) !{fnobj.emit_jz(null)}:lb_next [expr(fnobj)* !{fnobj.emit_jmp(null)}:lb_end] !{lb_next.as_current()} [expr_elif(fnobj)*:lbs_end] [expr(fnobj)*] !{lb_end.as_current()} !{lbs_end.map(fun(x) { x.as_current()})}

               | :while !{fnobj.current_label(false)}:lbcond
                   expr(fnobj)
                   !{fnobj.emit_jz(null)}:lbend [expr(fnobj)*] !{fnobj.emit_jmp_back(lbcond.as_current())} => lbend.as_current()

               | :for expr(fnobj) !{fnobj.current_label(false)}:lbcond expr(fnobj)
                   !{fnobj.emit_jz(null)}:lbend [expr(fnobj)*] expr(fnobj) !{fnobj.emit_jmp_back(lbcond.as_current())} => lbend.as_current()

               | :try
                  !{fnobj.current_label(true)}:label_begin_try
                    [expr(fnobj)*]
                  !{fnobj.emit_catch_jump()}:end_pos
                  !{fnobj.current_label(true)}:label_begin_catch
                    catch_decl:cp
                  !{fnobj.bind_catch_var(cp[1])}
                    [expr(fnobj)*] => fnobj.emit_try_catch(label_begin_try, label_begin_catch, end_pos, cp[0])
               | := [:id _:v] expr(fnobj)    => fnobj.emit_local_assignment(ast, v)
               | := [:index _:lhs :idx] expr(fnobj) expr(fnobj, idx) expr(fnobj, lhs)  => fnobj.emit_index_assignment(ast)
               | := [:field _:f] expr(fnobj) => fnobj.emit_field_assignment(ast, f)
               | :literal-number _:x => fnobj.emit_push_num_literal(ast, x)
               | :literal :this => fnobj.emit_push_this(ast)
               | :literal-string _:x => fnobj.emit_push_str_literal(ast, x)
               | :literal-symbol _:x => fnobj.emit_push_sym_literal(ast, x)
               | :literal :null       => fnobj.emit_push_null(ast)
               | :literal :true       => fnobj.emit_push_true(ast)
               | :literal :false      => fnobj.emit_push_false(ast)
               | :literal :module     => fnobj.emit_push_module(ast)
               | :literal :context    => fnobj.emit_push_context(ast)
               | :literal :argc       => fnobj.emit_push_argc(ast)
               | :id _:name             => fnobj.emit_push_var(ast, name)
               | :field _:name          => fnobj.emit_push_field(ast, name)
               | :literal-array  _:e exprs(fnobj, e)      => fnobj.emit_push_list(ast, e.size)
               | :literal-dict   dict_pairs(fnobj):p   => fnobj.emit_push_dict(ast, p.size)
               | :index _:e expr(fnobj) expr(fnobj, e)    => fnobj.emit_push_index(ast)
    ;

expr :fnobj = !{this.input.head()}:ast [stm(fnobj, ast)]
            | funliteral(fnobj)
            ;


catch_decl = [:catch [:id _:type] _:id]  => [type, id]
           | [:catch _:id]               => [null, id]
           ;

dict_pairs :fnobj = {[:pair expr(fnobj) expr(fnobj)]}*:e => e;


funliteral :fnobj = !{this.input.head()}:ast [:fun-literal !{fnobj.new_closure(p)}:fn fparams(fn):p !{fn.set_params(p)}
                       !{fn.set_line(ast)}
                       [:body [expr(fn)*]]]:ast
                     !{fn.set_text(ast.text)}
                      => fnobj.emit_push_closure(ast, fn);

cfunliteral :fnobj =  !{this.input.head()}:ast
                      !{fnobj.set_line(ast)}
                      [expr(fnobj)*] => fnobj;

args :fnobj =  [:args []] => 0
            |  [:args _:a !{a.reverse}:b arglist(fnobj, b):arity] => arity
            ;

arglist :fnobj = [expr(fnobj)+]:x => x.size;

</ometa>

end //MemeScriptTranslator
