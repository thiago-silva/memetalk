start = ['module' :license :meta
         !(self.i.new_module()):modobj
         preamble(modobj) code_sec(modobj)]

preamble :modobj = ['.preamble'
                    :params
                    !(modobj.set_params(params))
                    [module_default_param(modobj)*] [module_alias(modobj)*]]


module_default_param :modobj = ['param' :lhs ['library' :ns :name]]
                               -> modobj.add_default_param(lhs, ns, name)

module_alias :modobj = ['alias' :libname :alias] -> modobj.module_alias(libname, alias)


code_sec :modobj = ['.code' ~~[load_top_level_name(modobj)*] [definition(modobj)*]]

load_top_level_name :modobj = ['class' [:name :ignore] (:ignore)*] -> modobj.add_top_level_name(name)
                            | ['object' :name (:ignore)*] -> modobj.add_top_level_name(name)
                            | ['fun' :name (:ignore)*] -> modobj.add_top_level_name(name)

definition :modobj =  function_definition(modobj)
                   |  class_definition(modobj)
                   |  object_definition(modobj)

function_definition :modobj =  !(self.input.head()[0]):ast
                               ['fun' :name params:p  !(modobj.new_function(name, p)):fnobj
                                     !(fnobj.set_line(ast))
                                     :uses_env !(fnobj.uses_env(uses_env))
                                     ['body' body(fnobj.body_processor())]] !(fnobj.set_text(ast.text))

class_definition :modobj =  ['class' [:name :parent]
                              ['fields' :fields]
                                !(modobj.new_class(name, parent, fields)):klass
                              constructors(klass)
                              [instance_method(klass)*]
                              [class_method(klass)*]]

constructor :klass =  !(self.input.head()[0]):ast
                        ['ctor' :name params:p !(klass.new_ctor(name, p)):fnobj
                             !(fnobj.set_line(ast))
                             :uses_env !(fnobj.uses_env(uses_env))
                             ['body' body(fnobj.body_processor())]]  !(fnobj.set_text(ast.text))

constructors :klass = ['ctors' [constructor(klass)*]]
                     | ['ctors' []]

instance_method :klass = !(self.input.head()[0]):ast
                           ['fun' :name params:p !(klass.new_instance_method(name, p)):fnobj
                                  !(fnobj.set_line(ast))
                                  :uses_env !(fnobj.uses_env(uses_env))
                                  ['body' body(fnobj.body_processor())]]  !(fnobj.set_text(ast.text))

class_method :klass = !(self.input.head()[0]):ast
                         ['fun' :name params:p  !(klass.new_class_method(name, p)):fnobj
                          !(fnobj.set_line(ast))
                          :uses_env !(fnobj.uses_env(uses_env))
                          ['body' body(fnobj.body_processor())]]  !(fnobj.set_text(ast.text))

params = ['params' [param*:x]] -> x

param = ['var-arg' :x] -> x
        | :x

object_definition :modobj = ['object' :name !(modobj.new_object(name)):obj
                          [obj_slot(obj)+] [obj_function(obj)*]]

obj_slot :obj = ['slot' obj_slot_value(obj)]

obj_slot_value :obj  =  :name ['literal-number' :x]       -> obj.add_slot_literal_num(name,x)
                     |  :name ['literal-string' :x]       -> obj.add_slot_literal_string(name,x)
                     |  :name ['literal' 'null']            -> obj.add_slot_literal_null(name)
                     |  :name ['literal-array' [(:any)*]:x] -> obj.add_slot_literal_array(name, x)
                     |  :name ['literal-dict' (:any)*:x]  -> obj.add_slot_literal_dict(name, x)
                     |  :name :x                          -> obj.add_slot_ref(name, x)

obj_function :obj = constructor(obj)
                  | function_definition(obj)


body :fnobj = [(expr(fnobj)*) ['end-body']]:ast -> fnobj.emit_return_this(ast)
            | [['primitive' ['literal-string' :name]] (:ignore)*]   -> fnobj.set_primitive(name)

exprs :fnobj = [expr(fnobj)*]

expr_elif :fnobj :lb = ['elif' expr(fnobj) !(fnobj.emit_jz()):label [expr(fnobj)* !(fnobj.emit_jmp(lb=lb))]] !(label.as_current())

