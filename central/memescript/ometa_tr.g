meme central:memescript/compiler
requires io, ometa_base
where
  io = central:stdlib/io
  ometa_base = central:memescript/ometa_base
import OMetaBase from ometa_base

class OMetaTranslator < OMetaBase
fields: vars, indent;
init new: fun(input) {
  super.new(input);
  @indent = "  ";
  @vars = [];
}
instance_method incr_indent: fun() {
  @indent = @indent + "  ";
}
instance_method decr_indent: fun() {
  @indent = @indent.from(2);
}
instance_method _add_local_var: fun(name) {
  if (!@vars.has(name)) {
    @vars.append(name);
  }
}

instance_method _local_vars: fun() {
  var locals = @vars.map(fun(name) { ["  var ", name, " = null;\n"].join("") });
  @vars = [];
  return locals;
}

<ometa>
  mm_module = [:module string:p rules:r string:e]
            => [p, r.join("\n"), e].join("\n")
            ;

  ometa = [:grammar string:name inheritance:base rules:r]
          => ["class ", name, " < ", base, "\n", r.join("\n"), "\nend"].join("")
         ;

  inheritance = [:parent string:base] => base;

  rules = [rule+:x] => x;

  rule = [:rule _:name rule_args:args body:p]
       => (["instance_method ", name, ": fun() {\n"] + this._local_vars() + [args, p, "\n}"]).join("");

  rule_args = [:args _+:args]
       => args.map(fun(name) { ["  var ", name, " = this._apply(:anything)", ";"].join("") }).join("\n") + "\n"
            | => ""
            ;

   body = ~~[:or _ _+]
            [:or
             !{this.incr_indent()}
               expr+:p ?{p.size > 1}
             !{this.decr_indent()}]
        => [@indent,"return this._or([", p.map(fun(x) { ["fun() {\n", x, "\n", @indent, "}"].join("") }).join(", "), "]);"].join("")
        |  [:or expr:p] => p
        ;

  expr = [:and expr+:p] => p.join("\n")
       | [:and] => ""
       | [:or
           !{this.incr_indent()}
             expr+:p
           !{this.decr_indent()}]
        => [@indent,"this._or([", p.map(fun(x) { ["fun() {\n", x, "\n", @indent, "}"].join("") }).join(", "), "]);"].join("")
       | pattern
       ;

  pattern = [:bind string:name expr:e] !{this._add_local_var(name)}
             => [@indent, name, " = ", e.trim()].join("")
          | [:action _:ac] => [@indent, "return ", ac,";"].join("")
          | expression
          ;

  prod_arg = [:seq _:s] => s
           | [:token_string string:s] => s.toSource()
           | [:string_object string:s] => s.toSource()
           | [:symbol string:s] => (":" + s).toString()
           | [:id string:s] => s
           | [:form expr:s] => s
           ;


  expression = [:apply symbol:s]
               => [@indent, "this._apply(", s.toSource, ");"].join("")
             | [:apply_with_args [prod_arg+:args] _:r]
               => [@indent, "this._apply_with_args(", r.toSource, ", ", args.join(","),");"].join("")
             | [:apply_super string:s]
               => [@indent, "this._apply_super(:", s, ");"].join("")
             | [:seq string:s]
               => [@indent, "this._apply_with_args(:seq, ", s.toSource, ");"].join("")
             | [:token_string string:s]
               => [@indent, "this._apply_with_args(:token, ", s.toSource, ");"].join("")
             | [:many !{this.incr_indent()} expr:b !{this.decr_indent()}]
               => [@indent, "this._many(fun() {\n", b, "}, null);"].join("")
             | [:many1 !{this.incr_indent()} expr:b !{this.decr_indent()}]
               => [@indent, "this._many1(fun() {\n", b, "});"].join("")
             | [:not !{this.incr_indent()} expr:e !{this.decr_indent()}]
               => [@indent, "this._not(fun() {\n", e, "});"].join("")
             | [:optional !{this.incr_indent()} expr:x !{this.decr_indent()}]
               => [@indent, "this._opt(fun() {\n", x, "});"].join("")
             | [:form !{this.incr_indent()} expr:x !{this.decr_indent()}]
               => [@indent, "this._form(fun() {\n", x, "});"].join("")
             | [:symbol string:x]
               => [@indent, "this._apply_with_args(:exactly, :",x,");"].join("")
             | [:string_object string:x]
               => [@indent, "this._apply_with_args(:exactly, ",x.toSource,");"].join("")
             | [:sem_pred string:s] => [@indent, "this._pred(",s, ");"].join("")
             | [:sem_action string:s] => [@indent, s, ";"].join("")
             | [:lookahead !{this.incr_indent()} expr:x !{this.decr_indent()}]
               => [@indent, "this._lookahead(fun() {\n",la,"});"].join("")
             | [:keyword_string string:s]
               => [@indent, "this._apply_with_args(:keyword, ",s.toSource,");"].join("")
             | [:ometa-position] => [@indent, "this._apply(:spaces);\n", @indent, "var _pos = this.input.idx();"].join("")
             | [:ometa-ns-position] => [@indent, @indent, "var _pos = this.input.idx();"].join("")
             ;
</ometa>

end //OMetaTranslator
