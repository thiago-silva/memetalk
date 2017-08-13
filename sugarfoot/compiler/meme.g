.preamble(ometa_base, io)
  ometa_base: meme:ometa_base;
  io : meme:io;
  [OMetaBase] <= ometa_base;
.code

class MemeScriptParser < OMetaBase
fields: has_fun_literal;
init new: fun(input) {
  super.new(input);
  @has_fun_literal = false;
}
instance_method last_or_empty: fun(lst) {
  if (lst.size > 0) {
    return lst.last;
  } else {
    return [];
  }
}
instance_method maybe_append_semicol_expr: fun(body, no_semicol_expr) {
  if(no_semicol_expr) {
    body.append([:expression, no_semicol_expr]);
  }
}

<ometa>
alpha =  '+' | '*' | '-' | '/' | '=' | '<' | '>' | '?' | '!';

meme_keyword = ``fun`` | ``var`` | ``class`` | ``fields``;

id = ~meme_keyword identifier;

alpha_name = spaces ~meme_keyword {alpha | letter | '_'}:x {identifier_rest|alpha}*:xs => ([x] + xs).join("");

symbol_name = spaces {alpha | letter | '_'}:x {identifier_rest|'_'|alpha}*:xs !{xs.prepend(x)} => xs.join("");

space =  _:c ?{c.onlySpaces} => c
      | comment;

comment = '/*'  {~'*/' _}* '*/'
        | '//' {~'\n' _}* '\n';

license = ``.license``  {~{``.endlicense``} _}*:x
          ``.endlicense`` => [:license, x.join("")]
        | => [:license, ""];

meta_section = ``.meta`` => [:meta, null]
             |           => [:meta, null];

preamble_section = ``.preamble`` module_params:params
                  preamble_entry*:entries
                  module_alias*:aliases => [:preamble, params, entries, aliases];

code_section = ``.code``
               module_decl*:d
               ``.endcode`` => [:code, d];

start = license:lic
        meta_section:meta
        preamble_section:pre
        code_section:code
       => [:module, lic, meta, pre, code];

module_params = params
              | => [];

preamble_entry = id:name ":" module_spec:s ";" => [:param, name, s];

module_spec = id:ns ":" id:mname => [:library, ns, mname];

module_alias = "[" idlist:lst "]" "<=" id:x ";" => [:alias, x, lst];

module_decl = obj_decl | class_decl | top_level_fun | top_level_fn;

obj_decl = ``object`` id:name
             object_slot+:s
             obj_fun:f
           ``end``  => [:object, name, s, f];

obj_fun =  ``functions`` "{"
              {constructor | top_level_fun}+:f
           "}" => f
        | => [];

object_slot = id:name  ":" { literal | id }:value ";" => [:slot, name, value];

class_decl = ``class`` id:name { "<" id | "<" "null" | => "Object" }:parent
                fields_:f
                constructors:c
                instance_method_decl*:im
                class_method_decl*:cm
              ``end`` => [:class, [name, parent], f, c, im, cm];

fields_ = ``fields`` ":" idlist:xs ";" => [:fields, xs]
      | => [:fields, []];

constructors = constructor*:c => [:ctors, c];

constructor = !{@has_fun_literal = false}
              ``init`` alpha_name:name ":"
              ``fun`` fparams:p "{"
                  top_fun_body:body
                "}"
             => [:ctor, name, [:params, p], @has_fun_literal,
                  [:body,  body + [[:end-body]]]];

top_level_fn = spaces alpha_name:name ":"
                expr:e ";" => [:fun, name, [:params, []], [:body,  [e]]];

top_level_fun = spaces !{@has_fun_literal = false}
                alpha_name:name ":"
                spaces fun_rest(name);

fun_rest :name =
                ``fun``  fparams:p "{"
                  top_fun_body:body
                "}"
                  => [:fun, name, [:params, p], @has_fun_literal,
                                              [:body,  body + [[:end-body]]]];


instance_method_decl = !{@has_fun_literal = false}
                       ``instance_method`` alpha_name:name ":"
                       spaces fun_rest(name);


class_method_decl = !{@has_fun_literal = false}
                    ``class_method`` alpha_name:name ":"
                    spaces fun_rest(name);

params = "(" idlist:xs ")" => xs;

fparams = "(" ")" => []
        | "("  "*" id:x ")" => [[:var-arg, x]]
        | "("  id:x { "," id }*:xs ")" => [x] + xs
        | "("  id:x { "," id }*:xs pvar:y ")" => [x] + xs + [y];

pvar = "," "*" id:x => [:var-arg, x];

idlist = id:x {"," id}*:xs => [x] + xs
          | => [];

top_fun_body = primitive
             | stmts;

primitive =  "<" ``primiteive`` lit_string:s ">" => [[:primitive, s]];

stmts  =  stmt*:x => x;

stmt = control_expr
     | non_control_expr:e ";" => e;

non_control_expr = expr_ret
                 | expr_non_local_ret
                 | expr_attr
                 | expr:e => [:expression, e]
                 | expr_decl;

expr_ret =  ``return`` expr:e => [:return, e];

expr_non_local_ret =  "^" expr:e => [:non-local-return, e];

expr_decl =  ``var``
              id:name "=" expr:e => [:var-def, name, e];

expr_attr =  spaces  lhs:a "=" expr:b => [:=, a, b];

