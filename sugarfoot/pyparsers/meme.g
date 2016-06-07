alpha =  '+' | '*' | '-' | '/' | '=' | '<' | '>' | '?' | '!'

keyword = (token("fun") | token("var")) ~letterOrDigit

id = spaces ~keyword (letter | '_'):x (letterOrDigit|'_')*:xs !(xs.insert(0, x)) -> ''.join(xs)

alpha_name = spaces ~keyword (alpha | letter | '_'):x (letterOrDigit|'_'|alpha)*:xs !(xs.insert(0, x)) -> ''.join(xs)

symbol_name = spaces (alpha | letter | '_'):x (letterOrDigit|'_'|alpha)*:xs !(xs.insert(0, x)) -> ''.join(xs)

letter_or_digit_string = (letter | '_'):x (letterOrDigit|'_')*:xs -> x + ''.join(xs)

space = comment | anything:c ?(c.isspace())
comment = "/*"  (~"*/" anything)* "*/"
        | "//" (~"\n" anything)* "\n"

spaces = space*

license = token(".license")  (~(token(".endlicense")) anything)*:x
          token(".endlicense") -> [".license", ''.join(x)]
        | -> [".license", '']

meta_section = token(".meta") -> [".meta", None]
             |                -> [".meta", None]

preamble_section = spaces token(".preamble") module_params:params
                  preamble_entry*:entries
                  module_alias*:aliases -> [".preamble", params, entries, aliases]

code_section = spaces token(".code")
               module_decl*:d
               token(".end") -> [".code", d]

start = license:lic
        meta_section:meta
        preamble_section:pre
        code_section:code
       -> ["module", lic, meta, pre, code]

module_params = params
              | -> []

preamble_entry = id:name token(":") module_spec:s token(";") -> ["param", name, s]

module_spec = id:ns token(":") id:mname -> ["library", ns, mname]

module_alias = spaces token("[") idlist:lst token("]") token("<=") id:x token(";") -> ["alias", x, lst]

module_decl = obj_decl | class_decl | top_level_fun | top_level_fn

obj_decl = token("object") id:name
             object_slot+:s
             obj_fun:f
           token("end")  -> ["object", name, s, f]

obj_fun =  token("functions") token("{")
              (constructor|top_level_fun)+:f
           token("}") -> f
        | -> []

object_slot = id:name  token(":") (literal|id):value token(";") -> ['slot', name, value]

class_decl = token("class") id:name (token("<") id | token("<") token("null") | -> "Object"):parent
                fields_:f
                constructors:c
                instance_method_decl*:im
                class_method_decl*:cm
              token("end") -> ['class', [name, parent], f, c, im, cm]

fields_ = spaces token("fields") token(":") idlist:xs token(";") -> ['fields', xs]
      | -> ["fields", []]

constructors = constructor*:c -> ["ctors", c]

constructor = spaces !(self.has_fun_literal(False))
              token("init") alpha_name:name token(":")
              spaces !(self.input.position):begin
              token("fun") fparams:p token("{")
                  top_fun_body:body !(self.input.position):end
                token("}")
             -> self.i.ast(begin,['ctor', name, ["params", p], self.has_fun_literal(),
                  ['body', self.i.ast(begin, body + [self.i.sint_ast(end,['end-body'])])]])

top_level_fn = spaces alpha_name:name token(":") !(self.input.position):begin
                expr:e token(";") -> self.i.ast(begin,['fun', name, ['params', []],
                                                              ['body', self.i.ast(begin, [e])]])

top_level_fun = spaces !(self.has_fun_literal(False))
                alpha_name:name token(":")
                spaces fun_rest(name)

fun_rest :name = !(self.input.position):begin
                token("fun")  fparams:p token("{")
                  top_fun_body:body !(self.input.position):end
                token("}")
                  -> self.i.ast(begin,['fun', name, ["params", p], self.has_fun_literal(),
                                              ['body', self.i.sint_ast(end, body + [self.i.sint_ast(end,['end-body'])])]])


instance_method_decl = spaces !(self.has_fun_literal(False))
                       token("instance_method") alpha_name:name token(":")
                       spaces fun_rest(name)


class_method_decl = spaces !(self.has_fun_literal(False))
                    token("class_method") alpha_name:name token(":")
                    spaces fun_rest(name)

params = token("(") idlist:xs token(")") -> xs

fparams = token("(") token(")") -> []
        | token("(")  token("*") id:x token(")") -> [['var-arg', x]]
        | token("(")  id:x (token(",") id)*:xs token(")") -> [x]+xs
        | token("(")  id:x (token(",") id)*:xs pvar:y token(")") -> [x]+xs+[y]

pvar = token(",") token("*") id:x -> ['var-arg', x]

idlist = id:x (token(",") id)*:xs -> [x]+xs
          | -> []

top_fun_body = primitive
             | stmts

