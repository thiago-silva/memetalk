.preamble(io, ometa_base)
  io: meme:io;
  ometa_base: meme:ometa_base;
  [OMetaBase] <= ometa_base;
.code

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


instance_method mm_module: fun() {
  var p = null;
  var r = null;
  var e = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:module]);
      p = this._apply(:string);
      r = this._apply(:rules);
      e = this._apply(:string);});
    return [p, r.join("\n"), e].join("\n");
  }]);
}
instance_method ometa: fun() {
  var name = null;
  var base = null;
  var r = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:grammar]);
      name = this._apply(:string);
      base = this._apply(:inheritance);
      r = this._apply(:rules);});
    return ["class ", name," < ", base,"\n", r.join("\n"),"\nend"].join("");
  }]);
}
instance_method inheritance: fun() {
  var base = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:parent]);
      base = this._apply(:string);});
    return base;
  }]);
}
instance_method rules: fun() {
  return this._or([fun() {
    this._form(fun() {
      this._many1(fun() {
        this._apply(:rule);});});
  }]);
}
instance_method rule: fun() {
  var name = null;
  var args = null;
  var p = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:rule]);
      name = this._apply(:anything);
      args = this._apply(:rule_args);
      p = this._apply(:body);});
    return (["instance_method ", name,": fun() {\n"] + this._local_vars() + [args, p,"\n}"]).join("");
  }]);
}
instance_method rule_args: fun() {
  var args = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:args]);
      args = this._many1(fun() {
        this._apply(:anything);});});
    return args.map(fun(name) { ["  var ", name," = this._apply(:anything)",";"].join("") }).join("\n") +"\n";
  }, fun() {
    return "";
  }]);
}
instance_method body: fun() {
  var p = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:or]);
      this.incr_indent();
      p = this._many1(fun() {
        this._apply(:expr);});
      this.decr_indent();});
    return [@indent,"return this._or([", p.map(fun(x) { ["fun() {\n", x,"\n", @indent,"}"].join("") }).join(", "),"]);"].join("");
  }]);
}
instance_method expr: fun() {
  var p = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:and]);
      p = this._many1(fun() {
        this._apply(:expr);});});
    return p.join("\n");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:and]);});
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:or]);
      this.incr_indent();
      p = this._many1(fun() {
        this._apply(:expr);});
      this.decr_indent();});
    return [@indent,"this._or([", p.map(fun(x) { ["fun() {\n", x,"\n", @indent,"}"].join("") }).join(", "),"]);"].join("");
  }, fun() {
    this._apply(:pattern);
  }]);
}
instance_method pattern: fun() {
  var name = null;
  var e = null;
  var ac = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:bind]);
      name = this._apply(:string);
      e = this._apply(:expr);});
    this._add_local_var(name);
    return [@indent, name," = ", e.trim()].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:action]);
      ac = this._apply(:anything);});
    return [@indent,"return ", ac,";"].join("");
  }, fun() {
    this._apply(:expression);
  }]);
}
instance_method prod_arg: fun() {
  var s = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:seq]);
      s = this._apply(:anything);});
    return s;
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:token_string]);
      s = this._apply(:string);});
    return s.toSource();
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:string_object]);
      s = this._apply(:string);});
    return s.toSource();
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:symbol]);
      s = this._apply(:symbol);});
    return s.toSource();
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:id]);
      s = this._apply(:string);});
    return s;
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:form]);
      s = this._apply(:expr);});
    return s;
  }]);
}
instance_method expression: fun() {
  var s = null;
  var args = null;
  var r = null;
  var b = null;
  var e = null;
  var x = null;
  return this._or([fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:apply]);
      s = this._apply(:symbol);});
    return [@indent,"this._apply(", s.toSource,");"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:apply_with_args]);
      this._form(fun() {
        args = this._many1(fun() {
          this._apply(:prod_arg);});});
      r = this._apply(:anything);});
    return [@indent,"this._apply_with_args(", r.toSource,", [", args.join(","),"]);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:apply_super]);
      s = this._apply(:string);});
    return [@indent,"this._apply_super(:", s,");"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:seq]);
      s = this._apply(:string);});
    return [@indent,"this._apply_with_args(:seq, [", s.toSource,"]);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:token_string]);
      s = this._apply(:string);});
    return [@indent,"this._apply_with_args(:token, [", s.toSource,"]);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:many]);
      this.incr_indent();
      b = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._many(fun() {\n", b,"}, null);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:many1]);
      this.incr_indent();
      b = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._many1(fun() {\n", b,"});"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:not]);
      this.incr_indent();
      e = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._not(fun() {\n", e,"});"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:optional]);
      this.incr_indent();
      x = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._opt(fun() {\n", x,"});"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:form]);
      this.incr_indent();
      x = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._form(fun() {\n", x,"});"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:symbol]);
      x = this._apply(:string);});
    return [@indent,"this._apply_with_args(:exactly, [:",x,"]);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:string_object]);
      x = this._apply(:string);});
    return [@indent,"this._apply_with_args(:exactly, [",x.toSource,"]);"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:sem_pred]);
      s = this._apply(:string);});
    return [@indent,"this._pred(",s,");"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:sem_action]);
      s = this._apply(:string);});
    return [@indent, s,";"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:lookahead]);
      this.incr_indent();
      x = this._apply(:expr);
      this.decr_indent();});
    return [@indent,"this._lookahead(fun() {\n",la,"});"].join("");
  }, fun() {
    this._form(fun() {
      this._apply_with_args(:exactly, [:keyword_string]);
      s = this._apply(:string);});
    return [@indent,"this._apply_with_args(:keyword,[",s.toSource,"]);"].join("");
  }]);
}


end //OMetaTranslator

.endcode
