meme central:memescript/compiler

requires ometa_base, io
where
  ometa_base = central:memescript/ometa_base
  io         = central:stdlib/io
import OMetaBase from ometa_base


//extending List for AST node information
class List < List
fields: start_line, end_line, start_col, end_col, text;
instance_method start_line: fun() {
  return @start_line;
}
instance_method end_line: fun() {
  return @end_line;
}
instance_method start_col: fun() {
  return @start_col;
}
instance_method end_col: fun() {
  return @end_col;
}
instance_method text: fun() {
  return @text;
}
instance_method toSource: fun() {
  var ret = super();
  // var pos = "@<" + @start_line.toString + ":" + @start_col.toString + ":" + @end_line.toString + ":" + @end_col.toString + ">";
  return ret;// + pos;
}
instance_method at: fun(parser, begin_pos) {
  var end_pos = parser.input.idx;
  var full_input = parser.input.data; //assert type == String?
  @text = full_input.substr(begin_pos, end_pos - begin_pos);
  @start_line = full_input.substr(0, begin_pos).count("\n") + parser.line_offset;

  @start_col = begin_pos - full_input.substr(0, begin_pos).rindex("\n") - 1;
  @end_line = @start_line + @text.count("\n");
  var inside_nl = @text.rindex("\n");
  if (inside_nl == -1) {
    @end_col = @start_col + @text.size;
  } else {
    @end_col = @text.substr(inside_nl, @text.size - inside_nl).size;
  }
}
instance_method +: fun(other) { //#[] + [] yields a Core.List. This is a workaround
  var ret = [];
  this.each(fun(_, x) { ret.append(x) });
  other.each(fun(_, x) { ret.append(x) });
  return ret;
}
instance_method extends: fun(other) {
  other.each(fun(_, x) { this.append(x) });
}
end

class MemeScriptParser < OMetaBase
fields: has_fun_literal, line_offset;
init new: fun(input) {
  super.new(input);
  @has_fun_literal = false;
  @line_offset = 0;
}
instance_method set_line_offset: fun(line_offset) {
  @line_offset = line_offset;
}
instance_method line_offset: fun() {
  return @line_offset;
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
alpha =  '+' | '*' | '-' | '/' | '=' | '<' | '>' | '?' | '!' | '&' | '|';

meme_keyword = ``fun`` | ``var`` | ``class`` | ``fields``;

id = spaces ~meme_keyword identifier:i => i;

alpha_name = spaces ~meme_keyword {alpha | letter | '_'}:x {identifier_rest|alpha}*:xs => ([x] + xs).join("");

symbol_name = spaces {alpha | letter | '_'}:x {identifier_rest|'_'|alpha}*:xs !{xs.prepend(x)} => xs.join("");

space =  _:c ?{c.onlySpaces} => c
      | comment;

comment = '/*'  {~'*/' _}* '*/'
        | '//' {~'\n' _}* '\n';


start = 'm' 'e' 'm' 'e' space compiler_line:c
        meta_section:meta
        requirements_section:req
        code_section:code
        spaces $
        => [:module, c, meta, req, code];

compiler_line = {~'\n' _}+:xs => xs.join("");

meta_section = meta_variable*:xs => [:meta, xs];

meta_variable = spaces '@' spaces id:key ":" {~'\n' _}+:xs => [key, xs.join("")];

requirements_section = ``requires`` module_params:params
                         where_section:specs
                         module_import*:imp
                       => [:requirements, params, [:default-locations, specs], [:imports, imp]]
                     | => [:requirements, [], [:default-locations, []], [:imports, []]];

where_section = ``where`` module_default*
              | => []
              ;

module_params = id:x {"," id}*:xs => [x] + xs;

module_default = id:name "=" spaces module_location:loc => [name, loc];

module_location = {~'\n' _}+:xs => xs.join("");

module_import = ``import`` id:name ``from`` id:lib => [name, lib];


code_section = module_decl*:d => [:code, d];

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

constructor =  !{@has_fun_literal = false}
              ``init`` alpha_name:name ":"
              # ``fun`` fparams:p "{"
                  top_fun_body:body
                end_body:e
             => #[:ctor, name, [:params, p],
                  @has_fun_literal, [:body,  body + [e]]];

top_level_fn = alpha_name:name ":" #
                expr:e ";" => #[:fun, name, [:params, []], false, #[:body, #[[:return, e]]]];