primitive =  spaces !(self.input.position):begin token("<primitive") lit_string:s token(">") -> self.i.ast(begin, [self.i.ast(begin,["primitive", s])])

stmts  = !(self.input.position):begin stmt*:x -> self.i.ast(begin, x)

stmt = control_expr
     | non_control_expr:e token(";") -> e

non_control_expr = expr_ret
                 | expr_non_local_ret
                 | expr_attr
                 | expr:e -> ['expression', e]
                 | expr_decl

expr_ret =  spaces !(self.input.position):begin token("return") expr:e -> self.i.ast(begin,['return', e])
expr_non_local_ret = spaces !(self.input.position):begin token("^") expr:e -> self.i.ast(begin,['non-local-return', e])

expr_decl =  spaces !(self.input.position):begin token("var")
              id:name token("=") expr:e -> self.i.ast(begin,["var-def", name, e])

expr_attr =  spaces !(self.input.position):begin lhs:a token("=") expr:b -> self.i.ast(begin,["=", a, b])

lhs = !(self.input.position):begin expr:r ?(len(r)>0 and r[0] == 'index') -> self.i.ast(begin, r)
    | !(self.input.position):begin alpha_name:x -> self.i.ast(begin, ["id", x])
    | !(self.input.position):begin field_name:x -> self.i.ast(begin, ["field", x])

control_expr = expr_if
             | expr_while
             | expr_try

expr_if = spaces !(self.input.position):begin token("if") token("(") expr:e token(")") token("{")
           stmts:body
          token("}")
          expr_elif*:elif_
          (expr_else)?:else_ -> self.i.ast(begin, ["if", e, body, elif_, else_ or self.i.ast(begin,[])])

expr_elif = !(self.input.position):begin token("elif") token("(") expr:e token(")") token("{") stmts:body token("}") -> self.i.ast(begin, ["elif", e, body])

expr_else = !(self.input.position):begin token("else") token("{") stmts:body token("}") -> body


expr_while = spaces !(self.input.position):begin token("while") token("(") expr:e token(")") token("{")
             stmts:xs
             token("}") -> self.i.ast(begin,['while', e, xs])

expr_try = spaces !(self.input.position):begin token("try") token("{")
             stmts:s_try
           token("}") catch_part:c token("{")
             stmts:s_catch
           token("}")
          -> self.i.ast(begin, ['try', s_try, c, s_catch])

catch_part = !(self.input.position):begin token("catch") token("(") alpha_name:id token(")")
             -> self.i.ast(begin, ['catch', id])
           | !(self.input.position):begin token("catch") token("(") catch_type:t alpha_name:id token(")")
             -> self.i.ast(begin, ['catch', t, id])

catch_type = !(self.input.position):begin alpha_name:type -> self.i.ast(begin, ['id', type])

expr = spaces expr_or

expr_or =  !(self.input.position):begin expr_or:a token("or") expr_and:b -> self.i.ast(begin,['or', a, b])
        | expr_and

expr_and =  !(self.input.position):begin expr_and:a token("and") expr_eq:b -> self.i.ast(begin,['and', a, b])
         | expr_eq

expr_eq =  !(self.input.position):begin expr_eq:a token("==") expr_rel:b -> self.i.ast(begin,['==', a, b])
        |  !(self.input.position):begin expr_eq:a token("!=") expr_rel:b -> self.i.ast(begin,['!=', a, b])
        | expr_rel

expr_rel =  !(self.input.position):begin expr_rel:a token(">=") expr_add:b -> self.i.ast(begin, ['>=', a, b])
         |  !(self.input.position):begin expr_rel:a token(">") expr_add:b -> self.i.ast(begin, ['>', a, b])
         |  !(self.input.position):begin expr_rel:a token("<=") expr_add:b -> self.i.ast(begin, ['<=', a, b])
         |  !(self.input.position):begin expr_rel:a token("<") expr_add:b -> self.i.ast(begin, ['<', a, b])
         | expr_add

expr_add =  !(self.input.position):begin expr_add:a token("++") expr_mul:b -> self.i.ast(begin, ['++', a, b])
         |  !(self.input.position):begin expr_add:a token("+") expr_mul:b -> self.i.ast(begin, ['+', a, b])
         |  !(self.input.position):begin expr_add:a token("-") expr_mul:b -> self.i.ast(begin, ['-', a, b])
         | expr_mul

expr_mul =  !(self.input.position):begin expr_mul:a token("*") expr_unary:b -> self.i.ast(begin, ['*', a, b])
         |  !(self.input.position):begin expr_mul:a token("/") expr_unary:b -> self.i.ast(begin, ['/', a, b])
         | expr_unary

