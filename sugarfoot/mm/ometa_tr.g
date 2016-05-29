ometa OMetaTranslator {

  ometa = [:grammar string:name inheritance:base rules:r]
          => ["class ", name, " < ", base, "\n", r.join("\n"), "\nend"].join("")
         ;

  inheritance = [:parent string:base] => base;

  rules = [rule+];

  rule = [:rule _:name rule_args:args body:p] => (["instance_method ", name, ": fun() {\n"] + this._local_vars() + [args, p, this._endfun()]).join("");

  rule_args = [:args _+:args]
       => args.map(fun(name) { ["  var ", name, " = this._apply(:anything)", this._semicol].join("") }).join("\n") + "\n"
            | => ""
            ;

  body = [:and body+:p] => p.join(this._semicol + "\n") + this._semicol
       | [:and]
       | [:or
           !{this.incr_indent()}
             body+:p
           !{this.decr_indent()}]
        => [@indent,"return this._or([", p.map(fun(x) { ["fun() {\n", x, @indent, "}"].join("") }).join(",\n" + @indent), "])"].join("") + this._semicol
       | pattern
       ;

  pattern = [:bind string:name body:e] !{this._add_local_var(name)}
             => [@indent, name, " = ", e].join("")
          | [:action _:ac] => [@indent, "return ", ac].join("")
          | expression
          ;

  prod_arg = [:seq _:s] => s
           | [:token_string string:s] => s.toSource()
           | [:string_object string:s] => s.toSource()
           | [:symbol symbol:s] => s.toSource()
           | [:id string:s] => s
           | [:form body:s] => s
           ;


  expression = [:apply symbol:s]
               => [@indent, "this._apply(", s.toSource, ")"].join("")
             | [:apply_with_args [prod_arg+:args] _:r]
               => [@indent, "this._apply_with_args(", r.toSource, ", [", args.join(","),"])"].join("")
             | [:apply_super string:s]
               => [@indent, "this._apply_super(:", s, ")"].join("")
             | [:seq string:s]
               => [@indent, "this._apply_with_args(:seq, [[", s.split("").map(fun(x) { x.toSource }).join(",") , "]])"].join("")
             | [:token_string string:s]
               => [@indent, "this._apply_with_args(:token, [", s.toSource, "])"].join("")
             | [:many !{this.incr_indent()} body:b !{this.decr_indent()}]
               => [@indent, "this._many(fun() {\n", b, "}, null)"].join("")
             | [:many1 !{this.incr_indent()} body:b !{this.decr_indent()}]
               => [@indent, "this._many1(fun() {\n", b, "})"].join("")
             | [:not !{this.incr_indent()} expression:e !{this.decr_indent()}]
               => [@indent, "this._not(fun() {\n", e, "})"].join("")
             | [:optional !{this.incr_indent()} body:x !{this.decr_indent()}]
               => [@indent, "this._opt(fun() {\n", x, "})"].join("")
             | [:form !{this.incr_indent()} body:x !{this.decr_indent()}]
               => [@indent, "this._form(fun() {\n", x, "})"].join("")
             | [:symbol string:x]
               => [@indent, "this._apply_with_args(:exactly, [:",x,"])"].join("")
             | [:string_object string:x]
               => [@indent, "this._apply_with_args(:exactly, [",x.toSource,"])"].join("")
             | [:sem_pred string:s] => [@indent, "this._pred(",s, ")"].join("")
             | [:sem_action string:s] => [@indent, s].join("")
             | [:lookahead !{this.incr_indent()} body:x !{this.decr_indent()}]
               => [@indent, "this._lookahead(fun() {\n",la,"})"].join("")
             ;
}
