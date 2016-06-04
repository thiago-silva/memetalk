.preamble(ometa_base, io)
  ometa_base: meme:ometa_base;
  io : meme:io;
  [OMetaBase] <= ometa_base;
.code

class MemeScriptParser < OMetaBase
init new: fun(input) {
  super.new(input);
}


instance_method alpha: fun() {
  return this._or([fun() {
    this._apply_with_args(:seq, [["+"]])
  }, fun() {
    this._apply_with_args(:seq, [["*"]])
  }, fun() {
    this._apply_with_args(:seq, [["-"]])
  }, fun() {
    this._apply_with_args(:seq, [["/"]])
  }, fun() {
    this._apply_with_args(:seq, [["="]])
  }, fun() {
    this._apply_with_args(:seq, [["<"]])
  }, fun() {
    this._apply_with_args(:seq, [[">"]])
  }, fun() {
    this._apply_with_args(:seq, [["?"]])
  }, fun() {
    this._apply_with_args(:seq, [["!"]])
  }]);
}
instance_method meme_keyword: fun() {
  return this._or([fun() {
    this._apply_with_args(:keyword,["fun"])
  }, fun() {
    this._apply_with_args(:keyword,["var"])
  }]);
}
instance_method id: fun() {
  return this._or([fun() {
    this._apply(:identifier)
  }]);
}
instance_method letter_or_digit_string: fun() {
  return this._or([fun() {
    this._apply(:identifier)
  }]);
}
instance_method alpha_name: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    this._apply(:spaces);
    this._not(fun() {
      this._apply(:meme_keyword)});
    x = return this._or([fun() {
      this._apply(:alpha)
    }, fun() {
      this._apply(:letter)
    }, fun() {
      this._apply_with_args(:seq, [["_"]])
    }]);;
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply(:letterOrDigit)
      }, fun() {
        this._apply_with_args(:seq, [["_"]])
      }, fun() {
        this._apply(:alpha)
      }]);}, null);
    xs.insert(0, x);
    return xs.join("")
  }]);
}
instance_method space: fun() {
  var c = null;
  return this._or([fun() {
    c = this._apply(:anything);
    this._pred(c.onlySpaces);
    return c
  }, fun() {
    this._apply(:comment)
  }]);
}
instance_method comment: fun() {
  return this._or([fun() {
    this._apply_with_args(:seq, [["/","*"]]);
    this._many(fun() {
      return this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["*","/"]])});
        this._apply(:anything)
      }]);}, null);
    this._apply_with_args(:seq, [["*","/"]])
  }, fun() {
    this._apply_with_args(:seq, [["/","/"]]);
    this._many(fun() {
      return this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\n"]])});
        this._apply(:anything)
      }]);}, null);
    this._apply_with_args(:seq, [["\n"]])
  }]);
}
instance_method license: fun() {
  var x = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,[".license"]);
    x = this._many(fun() {
      return this._or([fun() {
        this._not(fun() {
          return this._or([fun() {
            this._apply_with_args(:keyword,[".endlicense"])
          }]);});
        this._apply(:anything)
      }]);}, null);
    this._apply_with_args(:keyword,[".endlicense"]);
    return [:license, x.join("")]
  }, fun() {
    return [:license,""]
  }]);
}
instance_method meta_section: fun() {
  return this._or([fun() {
    this._apply_with_args(:keyword,[".meta"]);
    return [:meta, null]
  }, fun() {
    return [:meta, null]
  }]);
}
instance_method preamble_section: fun() {
  var params = null;
  var entries = null;
  var aliases = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,[".preamble"]);
    params = this._apply(:module_params);
    entries = this._many(fun() {
      this._apply(:preamble_entry)}, null);
    aliases = this._many(fun() {
      this._apply(:module_alias)}, null);
    return [:preamble, params, entries, aliases]
  }]);
}
instance_method code_section: fun() {
  var d = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,[".code"]);
    d = this._many(fun() {
      this._apply(:module_decl)}, null);
    this._apply_with_args(:keyword,[".end"]);
    return [:code, d]
  }]);
}
instance_method start: fun() {
  var lic = null;
  var meta = null;
  var pre = null;
  var code = null;
  return this._or([fun() {
    lic = this._apply(:license);
    meta = this._apply(:meta_section);
    pre = this._apply(:preamble_section);
    code = this._apply(:code_section);
    return [:module, lic, meta, pre, code]
  }]);
}
instance_method module_params: fun() {
  return this._or([fun() {
    this._apply(:params)
  }, fun() {
    return []
  }]);
}
instance_method preamble_entry: fun() {
  var name = null;
  var s = null;
  return this._or([fun() {
    name = this._apply(:id);
    this._apply_with_args(:token, [":"]);
    s = this._apply(:module_spec);
    this._apply_with_args(:token, [";"]);
    return [:param, name, s]
  }]);
}
instance_method module_spec: fun() {
  var ns = null;
  var mname = null;
  return this._or([fun() {
    ns = this._apply(:id);
    this._apply_with_args(:token, [":"]);
    mname = this._apply(:id);
    return [:library, ns, mname]
  }]);
}
instance_method module_alias: fun() {
  var lst = null;
  var x = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["["]);
    lst = this._apply(:idlist);
    this._apply_with_args(:token, ["]"]);
    this._apply_with_args(:token, ["<="]);
    x = this._apply(:id);
    this._apply_with_args(:token, [";"]);
    return [:alias, x, lst]
  }]);
}
instance_method module_decl: fun() {
  return this._or([fun() {
    this._apply(:obj_decl)
  }, fun() {
    this._apply(:class_decl)
  }, fun() {
    this._apply(:top_level_fun)
  }, fun() {
    this._apply(:top_level_fn)
  }]);
}
instance_method obj_decl: fun() {
  var name = null;
  var s = null;
  var f = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["object"]);
    name = this._apply(:id);
    s = this._many1(fun() {
      this._apply(:object_slot)});
    f = this._apply(:obj_fun);
    this._apply_with_args(:keyword,["end"]);
    return [:object, name, s, f]
  }]);
}
instance_method obj_fun: fun() {
  var f = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["functions"]);
    this._apply_with_args(:token, ["{"]);
    f = this._many1(fun() {
      return this._or([fun() {
        this._apply(:constructor)
      }, fun() {
        this._apply(:top_level_fun)
      }]);});
    this._apply_with_args(:token, ["}"]);
    return f
  }, fun() {
    return []
  }]);
}
instance_method object_slot: fun() {
  var name = null;
  var value = null;
  return this._or([fun() {
    name = this._apply(:id);
    this._apply_with_args(:token, [":"]);
    value = return this._or([fun() {
      this._apply(:literal)
    }, fun() {
      this._apply(:id)
    }]);;
    this._apply_with_args(:token, [";"]);
    return [:slot, name, value]
  }]);
}
instance_method class_decl: fun() {
  var name = null;
  var parent = null;
  var f = null;
  var c = null;
  var im = null;
  var cm = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["class"]);
    name = this._apply(:id);
    parent = return this._or([fun() {
      this._apply_with_args(:token, ["<"]);
      this._apply(:id)
    }, fun() {
      this._apply_with_args(:token, ["<"]);
      this._apply_with_args(:token, ["null"])
    }, fun() {
      return "Object"
    }]);;
    f = this._apply(:fields);
    c = this._apply(:constructors);
    im = this._many(fun() {
      this._apply(:instance_method_decl)}, null);
    cm = this._many(fun() {
      this._apply(:class_method_decl)}, null);
    this._apply_with_args(:keyword,["end"]);
    return [:class, [name, parent], f, c, im, cm]
  }]);
}
instance_method fields: fun() {
  var xs = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["fields"]);
    this._apply_with_args(:token, [":"]);
    xs = this._apply(:idlist);
    this._apply_with_args(:token, [";"]);
    return [:fields, xs]
  }, fun() {
    return [:fields, []]
  }]);
}
instance_method constructors: fun() {
  var c = null;
  return this._or([fun() {
    c = this._many(fun() {
      this._apply(:constructor)}, null);
    return [:ctors, c]
  }]);
}
instance_method constructor: fun() {
  var name = null;
  var p = null;
  var body = null;
  return this._or([fun() {
    this.has_fun_literal(false);
    this._apply_with_args(:keyword,["init"]);
    name = this._apply(:alpha_name);
    this._apply_with_args(:token, [":"]);
    this._apply_with_args(:keyword,["fun"]);
    p = this._apply(:fparams);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:top_fun_body);
    this._apply_with_args(:token, ["}"]);
    return [:ctor, name, [:params, p], this.has_fun_literal(),
                  [:body,  body + [[:end-body]]]]
  }]);
}
instance_method top_level_fn: fun() {
  var name = null;
  var e = null;
  return this._or([fun() {
    this._apply(:spaces);
    name = this._apply(:alpha_name);
    this._apply_with_args(:token, [":"]);
    e = this._apply(:expr);
    this._apply_with_args(:token, [";"]);
    return [:fun, name, [:params, []], [:body,  [e]]]
  }]);
}
instance_method top_level_fun: fun() {
  var name = null;
  return this._or([fun() {
    this._apply(:spaces);
    this.has_fun_literal(false);
    name = this._apply(:alpha_name);
    this._apply_with_args(:token, [":"]);
    this._apply(:spaces);
    this._apply_with_args(:fun_rest, [name])
  }]);
}
instance_method fun_rest: fun() {
  var p = null;
  var body = null;
  var name = this._apply(:anything);
  return this._or([fun() {
    this._apply_with_args(:keyword,["fun"]);
    p = this._apply(:fparams);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:top_fun_body);
    this._apply_with_args(:token, ["}"]);
    return [:fun, name, [:params, p], this.has_fun_literal(),
                                              [:body,  body+ [[:end-body]]]]
  }]);
}
instance_method instance_method_decl: fun() {
  var name = null;
  return this._or([fun() {
    this.has_fun_literal(false);
    this._apply_with_args(:keyword,["instance_method"]);
    name = this._apply(:alpha_name);
    this._apply_with_args(:token, [":"]);
    this._apply(:spaces);
    this._apply_with_args(:fun_rest, [name])
  }]);
}
instance_method class_method_decl: fun() {
  var name = null;
  return this._or([fun() {
    this.has_fun_literal(false);
    this._apply_with_args(:keyword,["class_method"]);
    name = this._apply(:alpha_name);
    this._apply_with_args(:token, [":"]);
    this._apply(:spaces);
    this._apply_with_args(:fun_rest, [name])
  }]);
}
instance_method params: fun() {
  var xs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["("]);
    xs = this._apply(:idlist);
    this._apply_with_args(:token, [")"]);
    return xs
  }]);
}
instance_method fparams: fun() {
  var x = null;
  var xs = null;
  var y = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["("]);
    this._apply_with_args(:token, [")"]);
    return []
  }, fun() {
    this._apply_with_args(:token, ["("]);
    this._apply_with_args(:token, ["*"]);
    x = this._apply(:id);
    this._apply_with_args(:token, [")"]);
    return [[:var-arg, x]]
  }, fun() {
    this._apply_with_args(:token, ["("]);
    x = this._apply(:id);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:id)
      }]);}, null);
    this._apply_with_args(:token, [")"]);
    return [x]+xs
  }, fun() {
    this._apply_with_args(:token, ["("]);
    x = this._apply(:id);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:id)
      }]);}, null);
    y = this._apply(:pvar);
    this._apply_with_args(:token, [")"]);
    return [x]+xs+[y]
  }]);
}
instance_method pvar: fun() {
  var x = null;
  return this._or([fun() {
    this._apply_with_args(:token, [","]);
    this._apply_with_args(:token, ["*"]);
    x = this._apply(:id);
    return [:var-arg, x]
  }]);
}
instance_method idlist: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    x = this._apply(:id);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:id)
      }]);}, null);
    return [x]+xs
  }, fun() {
    return []
  }]);
}
instance_method top_fun_body: fun() {
  return this._or([fun() {
    this._apply(:primitive)
  }, fun() {
    this._apply(:stmts)
  }]);
}
instance_method primitive: fun() {
  var s = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["<"]);
    this._apply_with_args(:keyword,["primiteive"]);
    s = this._apply(:lit_string);
    this._apply_with_args(:token, [">"]);
    return [[:primitive, s]]
  }]);
}
instance_method stmts: fun() {
  var x = null;
  return this._or([fun() {
    x = this._many(fun() {
      this._apply(:stmt)}, null);
    return x
  }]);
}
instance_method stmt: fun() {
  var e = null;
  return this._or([fun() {
    this._apply(:control_expr)
  }, fun() {
    e = this._apply(:non_control_expr);
    this._apply_with_args(:token, [";"]);
    return e
  }]);
}
instance_method non_control_expr: fun() {
  var e = null;
  return this._or([fun() {
    this._apply(:expr_ret)
  }, fun() {
    this._apply(:expr_non_local_ret)
  }, fun() {
    this._apply(:expr_attr)
  }, fun() {
    e = this._apply(:expr);
    return [:expression, e]
  }, fun() {
    this._apply(:expr_decl)
  }]);
}
instance_method expr_ret: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["return"]);
    e = this._apply(:expr);
    return [:return, e]
  }]);
}
instance_method expr_non_local_ret: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["^"]);
    e = this._apply(:expr);
    return [:non-local-return, e]
  }]);
}
instance_method expr_decl: fun() {
  var name = null;
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["var"]);
    name = this._apply(:id);
    this._apply_with_args(:token, ["="]);
    e = this._apply(:expr);
    return [:var-def, name, e]
  }]);
}
instance_method expr_attr: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    this._apply(:spaces);
    a = this._apply(:lhs);
    this._apply_with_args(:token, ["="]);
    b = this._apply(:expr);
    return [:=, a, b]
  }]);
}
instance_method lhs: fun() {
  var r = null;
  var x = null;
  return this._or([fun() {
    r = this._apply(:expr);
    this._pred(len(r)>0 and r[0] == 'index');
    return r
  }, fun() {
    x = this._apply(:alpha_name);
    return [:id, x]
  }, fun() {
    x = this._apply(:field_name);
    return [:field, x]
  }]);
}
instance_method control_expr: fun() {
  return this._or([fun() {
    this._apply(:expr_if)
  }, fun() {
    this._apply(:expr_while)
  }, fun() {
    this._apply(:expr_try)
  }]);
}
instance_method expr_if: fun() {
  var e = null;
  var body = null;
  var elif_ = null;
  var else_ = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["if"]);
    this._apply_with_args(:token, ["("]);
    e = this._apply(:expr);
    this._apply_with_args(:token, [")"]);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    elif_ = this._many(fun() {
      this._apply(:expr_elif)}, null);
    else_ = this._opt(fun() {
      this._apply(:expr_else)});
    return [:if, e, body, elif_, else_ or []])
  }]);
}
instance_method expr_elif: fun() {
  var e = null;
  var body = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["elif"]);
    this._apply_with_args(:token, ["("]);
    e = this._apply(:expr);
    this._apply_with_args(:token, [")"]);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    return [:elif, e, body]
  }]);
}
instance_method expr_else: fun() {
  var body = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["else"]);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    return body
  }]);
}
instance_method expr_while: fun() {
  var e = null;
  var xs = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["while"]);
    this._apply_with_args(:token, ["("]);
    e = this._apply(:expr);
    this._apply_with_args(:token, [")"]);
    this._apply_with_args(:token, ["{"]);
    xs = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    return [:while e, xs]
  }]);
}
instance_method expr_try: fun() {
  var s_try = null;
  var c = null;
  var s_catch = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["try"]);
    this._apply_with_args(:token, ["{"]);
    s_try = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    c = this._apply(:catch_part);
    this._apply_with_args(:token, ["{"]);
    s_catch = this._apply(:stmts);
    this._apply_with_args(:token, ["}"]);
    return [:try, s_try, c, s_catch]
  }]);
}
instance_method catch_part: fun() {
  var id = null;
  var t = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["catch"]);
    this._apply_with_args(:token, ["("]);
    id = this._apply(:alpha_name);
    this._apply_with_args(:token, [")"]);
    return [:catch, id]
  }, fun() {
    this._apply_with_args(:keyword,["catch"]);
    this._apply_with_args(:token, ["("]);
    t = this._apply(:catch_type);
    id = this._apply(:alpha_name);
    this._apply_with_args(:token, [")"]);
    return [:catch, t, id]
  }]);
}
instance_method catch_type: fun() {
  var type = null;
  return this._or([fun() {
    type = this._apply(:alpha_name);
    return [:id, type]
  }]);
}
instance_method expr: fun() {
  return this._or([fun() {
    this._apply(:spaces);
    this._apply(:expr_or)
  }]);
}
instance_method expr_or: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_or);
    this._apply_with_args(:keyword,["or"]);
    b = this._apply(:expr_and);
    return [:or a, b]
  }, fun() {
    this._apply(:expr_and)
  }]);
}
instance_method expr_and: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_and);
    this._apply_with_args(:keyword,["and"]);
    b = this._apply(:expr_eq);
    return [:and a, b]
  }, fun() {
    this._apply(:expr_eq)
  }]);
}
instance_method expr_eq: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_eq);
    this._apply_with_args(:token, ["=="]);
    b = this._apply(:expr_rel);
    return [:==, a, b]
  }, fun() {
    a = this._apply(:expr_eq);
    this._apply_with_args(:token, ["!="]);
    b = this._apply(:expr_rel);
    return [:!=, a, b]
  }, fun() {
    this._apply(:expr_rel)
  }]);
}
instance_method expr_rel: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_rel);
    this._apply_with_args(:token, [">="]);
    b = this._apply(:expr_add);
    return [:>=, a, b]
  }, fun() {
    a = this._apply(:expr_rel);
    this._apply_with_args(:token, [">"]);
    b = this._apply(:expr_add);
    return [:>, a, b]
  }, fun() {
    a = this._apply(:expr_rel);
    this._apply_with_args(:token, ["<="]);
    b = this._apply(:expr_add);
    return [:<=, a, b]
  }, fun() {
    a = this._apply(:expr_rel);
    this._apply_with_args(:token, ["<"]);
    b = this._apply(:expr_add);
    return [:<, a, b]
  }, fun() {
    this._apply(:expr_add)
  }]);
}
instance_method expr_add: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_add);
    this._apply_with_args(:token, ["++"]);
    b = this._apply(:expr_mul);
    return [:++, a, b]
  }, fun() {
    a = this._apply(:expr_add);
    this._apply_with_args(:token, ["+"]);
    b = this._apply(:expr_mul);
    return [:+, a, b]
  }, fun() {
    a = this._apply(:expr_add);
    this._apply_with_args(:token, ["-"]);
    b = this._apply(:expr_mul);
    return [:-, a, b]
  }, fun() {
    this._apply(:expr_mul)
  }]);
}
instance_method expr_mul: fun() {
  var a = null;
  var b = null;
  return this._or([fun() {
    a = this._apply(:expr_mul);
    this._apply_with_args(:token, ["*"]);
    b = this._apply(:expr_unary);
    return [:*, a, b]
  }, fun() {
    a = this._apply(:expr_mul);
    this._apply_with_args(:token, ["/"]);
    b = this._apply(:expr_unary);
    return [:/, a, b]
  }, fun() {
    this._apply(:expr_unary)
  }]);
}
instance_method expr_unary: fun() {
  var a = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["+"]);
    a = this._apply(:prim_expr);
    return [:positive, a]
  }, fun() {
    this._apply_with_args(:token, ["-"]);
    a = this._apply(:prim_expr);
    return [:negative, a]
  }, fun() {
    this._apply_with_args(:token, ["!"]);
    a = this._apply(:expr_unary);
    return [:not, a]
  }, fun() {
    this._apply_with_args(:token, ["~"]);
    a = this._apply(:expr_unary);
    return [:bit-neg, a]
  }, fun() {
    this._apply(:spaces);
    this._apply(:suffix_expr)
  }]);
}
instance_method suffix_expr: fun() {
  var sel = null;
  var p = null;
  var r = null;
  var i = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["super"]);
    this._apply_with_args(:token, ["."]);
    sel = this._apply(:alpha_name);
    p = this._apply(:args);
    return [:super-ctor-send, sel, [:args, p]]
  }, fun() {
    r = this._apply(:suffix_expr);
    this._apply_with_args(:token, ["."]);
    sel = this._apply(:alpha_name);
    p = this._apply(:args);
    return [:send, r, sel, [:args, p]]
  }, fun() {
    r = this._apply(:suffix_expr);
    this._apply_with_args(:token, ["."]);
    sel = this._apply(:alpha_name);
    return [:send, r, sel, [:args, []]]
  }, fun() {
    r = this._apply(:suffix_expr);
    this._apply_with_args(:token, ["["]);
    i = this._apply(:expr);
    this._apply_with_args(:token, ["]"]);
    return [:index, r, i]
  }, fun() {
    this._apply(:call_expr)
  }]);
}
instance_method call_expr: fun() {
  var r = null;
  var p = null;
  return this._or([fun() {
    r = this._apply(:call_expr);
    p = this._apply(:args);
    return [:call, r, [:args, p]]
  }, fun() {
    this._apply_with_args(:keyword,["super"]);
    p = this._apply(:args);
    return [:super-send, [:args,p]]
  }, fun() {
    this._apply(:spaces);
    r = this._apply(:id);
    p = this._apply(:args);
    return [:send-or-local-call, r, [:args,p]]
  }, fun() {
    this._apply(:prim_expr)
  }]);
}
instance_method prim_expr: fun() {
  var e = null;
  var x = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["("]);
    e = this._apply(:expr);
    this._apply_with_args(:token, [")"]);
    return e
  }, fun() {
    this._apply(:literal)
  }, fun() {
    this._apply(:spaces);
    x = this._apply(:field_name);
    return [:field, x]
  }, fun() {
    this._apply(:spaces);
    x = this._apply(:alpha_name);
    return [:id, x]
  }]);
}
instance_method pair_list: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    x = this._apply(:pair);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:pair)
      }]);}, null);
    return [x]+xs
  }, fun() {
    return []
  }]);
}
instance_method pair: fun() {
  var key = null;
  var val = null;
  return this._or([fun() {
    key = this._apply(:expr);
    this._apply_with_args(:token, [":"]);
    val = this._apply(:expr);
    return [:pair, key, val]
  }]);
}
instance_method args: fun() {
  var p = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["("]);
    p = this._apply(:expr_list);
    this._apply_with_args(:token, [")"]);
    return p
  }]);
}
instance_method expr_list: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    x = this._apply(:expr);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:expr)
      }]);}, null);
    return [x]+xs
  }, fun() {
    return []
  }]);
}
instance_method literal: fun() {
  var e = null;
  var x = null;
  return this._or([fun() {
    this._apply(:lit_number)
  }, fun() {
    this._apply(:lit_string)
  }, fun() {
    this._apply(:lit_symbol)
  }, fun() {
    this._apply_with_args(:token, ["["]);
    e = this._apply(:expr_list);
    this._apply_with_args(:token, ["]"]);
    return [:literal-array][e]
  }, fun() {
    this._apply_with_args(:token, ["{"]);
    e = this._apply(:pair_list);
    this._apply_with_args(:token, ["}"]);
    return [:literal-dict]+e
  }, fun() {
    this._apply_with_args(:keyword,["thisModule"]);
    return [:literal, :module]
  }, fun() {
    this._apply_with_args(:keyword,["thisContext"]);
    return [:literal, :context])
  }, fun() {
    this._apply_with_args(:keyword,["this"]);
    return [:literal, :this])
  }, fun() {
    this._apply_with_args(:keyword,["null"]);
    return [:literal, :null]
  }, fun() {
    this._apply_with_args(:keyword,["true"]);
    return [:literal, :true])
  }, fun() {
    this._apply_with_args(:keyword,["false"]);
    return [:literal, :false]
  }, fun() {
    x = this._apply(:funliteral);
    this.has_fun_literal(true);
    return x
  }]);
}
instance_method funliteral: fun() {
  var p = null;
  var body = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["fun"]);
    p = this._apply(:params);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:funliteral_body);
    this._apply_with_args(:token, ["}"]);
    return [:fun-literal, [:params, p], [:body, body]]
  }]);
}
instance_method funliteral_body: fun() {
  var body = null;
  var no_semicol_expr = null;
  var last = null;
  return this._or([fun() {
    body = this._apply(:stmts);
    no_semicol_expr = this._opt(fun() {
      this._apply(:expr)});
    !no_semicol_expr or body.append([:expression, no_semicol_expr]);
    last = body.get(-1, []);
    this._apply_with_args(:rewrite_last_stmt, [last]);
    return body + [[:return-null]]
  }]);
}
instance_method rewrite_last_stmt: fun() {
  var x = null;
  var c = null;
  return this._or([fun() {
    c = this._form(fun() {
      x = this._apply_with_args(:exactly, [:expression])});
    return c.__setitem__(0, :return)
  }, fun() {
    this._apply_with_args(:exactly, [:x])
  }]);
}
instance_method cfunliteral_body: fun() {
  var x = null;
  return this._or([fun() {
    x = this._apply(:funliteral_body);
    this._apply(:spaces);
    this._not(fun() {
      this._apply(:anything)});
    return x
  }]);
}
instance_method lit_symbol: fun() {
  var xs = null;
  return this._or([fun() {
    this._apply_with_args(:token, [":"]);
    xs = this._apply(:alpha_name);
    return [:literal-symbol, xs]
  }]);
}
instance_method lit_number: fun() {
  var ds = null;
  return this._or([fun() {
    this._apply(:spaces);
    ds = this._many1(fun() {
      this._apply(:digit)});
    return [:literal-number, int(ds.join("").parseInt)]
  }]);
}
instance_method lit_string: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    this._apply_with_args(:seq, [["\""]]);
    xs = this._many(fun() {
      return this._or([fun() {
        this._apply(:lit_escaped)
      }, fun() {
        x = this._not(fun() {
          this._apply_with_args(:seq, [["\""]])})
      }]);}, null);
    this._apply_with_args(:seq, [["\""]]);
    return [:literal-string, xs.join("").decodeEscape]
  }]);
}
instance_method lit_escaped: fun() {
  var x = null;
  return this._or([fun() {
    this._not(fun() {
      this._apply_with_args(:seq, [["\""]])});
    x = this._apply_with_args(:seq, [["\\"]]);
    return "\\" + x
  }]);
}
instance_method field_name: fun() {
  var x = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["@"]);
    x = this._apply(:letter_or_digit_string);
    return x
  }]);
}
instance_method single_top_level_fun: fun() {
  var p = null;
  var body = null;
  var name = this._apply(:anything);
  return this._or([fun() {
    this._apply_with_args(:keyword,["fun"]);
    p = this._apply(:fparams);
    this._apply_with_args(:token, ["{"]);
    body = this._apply(:top_fun_body);
    this._apply_with_args(:token, ["}"]);
    return [:fun, name, [:params, p],
                              [:body, body+ [:end-body]]]
  }]);
}


end //OMeta

.endcode
