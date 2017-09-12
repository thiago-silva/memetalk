alpha =  '+' | '*' | '-' | '/' | '=' | '<' | '>' | '?' | '!' | '&' | '|'

keyword = (token("fun") | token("var")) ~letterOrDigit

id = spaces ~keyword (letter | '_'):x (letterOrDigit|'_')*:xs !(xs.insert(0, x)) -> ''.join(xs)

alpha_name = spaces ~keyword (alpha | letter | '_'):x (letterOrDigit|'_'|alpha)*:xs !(xs.insert(0, x)) -> ''.join(xs)

symbol_name = spaces (alpha | letter | '_'):x (letterOrDigit|'_'|alpha)*:xs !(xs.insert(0, x)) -> ''.join(xs)

letter_or_digit_string = (letter | '_'):x (letterOrDigit|'_')*:xs -> x + ''.join(xs)

space = comment | anything:c ?(c.isspace())
comment = "/*"  (~"*/" anything)* "*/"
        | "//" (~"\n" anything)* "\n"

spaces = space*


start = 'm' 'e' 'm' 'e' space compiler_line:c
        meta_section:meta
        requirements_section:req
        code_section:code
        spaces end
        -> ["module", c, meta, req, code]

compiler_line = (~'\n' anything)+:xs -> "".join(xs)

meta_section = meta_variable*:xs -> ["meta", xs]

meta_variable = spaces '@' spaces id:key token(":")  (~'\n' anything)+:xs -> [key, "".join(xs)]

requirements_section = token("requires") module_params:params
                       where_section:specs
                       module_import*:imp
                      -> ["requirements", params, ["default-locations", specs], ["imports", imp]]
                    | -> ["requirements", [], ["default-locations", []], ["imports", []]]

where_section = token("where") module_default*
              | -> []

module_params = id:x (token(",") id)*:xs -> [x] + xs

module_default = id:name token("=") spaces module_location:loc -> [name, loc]

module_location = (~'\n' anything)+:xs -> "".join(xs)

module_import = token("import") id:name token("from") id:lib -> [name, lib]




code_section = module_decl*:d -> ["code", d]

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
                  top_fun_body:body
              end_body:e
             -> self.i.ast(begin,['ctor', name, ["params", p], self.has_fun_literal(),
                  ['body', self.i.ast(begin, body + [e])]])

end_body = spaces !(self.input.position):begin token("}") -> self.i.ast(begin,['end-body'])

top_level_fn = spaces alpha_name:name token(":") spaces !(self.input.position):begin
                expr:e token(";") -> self.i.ast(begin,['fun', name, ['params', []], False,
                                                              ['body', [['return', e]]]])

top_level_fun = spaces !(self.has_fun_literal(False))
                alpha_name:name token(":")
                spaces fun_rest(name)

fun_rest :name = !(self.input.position):begin
                token("fun")  fparams:p token("{")
                  top_fun_body:body !(self.input.position):end
                end_body:e
                  -> self.i.ast(begin,['fun', name, ["params", p], self.has_fun_literal(),
                                              ['body', self.i.sint_ast(end, body + [e])]])


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
                 | expr_assign
                 | expr:e -> ['expression', e]
                 | expr_decl

expr_ret =  spaces !(self.input.position):begin token("return") expr:e -> self.i.ast(begin,['return', e])
expr_non_local_ret = spaces !(self.input.position):begin token("^") expr:e -> self.i.ast(begin,['non-local-return', e])

expr_decl =  spaces !(self.input.position):begin token("var")
              id:name token("=") expr:e -> self.i.ast(begin,["var-def", name, e])

expr_assign =  spaces !(self.input.position):begin lhs:a token("=") expr:b -> self.i.ast(begin,["=", a, b])
            |  spaces !(self.input.position):begin lhs:a token("+=") expr:b -> self.i.ast(begin,["=", a, ['+', a, b]])
            |  spaces !(self.input.position):begin lhs:a token("-=") expr:b -> self.i.ast(begin,["=", a, ['-', a, b]])

lhs = !(self.input.position):begin expr:r ?(len(r)>0 and r[0] == 'index') -> self.i.ast(begin, r)
    | !(self.input.position):begin alpha_name:x -> self.i.ast(begin, ["id", x])
    | !(self.input.position):begin field_name:x -> self.i.ast(begin, ["field", x])

control_expr = expr_if
             | expr_while
             | expr_try
             | expr_for

expr_if = spaces !(self.input.position):begin token("if") token("(") expr:e token(")") token("{")
           stmts:body
          token("}")
          expr_elif*:elif_
          (expr_else)?:else_ -> self.i.ast(begin, ["if", e, body, elif_, else_ or self.i.ast(begin,[])])

expr_elif = !(self.input.position):begin token("elif") token("(") expr:e token(")") token("{") stmts:body token("}") -> self.i.ast(begin, ["elif", e, body])

expr_else = !(self.input.position):begin token("else") token("{") stmts:body token("}") -> body