stm :fnobj :ast = 'var-def' :id expr(fnobj) ->  fnobj.emit_var_decl(ast, id)
               | 'return' expr(fnobj)       ->  fnobj.emit_return_top(ast)
               | 'return-null' -> fnobj.emit_return_null(ast)
               | 'return-top' -> fnobj.emit_return_top(ast)
               | 'non-local-return' expr(fnobj) ->  fnobj.emit_non_local_return(ast)
               | 'super-ctor-send' :s args(fnobj):arity -> fnobj.emit_super_ctor_send(ast, s, arity)
               | 'send-or-local-call' :name args(fnobj):arity -> fnobj.emit_send_or_local_call(ast, name, arity)
               | 'super-send' args(fnobj):arity -> fnobj.emit_super_send(ast, arity)
               | 'send' :e :s args(fnobj):arity apply('expr' fnobj e) -> fnobj.emit_send(ast, s, arity)
               | 'call' :e args(fnobj):arity apply('expr' fnobj e) -> fnobj.emit_call(ast, arity)
               | 'expression' expr(fnobj) -> fnobj.emit_pop(ast)
               | 'not' expr(fnobj) -> fnobj.emit_unary(ast, '!')
               | 'negative' expr(fnobj) -> fnobj.emit_unary(ast, '-')
               | 'bit-neg' expr(fnobj) -> fnobj.emit_unary(ast, '~')
               | 'and' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, 'and')
               | 'or' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, 'or')
               | '+'  :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '+')
               | '-'  :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '-')
               | '*'  :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '*')
               | '<'  :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '<')
               | '<=' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '<=')
               | '>'  :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '>')
               | '>=' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '>=')
               | '==' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '==')
               | '!=' :e expr(fnobj) apply('expr' fnobj e) -> fnobj.emit_binary(ast, '!=')
               | 'if' expr(fnobj) !(fnobj.emit_jz()):label [expr(fnobj)* !(fnobj.emit_jmp()):lb2] !(label.as_current()) [expr_elif(fnobj lb2)*] [expr(fnobj)*] !(lb2.as_current())
               | 'while' !(fnobj.label_for_current()):lbcond
                   expr(fnobj)
                   !(fnobj.emit_jz()):lbend [expr(fnobj)*] !(fnobj.emit_jmp_back(lbcond.as_current())) -> lbend.as_current()
               | 'try'
                  !(fnobj.ref_for_current()):label_begin_try
                    [expr(fnobj)*]
                  !(fnobj.emit_catch_jump()):end_pos
                    catch_decl:cp
                  !(fnobj.ref_for_current()):label_begin_catch
                  !(fnobj.bind_catch_var(cp[1]))
                    [expr(fnobj)*] -> fnobj.emit_try_catch(label_begin_try, label_begin_catch, end_pos, cp[0])
               | '=' ['id' :v] expr(fnobj)    -> fnobj.emit_local_assignment(ast, v)
               | '=' ['index' :lhs expr(fnobj)] expr(fnobj) apply('expr' fnobj lhs)  -> fnobj.emit_index_assignment(ast)
               | '=' ['field' :f] expr(fnobj) -> fnobj.emit_field_assignment(ast, f)
               | 'literal-number' :x -> fnobj.emit_push_num_literal(ast, x)
               | 'literal' 'this' -> fnobj.emit_push_this(ast)
               | 'literal-string' :x -> fnobj.emit_push_str_literal(ast, x)
               | 'literal-symbol' :x -> fnobj.emit_push_sym_literal(ast, x)
               | 'literal' 'null'       -> fnobj.emit_push_null(ast)
               | 'literal' 'true'       -> fnobj.emit_push_true(ast)
               | 'literal' 'false'      -> fnobj.emit_push_false(ast)
               | 'literal' 'module'     -> fnobj.emit_push_module(ast)
               | 'literal' 'context'    -> fnobj.emit_push_context(ast)
               | 'id' :name             -> fnobj.emit_push_var(ast, name)
               | 'field' :name          -> fnobj.emit_push_field(ast, name)
               | 'literal-array'  :e apply('exprs' fnobj e)      -> fnobj.emit_push_list(ast, len(e))
               | 'literal-dict'   dict_pairs(fnobj):p   -> fnobj.emit_push_dict(ast, len(p))
               | 'index' :e expr(fnobj) apply('expr' fnobj e)    -> fnobj.emit_push_index(ast)

expr :fnobj = !(self.input.head()[0]):ast [stm(fnobj ast)]
            | funliteral(fnobj)


catch_decl = ['catch' ['id' :type] :id]  -> (type, id)
           | ['catch' :id]               -> (None, id)

dict_pairs :fnobj = (['pair' expr(fnobj) expr(fnobj)])*:e -> e

funliteral :fnobj = !(self.input.head()[0]):ast ['fun-literal'  params:p
                       !(fnobj.new_closure(p)):fn
                       !(fn.set_line(ast))
                       ['body' [expr(fn)*]]]:ast
                     !(fn.set_text(ast.text))
                      -> fnobj.emit_push_closure(ast, fn)

cfunliteral :fnobj =  !(self.input.head()[0]):ast
                      !(fnobj.set_line(ast))
                      [expr(fnobj)*] -> fnobj

args :fnobj =  ['args' []] -> 0
            |  ['args' arglist(fnobj):arity] -> arity

arglist :fnobj = [expr(fnobj)+]:x -> len(x)