lhs =  expr:r ?{r.size > 0 and r[0] == "index"} => r
    |  alpha_name:x => [:id, x]
    |  field_name:x => [:field, x];

control_expr = expr_if
             | expr_while
             | expr_try;

expr_if = ``if`` "(" expr:e ")" "{"
           stmts:body
          "}"
          expr_elif*:elif_
          expr_else?:else_ => [:if, e, body, elif_, else_ or []];

expr_elif =  ``elif`` "(" expr:e ")" "{" stmts:body "}" => [:elif, e, body];

expr_else =  ``else`` "{" stmts:body "}" => body;


expr_while =  ``while`` "(" expr:e ")" "{"
             stmts:xs
             "}" => [:while, e, xs];

expr_try =  ``try`` "{"
             stmts:s_try
           "}" catch_part:c "{"
             stmts:s_catch
           "}"
          => [:try, s_try, c, s_catch];

catch_part =  ``catch`` "(" alpha_name:id ")"
             => [:catch, id]
           |  ``catch`` "(" catch_type:t alpha_name:id ")"
             => [:catch, t, id];

catch_type =  alpha_name:type => [:id, type];

expr = spaces expr_or;

expr_or =   expr_or:a ``or`` expr_and:b => [:or, a, b]
        | expr_and;

expr_and =   expr_and:a ``and`` expr_eq:b => [:and, a, b]
         | expr_eq;

expr_eq =   expr_eq:a "==" expr_rel:b => [:==, a, b]
        |   expr_eq:a "!=" expr_rel:b => [:!=, a, b]
        | expr_rel;

expr_rel =   expr_rel:a ">=" expr_add:b => [:>=, a, b]
         |   expr_rel:a ">" expr_add:b => [:>, a, b]
         |   expr_rel:a "<=" expr_add:b => [:<=, a, b]
         |   expr_rel:a "<" expr_add:b => [:<, a, b]
         | expr_add;

expr_add =   expr_add:a "++" expr_mul:b => [:++, a, b]
         |   expr_add:a "+" expr_mul:b => [:+, a, b]
         |   expr_add:a "-" expr_mul:b => [:-, a, b]
         | expr_mul;

expr_mul =   expr_mul:a "*" expr_unary:b => [:*, a, b]
         |   expr_mul:a "/" expr_unary:b => [:/, a, b]
         | expr_unary;

expr_unary =   "+" prim_expr:a  => [:positive, a]
            |  "-" prim_expr:a  => [:negative, a]
            |  "!" expr_unary:a => [:not, a]
            |  "~" expr_unary:a => [:bit-neg, a]
            | spaces suffix_expr;

suffix_expr =  ``super`` "." alpha_name:sel args:p
            => [:super-ctor-send, sel, [:args, p]]
          |   suffix_expr:r "." alpha_name:sel args:p
            => [:send, r, sel, [:args, p]]
          |   suffix_expr:r "." alpha_name:sel
            => [:send, r, sel, [:args, []]]
          |  suffix_expr:r "[" expr:i "]"
             => [:index, r, i]
          | call_expr;

call_expr =   call_expr:r args:p
            => [:call, r, [:args, p]]
          |   ``super`` args:p
            => [:super-send, [:args, p]]
          |    id:r args:p
            => [:send-or-local-call, r, [:args, p]]
          | prim_expr;

prim_expr = "(" expr:e ")" => e
          | literal
          |  spaces  field_name:x => [:field, x]
          |  spaces  alpha_name:x => [:id, x];

pair_list = pair:x {"," pair}*:xs => [x] + xs
          | => [];

pair =  expr:key ":" expr:val => [:pair, key, val];

args = "(" expr_list:p  ")" => p;

expr_list = expr:x {"," expr}*:xs => [x] + xs
          | => [];

literal = lit_number
        | lit_string
        | lit_symbol
        | "[" expr_list:e "]"    => [:literal-array] + [e]
        | "{" pair_list:e "}"    => [:literal-dict] + e
        | ``thisModule`` => [:literal, :module]
        | ``thisContext`` => [:literal, :context]
        | ``this``   => [:literal, :this]
        | ``null``   => [:literal, :null]
        | ``true``   => [:literal, :true]
        | ``false``  => [:literal, :false]
        | funliteral:x !{@has_fun_literal = true} => x;

funliteral = ``fun`` params:p "{"
               funliteral_body:body
             "}" => [:fun-literal, [:params, p], [:body, body]];

funliteral_body =
                  stmts:body
                    expr?:no_semicol_expr
                   !{this.maybe_append_semicol_expr(body, no_semicol_expr)}
                   !{this.last_or_empty(body)}:last
                    rewrite_last_stmt(last)
                => body + [[:return-null]];


rewrite_last_stmt = [:expression _]:c => c.prepend(:return)
                  | _;

cfunliteral_body = funliteral_body:x spaces ~_ => x;

lit_symbol = ":" symbol_name:xs
           => [:literal-symbol, xs];

lit_number = spaces  digit+:ds => [:literal-number, ds.join("").toInteger];

lit_string  = '"' { lit_escaped | ~'"' _}*:xs '"'
               => [:literal-string, xs.join("")];

lit_escaped = ~'"' '\\' _:x => "\\" + x;

field_name = "@" id;


single_top_level_fun :name = ``fun``
                             fparams:p "{"
                         top_fun_body:body
                       "}"
                       => [:fun, name, [:params, p],
                              [:body, body + [:end-body]]];
</ometa>

end //OMeta

.endcode