top_level_fun = spaces !{@has_fun_literal = false}
                alpha_name:name ":"
                spaces fun_rest(name):r => r;

fun_rest :name = # ``fun``  fparams:p "{"
                  top_fun_body:body
                end_body:e
                  => #[:fun, name, [:params, p],
                       @has_fun_literal, [:body,  body + [e]]];

end_body = # "}" => #[:end-body];

instance_method_decl = !{@has_fun_literal = false}
                       ``instance_method`` alpha_name:name ":"
                       spaces fun_rest(name):r => r;


class_method_decl = !{@has_fun_literal = false}
                    ``class_method`` alpha_name:name ":"
                    spaces fun_rest(name):r => r;

params = "(" idlist:xs ")" => xs;

fparams = "(" ")" => []
        | "("  "*" ")" => [[:var-arg]]
        | "("  id:x { "," id }*:xs ")" => [x] + xs
        | "("  id:x { "," id }*:xs pvar:y ")" => [x] + xs + [y];


pvar = "," "*"  => [:var-arg];

idlist = id:x {"," id}*:xs => [x] + xs
          | => [];

top_fun_body = primitive
             | stmts
             ;

primitive =  "<primitive" spaces lit_string:s ">" => [[:primitive, s]];

stmts  =  stmt*:s => s;

stmt = control_expr
     | non_control_expr:e ";" => e;

non_control_expr = expr_ret
                 | expr_non_local_ret
                 | expr_assign
                 | expr:e => [:expression, e]
                 | expr_decl;

expr_ret =  # ``return`` expr:e => #[:return, e];

expr_non_local_ret =  # "^" expr:e => #[:non-local-return, e];

expr_decl =  # ``var`` id:name "=" expr:e => #[:var-def, name, e];

expr_assign =  # lhs:a "=" expr:b => #[:=, a, b]
            |  # lhs:a "+=" expr:b => #[:=, a, [:+, a, b]]
            |  # lhs:a "-=" expr:b => #[:=, a, [:-, a, b]]
            ;

lhs =  expr:r ?{r.size > 0 and r[0] == :index} => r
    |  # alpha_name:x => #[:id, x]
    |  # field_name:x => #[:field, x];

control_expr = expr_if
             | expr_while
             | expr_try
             | expr_for
             ;

expr_for = # ``for`` "(" {expr_decl | expr_assign}:v ";" expr:c ";" expr_assign:n ")" "{"
               stmts:st
             "}"
           => [:for, v, c, st, n]
         ;

expr_if = # ``if`` "(" expr:e ")" "{"
           stmts:body
          "}"
          expr_elif*:elif_
          expr_else?:else_ => #[:if, e, body, elif_, else_ or []];

expr_elif =  # ``elif`` "(" expr:e ")" "{" stmts:body "}" => #[:elif, e, body];

expr_else =  ``else`` "{" stmts:body "}" => body;


expr_while =  # ``while`` "(" expr:e ")" "{"
             stmts:xs
             "}" => #[:while, e, xs];

expr_try =  # ``try`` "{"
             stmts:s_try
           "}" catch_part:c "{"
             stmts:s_catch
           "}"
          => #[:try, s_try, c, s_catch];

catch_part =  # ``catch`` "(" alpha_name:id ")"
             => #[:catch, id]
           |  # ``catch`` "(" catch_type:t alpha_name:id ")"
             => #[:catch, t, id];

catch_type =  # alpha_name:type => #[:id, type];

expr = spaces expr_or:e => e;

expr_or =  #! expr_or:a "or" spaces expr_and:b => #[:or, a, b]
        | expr_and;

expr_and = #! expr_and:a "and" spaces expr_eq:b => #[:and, a, b]
         | expr_eq;

expr_eq =  #! expr_eq:a "==" spaces expr_rel:b => #[:==, a, b]
        |  #! expr_eq:a "!=" spaces expr_rel:b => #[:!=, a, b]
        | expr_rel;

expr_rel =  #! expr_rel:a ">="     spaces expr_add:b => #[:>=, a, b]
         |  #! expr_rel:a ">" ~'>' spaces expr_add:b => #[:>, a, b]
         |  #! expr_rel:a "<="     spaces expr_add:b => #[:<=, a, b]
         |  #! expr_rel:a "<" ~'<' spaces expr_add:b => #[:<, a, b]
         | expr_add;

