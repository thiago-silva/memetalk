.preamble(ometa_base)
  ometa_base: meme:ometa_base;
  [OMetaBase] <= ometa_base;
.code

escaped: fun(chr) {
  if (chr == "b") { return  "\b"; }
  if (chr == "f") { return  "\f"; }
  if (chr == "n") { return  "\n"; }
  if (chr == "r") { return  "\r"; }
  if (chr == "t") { return  "\t"; }
  if (chr == "v") { return  "\v"; }
  return chr;
}


class OMeta < OMetaBase
fields: current_production, local_vars;
init new: fun(input) {
  super.new(input);
  @local_vars = [];
}


instance_method space: fun() {
  return this._or([fun() {
    this._apply_with_args(:seq, [["/","*"]]);
    this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["*","/"]]);});
        this._apply(:anything);
      }]);}, null);
    this._apply_with_args(:seq, [["*","/"]]);
  }, fun() {
    this._apply_with_args(:seq, [["/","/"]]);
    this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\n"]]);});
        this._apply(:anything);
      }]);}, null);
    this._apply_with_args(:seq, [["\n"]]);
  }, fun() {
    this._apply_super(:space);
  }]);
}
instance_method keyword: fun() {
  var xs = this._apply(:anything);
  return this._or([fun() {
    this._apply_with_args(:token, [xs]);
    this._not(fun() {
      this._apply(:identifier_rest);});
    return xs;
  }]);
}
instance_method char_sequence: fun() {
  var c = null;
  var cs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["'"]);
    cs = this._many1(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["'"]]);});
        this._apply_with_args(:seq, [["\\"]]);
        c = this._apply(:char);
        return escaped(c);
      }, fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["'"]]);});
        this._apply(:char);
      }]);});
    this._apply_with_args(:seq, [["'"]]);
    return [:seq, cs.join("")];
  }]);
}
instance_method token_string: fun() {
  var c = null;
  var cs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["\""]);
    cs = this._many1(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\""]]);});
        this._apply_with_args(:seq, [["\\"]]);
        c = this._apply(:char);
        return escaped(c);
      }, fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\""]]);});
        this._apply(:char);
      }]);});
    this._apply_with_args(:seq, [["\""]]);
    return [:token_string, cs.join("")];
  }]);
}
instance_method keyword_string: fun() {
  var c = null;
  var cs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["``"]);
    cs = this._many1(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["`"]]);});
        this._apply_with_args(:seq, [["\\"]]);
        c = this._apply(:char);
        return escaped(c);
      }, fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["`"]]);});
        this._apply(:char);
      }]);});
    this._apply_with_args(:seq, [["`","`"]]);
    return [:keyword_string, cs.join("")];
  }]);
}
instance_method string_object: fun() {
  var c = null;
  var cs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["`"]);
    cs = this._many1(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["`"]]);});
        this._apply_with_args(:seq, [["\\"]]);
        c = this._apply(:char);
        return escaped(c);
      }, fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["`"]]);});
        this._apply(:char);
      }]);});
    this._apply_with_args(:seq, [["`"]]);
    return [:string_object, cs.join("")];
  }]);
}
instance_method asymbol: fun() {
  var s = null;
  return this._or([fun() {
    this._apply_with_args(:token, [":"]);
    s = this._apply(:identifier);
    return [:symbol, s];
  }]);
}
instance_method s_expr: fun() {
  var s = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["["]);
    s = this._apply(:choice);
    this._apply_with_args(:token, ["]"]);
    return [:form, s];
  }]);
}
instance_method mm_module: fun() {
  var p = null;
  var r = null;
  var e = null;
  return this._or([fun() {
    p = this._apply(:prologue_code);
    r = this._apply(:rules);
    e = this._apply(:epilogue_code);
    return [:module, p, r, e];
  }]);
}
instance_method prologue_code: fun() {
  var x = null;
  return this._or([fun() {
    x = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["<","o","m","e","t","a",">"]]);});
        this._apply(:anything);
      }]);}, null);
    this._apply_with_args(:seq, [["<","o","m","e","t","a",">"]]);
    this._apply(:spaces);
    return x.join("");
  }]);
}
instance_method epilogue_code: fun() {
  var r = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["</ometa>"]);
    r = this._many(fun() {
      this._apply(:anything);}, null);
    return r.join("");
  }]);
}
instance_method ometa: fun() {
  var name = null;
  var i = null;
  var r = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["ometa"]);
    name = this._apply(:identifier);
    i = this._apply(:inheritance);
    this._apply_with_args(:token, ["{"]);
    r = this._apply(:rules);
    this._apply_with_args(:token, ["}"]);
    return [:grammar, name, i, r];
  }]);
}
instance_method inheritance: fun() {
  var i = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["<:"]);
    i = this._apply(:identifier);
    return [:parent, i];
  }, fun() {
    return [:parent,"OMetaBase"];
  }]);
}
instance_method rules: fun() {
  return this._or([fun() {
    this._many1(fun() {
      this._apply(:rule);});
  }]);
}
instance_method rule: fun() {
  var name = null;
  var r = null;
  return this._or([fun() {
    name = this._apply(:identifier);
    @current_production = name;
    r = this._apply_with_args(:rule_rest, [name]);
    this._apply_with_args(:token, [";"]);
    return r;
  }]);
}
instance_method rule_rest: fun() {
  var ac = null;
  var c = null;
  var args = null;
  var name = this._apply(:anything);
  return this._or([fun() {
    ac = this._apply(:action);
    return [:rule, name, ac];
  }, fun() {
    this._apply_with_args(:token, ["="]);
    c = this._apply(:choices);
    return [:rule, name, c];
  }, fun() {
    args = this._many1(fun() {
      this._apply(:binding);});
    ac = this._apply(:action);
    return [:rule, name, [:args] + args, ac];
  }, fun() {
    args = this._many1(fun() {
      this._apply(:binding);});
    this._apply_with_args(:token, ["="]);
    c = this._apply(:choices);
    return [:rule, name, [:args] + args, c];
  }]);
}
instance_method choices: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    x = this._apply(:choice);
    xs = this._many(fun() {
      this._or([fun() {
        this._apply_with_args(:token, ["|"]);
        this._apply(:choice);
      }]);}, null);
    return [:or, x] + xs;
  }]);
}
instance_method choice: fun() {
  var x = null;
  var ac = null;
  return this._or([fun() {
    x = this._many(fun() {
      this._apply(:top_expression);}, null);
    ac = this._apply(:action);
    return [:and] + x + [ac];
  }, fun() {
    x = this._many(fun() {
      this._apply(:top_expression);}, null);
    return [:and] + x;
  }]);
}
instance_method top_expression: fun() {
  return this._or([fun() {
    this._apply(:bind_expression);
  }, fun() {
    this._apply(:repeated_expression);
  }]);
}
instance_method bind_expression: fun() {
  var e = null;
  var b = null;
  return this._or([fun() {
    e = this._apply(:repeated_expression);
    b = this._apply(:binding);
    return [:bind, b, e];
  }]);
}
instance_method repeated_expression: fun() {
  var e = null;
  return this._or([fun() {
    e = this._apply(:term);
    this._apply_with_args(:seq, [["*"]]);
    return [:many, e];
  }, fun() {
    e = this._apply(:term);
    this._apply_with_args(:seq, [["+"]]);
    return [:many1, e];
  }, fun() {
    e = this._apply(:term);
    this._apply_with_args(:seq, [["?"]]);
    return [:optional, e];
  }, fun() {
    this._apply(:term);
  }]);
}
instance_method term: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["~"]);
    e = this._apply(:element);
    return [:not, e];
  }, fun() {
    this._apply_with_args(:token, ["&"]);
    e = this._apply(:element);
    return [:lookahead, e];
  }, fun() {
    this._apply(:element);
  }]);
}
instance_method binding: fun() {
  return this._or([fun() {
    this._apply_with_args(:token, [":"]);
    this._apply(:identifier);
  }]);
}
instance_method element: fun() {
  var s = null;
  var c = null;
  return this._or([fun() {
    this._apply(:prod_app);
  }, fun() {
    this._apply(:data_element);
  }, fun() {
    this._apply_with_args(:token, ["?{"]);
    s = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["}"]]);});
        this._apply(:anything);
      }]);}, null);
    this._apply_with_args(:seq, [["}"]]);
    return [:sem_pred, s.join("")];
  }, fun() {
    this._apply_with_args(:token, ["!{"]);
    s = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["}"]]);});
        this._apply(:anything);
      }]);}, null);
    this._apply_with_args(:seq, [["}"]]);
    return [:sem_action, s.join("")];
  }, fun() {
    this._apply_with_args(:token, ["{"]);
    c = this._apply(:choices);
    this._apply_with_args(:token, ["}"]);
    return c;
  }]);
}
instance_method data_element: fun() {
  return this._or([fun() {
    this._apply(:char_sequence);
  }, fun() {
    this._apply(:token_string);
  }, fun() {
    this._apply(:keyword_string);
  }, fun() {
    this._apply(:string_object);
  }, fun() {
    this._apply(:asymbol);
  }, fun() {
    this._apply(:s_expr);
  }]);
}
instance_method host_str: fun() {
  var c = null;
  var cs = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["\""]);
    cs = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\""]]);});
        this._apply_with_args(:seq, [["\\"]]);
        c = this._apply(:char);
        return "\\" + c;
      }, fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["\""]]);});
        this._apply(:char);
      }]);}, null);
    this._apply_with_args(:seq, [["\""]]);
    return "\"" + cs.join("") +"\"";
  }]);
}
instance_method host_paren: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:seq, [["("]]);
    e = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [[")"]]);});
        this._apply(:host_element);
      }]);}, null);
    this._apply_with_args(:seq, [[")"]]);
    return ["(", e.join(""),")"].join("");
  }]);
}
instance_method host_sq_brk: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:seq, [["["]]);
    e = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["]"]]);});
        this._apply(:host_element);
      }]);}, null);
    this._apply_with_args(:seq, [["]"]]);
    return ["[", e.join(""),"]"].join("");
  }]);
}
instance_method host_c_brk: fun() {
  var e = null;
  return this._or([fun() {
    this._apply_with_args(:seq, [["{"]]);
    e = this._many(fun() {
      this._or([fun() {
        this._not(fun() {
          this._apply_with_args(:seq, [["}"]]);});
        this._apply(:host_element);
      }]);}, null);
    this._apply_with_args(:seq, [["}"]]);
    return ["{", e.join(""),"}"].join("");
  }]);
}
instance_method host_element: fun() {
  return this._or([fun() {
    this._apply(:host_str);
  }, fun() {
    this._apply(:host_paren);
  }, fun() {
    this._apply(:host_sq_brk);
  }, fun() {
    this._apply(:host_c_brk);
  }, fun() {
    this._apply(:anything);
  }]);
}
instance_method host_expr: fun() {
  var x = null;
  return this._or([fun() {
    x = this._many1(fun() {
      this._or([fun() {
        this._not(fun() {
          this._or([fun() {
            this._apply_with_args(:seq, [[";"]]);
          }, fun() {
            this._apply_with_args(:seq, [["}"]]);
          }, fun() {
            this._apply_with_args(:seq, [["|"]]);
          }]);});
        this._apply(:host_element);
      }]);});
    return x.join("").trim();
  }]);
}
instance_method action: fun() {
  var s = null;
  return this._or([fun() {
    this._apply_with_args(:token, ["=>"]);
    s = this._apply(:host_expr);
    return [:action, s];
  }]);
}
instance_method prod_app: fun() {
  var p = null;
  var args = null;
  return this._or([fun() {
    this._apply_with_args(:keyword,["_"]);
    return [:apply, :anything];
  }, fun() {
    this._apply_with_args(:keyword,["$"]);
    return [:apply, :end];
  }, fun() {
    p = this._apply(:identifier);
    this._apply_with_args(:token, ["("]);
    args = this._apply(:prod_args);
    this._apply_with_args(:token, [")"]);
    return [:apply_with_args, args, p.toSymbol];
  }, fun() {
    p = this._apply(:identifier);
    return [:apply, p.toSymbol];
  }, fun() {
    this._apply_with_args(:token, ["^"]);
    return [:apply_super, @current_production];
  }]);
}
instance_method prod_args: fun() {
  var x = null;
  var xs = null;
  return this._or([fun() {
    x = this._apply(:prod_arg);
    xs = this._many(fun() {
      this._or([fun() {
        this._apply_with_args(:token, [","]);
        this._apply(:prod_arg);
      }]);}, null);
    return [x] + xs;
  }]);
}
instance_method prod_arg: fun() {
  var i = null;
  return this._or([fun() {
    this._apply(:data_element);
  }, fun() {
    i = this._apply(:identifier);
    return [:id, i];
  }]);
}


end //OMeta

.endcode