expr_for = spaces !(self.input.position):begin token("for") token("(") (expr_decl | expr_assign):v token(";") expr:c token(";") expr_assign:n token(")") token("{")
             stmts:st
           token("}")
           -> self.i.ast(begin,['for', v, c, st, n])

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

expr_or =  !(self.input.position):begin expr_or:a token("or") spaces expr_and:b -> self.i.ast(begin,['or', a, b])
        | expr_and

expr_and =  !(self.input.position):begin expr_and:a token("and") spaces expr_eq:b -> self.i.ast(begin,['and', a, b])
         | expr_eq

expr_eq =  !(self.input.position):begin expr_eq:a token("==") spaces expr_rel:b -> self.i.ast(begin,['==', a, b])
        |  !(self.input.position):begin expr_eq:a token("!=") spaces expr_rel:b -> self.i.ast(begin,['!=', a, b])
        | expr_rel

expr_rel =  !(self.input.position):begin expr_rel:a token(">=")     spaces expr_add:b -> self.i.ast(begin, ['>=', a, b])
         |  !(self.input.position):begin expr_rel:a token(">") ~'>' spaces expr_add:b -> self.i.ast(begin, ['>', a, b])
         |  !(self.input.position):begin expr_rel:a token("<=")     spaces expr_add:b -> self.i.ast(begin, ['<=', a, b])
         |  !(self.input.position):begin expr_rel:a token("<") ~'<' spaces expr_add:b -> self.i.ast(begin, ['<', a, b])
         | expr_add

expr_add =  !(self.input.position):begin expr_add:a token("++") spaces expr_mul:b -> self.i.ast(begin, ['++', a, b])
         |  !(self.input.position):begin expr_add:a token("+")  spaces expr_mul:b -> self.i.ast(begin, ['+', a, b])
         |  !(self.input.position):begin expr_add:a token("-")  spaces expr_mul:b -> self.i.ast(begin, ['-', a, b])
         |  !(self.input.position):begin expr_add:a token("<<") spaces expr_mul:b -> self.i.ast(begin, ['<<', a, b])
         |  !(self.input.position):begin expr_add:a token(">>") spaces expr_mul:b -> self.i.ast(begin, ['>>', a, b])
         |  !(self.input.position):begin expr_add:a token("&")  spaces expr_mul:b -> self.i.ast(begin, ['&', a, b])
         |  !(self.input.position):begin expr_add:a token("|")  spaces expr_mul:b -> self.i.ast(begin, ['|', a, b])
         | expr_mul

expr_mul =  !(self.input.position):begin expr_mul:a token("*") spaces expr_unary:b -> self.i.ast(begin, ['*', a, b])
         |  !(self.input.position):begin expr_mul:a token("/") spaces expr_unary:b -> self.i.ast(begin, ['/', a, b])
         | expr_unary

expr_unary =   spaces !(self.input.position):begin token("+") spaces prim_expr:a  -> self.i.ast(begin, ['positive', a])
            |  spaces !(self.input.position):begin token("-") spaces prim_expr:a  -> self.i.ast(begin, ['negative', a])
            |  spaces !(self.input.position):begin token("!") spaces expr_unary:a -> self.i.ast(begin, ['not', a])
            |  spaces !(self.input.position):begin token("~") spaces expr_unary:a -> self.i.ast(begin, ['bit-neg', a])
            |  suffix_expr

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
          |  !(self.input.position):begin token("super") args:p
            -> self.i.ast(begin, ['super-send', ['args',p]])
          |  !(self.input.position):begin id:r args:p
            -> self.i.ast(begin, ['send-or-local-call', r, ['args',p]])
          | prim_expr

prim_expr = token("(") expr:e token(")") -> e
          | literal
          | spaces !(self.input.position):begin field_name:x -> self.i.ast(begin, ["field", x])
          | spaces !(self.input.position):begin alpha_name:x -> self.i.ast(begin, ["id", x])

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
             end_body:e -> self.i.ast(begin, ['fun-literal', ["params", p], ["body", body + [e]]])

funliteral_body = !(self.input.position):begin
                  stmts:body
                    expr?:no_semicol_expr !(not no_semicol_expr or body.append(['expression', no_semicol_expr]))
                    !(body.get(-1, [])):last
                    rewrite_last_stmt(last)
                -> body


rewrite_last_stmt = ['expression' :x]:c -> c.__setitem__(0, 'return')
                  | :x

cfunliteral_body = funliteral_body:x spaces ~anything -> self.i.ast(0, x + [['end-body']])

lit_symbol = spaces !(self.input.position):begin token(":") symbol_name:xs
           -> self.i.ast(begin, ["literal-symbol", xs])

lit_number = spaces !(self.input.position):begin (base_10 | base_16):x -> self.i.ast(begin, ["literal-number", x])

base_10 = digit+:ds ~letter -> int(''.join(ds))
base_16 = '0' 'x' (digit | 'A' | 'B' | 'C' | 'D' | 'E' | 'F')+:xs ~letter -> int(''.join(xs), 16)

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