expr_add =  #! expr_add:a "++" spaces expr_mul:b => #[:++, a, b]
         |  #! expr_add:a "+"  spaces expr_mul:b => #[:+, a, b]
         |  #! expr_add:a "-"  spaces expr_mul:b => #[:-, a, b]
         |  #! expr_add:a "<<" spaces expr_mul:b => #[:<<, a, b]
         |  #! expr_add:a ">>" spaces expr_mul:b => #[:>>, a, b]
         |  #! expr_add:a "&"  spaces expr_mul:b =>  #[:&, a, b]
         |  #! expr_add:a "|"  spaces expr_mul:b =>  #[:|, a, b]
         | expr_mul;

expr_mul =  #! expr_mul:a "*" spaces expr_unary:b => #[:*, a, b]
         |  #! expr_mul:a "/" spaces expr_unary:b => #[:/, a, b]
         | expr_unary;

expr_unary =  # "+" spaces prim_expr:a  => #[:positive, a]
            | # "-" spaces prim_expr:a  => #[:negative, a]
            | # "!" spaces expr_unary:a => #[:not, a]
            | # "*" spaces expr_unary:a => #[:get-var-arg, a]
            | # "~" spaces expr_unary:a => #[:bit-neg, a]
            | suffix_expr;

suffix_expr = # ``super`` "." alpha_name:sel args:p
            => #[:super-ctor-send, sel, [:args, p]]
          |  # suffix_expr:r "." alpha_name:sel args:p
            => #[:send, r, sel, [:args, p]]
          |  # suffix_expr:r "." alpha_name:sel
            => #[:send, r, sel, [:args, []]]
          |  # suffix_expr:r "[" expr:i "]"
             => #[:index, r, i]
          | call_expr;

call_expr =  # call_expr:r args:p
            => #[:call, r, [:args, p]]
          |  # ``super`` args:p
            => #[:super-send, [:args, p]]
          |  #  id:r args:p
            => #[:send-or-local-call, r, [:args, p]]
          | prim_expr;

prim_expr = "(" expr:e ")" => e
          | literal
          |  # field_name:x => #[:field, x]
          |  # alpha_name:x => #[:id, x]; //should it be id?

pair_list = pair:x {"," pair}*:xs => [x] + xs
          | => [];

pair =  # expr:key ":" expr:val => #[:pair, key, val];

args = "(" expr_list:p  ")" => p;

expr_list = expr:x {"," expr}*:xs => [x] + xs
          | => [];

literal = lit_number
        | lit_string
        | lit_symbol
        | # "[" expr_list:e "]"    => #([:literal-array].extends([e]))
        | # "{" pair_list:e "}"    => #([:literal-dict]).extends(e)
        | # ``thisModule`` => #[:literal, :module]
        | # ``thisContext`` => #[:literal, :context]
        | # ``this``   => #[:literal, :this]
        | # ``null``   => #[:literal, :null]
        | # ``true``   => #[:literal, :true]
        | # ``false``  => #[:literal, :false]
        | # "*#"       => #[:literal, :argc]
        | funliteral:x !{@has_fun_literal = true} => x;

funliteral = # ``fun`` params:p "{"
               funliteral_body:body
             end_body:e => #[:fun-literal, [:params, p], [:body, body + [e]]];

funliteral_body = stmts:body
                    expr?:no_semicol_expr
                   !{this.maybe_append_semicol_expr(body, no_semicol_expr)}
                   !{this.last_or_empty(body)}:last
                    rewrite_last_stmt(last)
                => body;

rewrite_last_stmt = [:expression _]:c => c.set(0, :return)
                  | _
                  ;

cfunliteral_body = funliteral_body:x # spaces ~_ => x + #[#[:end-body]];

lit_symbol = # ":" symbol_name:xs
           => #[:literal-symbol, xs];

lit_number = # {base_10 | base_16}:x => #[:literal-number, x];

base_10 = digit+:ds ~letter => ds.join("").toInteger;

base_16 = '0' 'x' {digit | 'A' | 'B' | 'C' | 'D' | 'E' | 'F'}+:xs ~letter => xs.join("").asHex;


lit_string  = # '"' { lit_escaped | ~'"' _}*:xs '"'
               => #[:literal-string, xs.join("").escape];

lit_escaped = ~'"' '\\' _:x => "\\" + x;

field_name = "@" id:i => i;


single_top_level_fun :name = # ``fun`` fparams:p "{"
                         top_fun_body:body
                       end_body:e
                       => #[:fun, name, [:params, p], false,
                              [:body, body + [e]]];
</ometa>

end //OMeta