expr_unary =   spaces !(self.input.position):begin token("+") prim_expr:a  -> self.i.ast(begin, ['positive', a])
            |  spaces !(self.input.position):begin token("-") prim_expr:a  -> self.i.ast(begin, ['negative', a])
            |  spaces !(self.input.position):begin token("!") expr_unary:a -> self.i.ast(begin, ['not', a])
            |  spaces !(self.input.position):begin token("~") expr_unary:a -> self.i.ast(begin, ['bit-neg', a])
            | spaces suffix_expr

suffix_expr = !(self.input.position):begin token("super") token(".") alpha_name:sel args:p
            -> self.i.ast(begin, ['super-ctor-send', sel, ['args', p]])
          |  !(self.input.position):begin suffix_expr:r token(".") alpha_name:sel args:p
            -> self.i.ast(begin, ['send', r, sel, ['args', p]])
          |  !(self.input.position):begin suffix_expr:r token(".") alpha_name:sel
            -> self.i.ast(begin, ['send', r, sel, ['args', []]])
          | !(self.input.position):begin suffix_expr:r token("[") expr:i token("]")
             -> self.i.ast(begin, ['index', r, i])
          | call_expr

call_expr =  !(self.input.position):begin call_expr:r args:p
            -> self.i.ast(begin, ['call', r, ['args', p]])
          |  spaces !(self.input.position):begin token("super") args:p
            -> self.i.ast(begin, ['super-send', ['args',p]])
          |  spaces !(self.input.position):begin id:r args:p
            -> self.i.ast(begin, ['send-or-local-call', r, ['args',p]])
          | prim_expr

prim_expr = token("(") expr:e token(")") -> e
          | literal
          |  spaces !(self.input.position):begin field_name:x -> self.i.ast(begin, ["field", x])
          |  spaces !(self.input.position):begin alpha_name:x -> self.i.ast(begin, ["id", x])

pair_list = pair:x (token(",") pair)*:xs -> [x]+xs
          | -> []

pair = !(self.input.position):begin expr:key token(":") expr:val -> self.i.ast(begin, ['pair', key, val])

args = token("(") expr_list:p  token(")") -> p

expr_list = expr:x (token(",") expr)*:xs -> [x]+xs
          | -> self.i.ast(self.input.position, [])

literal = lit_number
        | lit_string
        | lit_symbol
        | spaces !(self.input.position):begin token("[") expr_list:e token("]")    -> self.i.ast(begin, ['literal-array']+[e])
        | spaces !(self.input.position):begin token("{") pair_list:e token("}")    -> self.i.ast(begin, ['literal-dict']+e)
        | spaces !(self.input.position):begin token("thisModule")-> self.i.ast(begin, ['literal', 'module'])
        | spaces !(self.input.position):begin token("thisContext")-> self.i.ast(begin,['literal', 'context'])
        | spaces !(self.input.position):begin token("this")   -> self.i.ast(begin,['literal', 'this'])
        | spaces !(self.input.position):begin token("null")   -> self.i.ast(begin, ['literal', 'null'])
        | spaces !(self.input.position):begin token("true")   -> self.i.ast(begin,['literal', 'true'])
        | spaces !(self.input.position):begin token("false")  -> self.i.ast(begin, ['literal', 'false'])
        | funliteral:x !(self.has_fun_literal(True)) -> x

funliteral = spaces !(self.input.position):begin token("fun") params:p token("{")
               funliteral_body:body
             token("}") -> self.i.ast(begin, ['fun-literal', ["params", p], ["body", body]])

funliteral_body = !(self.input.position):begin
                  stmts:body
                    expr?:no_semicol_expr !(not no_semicol_expr or body.append(['expression', no_semicol_expr]))
                    !(body.get(-1, [])):last
                    rewrite_last_stmt(last)
                -> self.i.ast(begin, body + self.i.sint_ast(self.input.position,[self.i.sint_ast(self.input.position,['return-null'])]))


rewrite_last_stmt = ['expression' :x]:c -> c.__setitem__(0, 'return')
                  | :x

cfunliteral_body = funliteral_body:x spaces ~anything -> x

lit_symbol = spaces !(self.input.position):begin token(":") symbol_name:xs
           -> self.i.ast(begin, ["literal-symbol", xs])

lit_number = spaces !(self.input.position):begin digit+:ds -> self.i.ast(begin, ["literal-number", int(''.join(ds))])

lit_string  = spaces !(self.input.position):begin '"' ( lit_escaped | ~'"' :x)*:xs '"'
               -> self.i.ast(begin, ["literal-string", b''.join(xs).decode("string_escape")])

lit_escaped = ~'"' '\\' :x -> "\\" + x

field_name = spaces '@' letter_or_digit_string:x -> x


single_top_level_fun :name = spaces !(self.input.position):begin token("fun")
                             fparams:p token("{")
                         top_fun_body:body !(self.input.position):end
                       token("}")
                       -> self.i.ast(begin,['fun', name, ["params", p],
                              ['body', self.i.sint_ast(end, body + [self.i.sint_ast(end,['end-body'])])]])
