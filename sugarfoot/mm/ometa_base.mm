.preamble(io)
  io: meme:io;
.code

class OMetaException
instance_method throw: fun() {
  <primitive "exception_throw">
}
class_method throw: fun() {
  this.new.throw;
}

end


class OMetaStream
fields: memo, idx, hd, tl, data;
init with_data: fun(data) {
  @memo = {};
  @idx = 0;
  @data = data;
  @hd = null;
  @tl = null;
}
init with_head_and_tail: fun(hd, tl) {
  @memo = {};
  @idx = tl.idx;
  @data = tl.data;
  @hd = hd;
  @tl = tl;
}
init from_idx_and_data: fun(idx, data) {
  @memo = {};
  @idx = idx;
  @data = data;
  @hd = null;
  @tl = null;
}
instance_method remaining: fun() {
  return "[" + @data.from(@idx) + "] pos: " + @idx.toString;
}
instance_method data: fun() {
  return @data;
}

instance_method idx: fun() {
  return @idx;
}

instance_method head: fun() {
  if (!@hd) {
    @hd = @data[@idx];
  }
  return @hd;
}

instance_method tail: fun() {
 if (!@tl) {
   var nidx = @idx + 1;
   if (nidx < @data.size) {
     @tl = OMetaStream.from_idx_and_data(nidx, @data);
   } else {
     @tl = OMetaStreamEnd.from_idx_and_data(nidx, @data);
   }
  }
 return @tl;
}

instance_method memo: fun() {
  return @memo;
}

instance_method toString: fun() {
  return  "{" + @data.from(@idx).toString + " pos: " + @idx.toString + "}";
}
end


class OMetaStreamEnd < OMetaStream
fields: memo;
init from_idx_and_data: fun(idx, data) {
  super.from_idx_and_data(idx, data);
}
instance_method head: fun() {
  OMetaException.throw();
}
instance_method tail: fun() {
  OMetaException.throw();
}
end

class OMetaBase
fields: indent, input, error_pos, error_rules;
init new: fun(input) {
  @indent = "";
  @input = input;
  @error_pos = 0;
  @error_rules = [];
}

instance_method set_error: fun(rule, input) {
  if (input.idx > @error_pos) {
    @error_pos = input.idx;
    @error_rules.append(rule);
  } else {
    if (input.idx == @error_pos) {
       @error_rules.append(rule);
     }
  }
}

instance_method input: fun() {
  return @input;
}

instance_method get_error: fun() {
  return [@error_pos, @error_rules];
}
instance_method _apply: fun(rule) {
  var memo = @input.memo;
  if (!memo.has(rule)) {
    var input = @input;
    try {
      var res = this.send(rule, []);
      input.memo[rule] = {:input : @input, :ans : res};
    } catch(OMetaException e) {
      @input = input;
      input.memo[rule] = {:error : e};
      this.set_error(rule.toString, input);
      e.throw();
    }
  } else {
    if (memo[rule].has(:error)) {
      memo[rule][:error].throw();
    }
  }
  @input = memo[rule][:input];
  return memo[rule][:ans];
}
instance_method _apply_with_args: fun(rule, args) {
  args.reverse.each(fun(arg) {
    this.prepend_input(arg);
  });
  try {
    return this._apply(rule);
  } catch(OMetaException e) {
      this.set_error(rule.toString + "(" + args.toString + ")", @input);
      e.throw();
  }
}

instance_method _apply_super: fun(rule) {
  //non-memoized
  return this.super_send(rule, []);
}

instance_method prepend_input: fun(value) {
  @input = OMetaStream.with_head_and_tail(value, @input);
}



//*************************** basic functions


instance_method _pred: fun(x) {
// fails if not x
  if (x) {
     return true;
  } else {
    OMetaException.throw();
  }
}

instance_method _not: fun(fn) {
// returns true if fn() fails. Fails otherwise
  var input = @input;
  try {
    fn();
  } catch(OMetaException e) {
    @input = input;
    return true;
  }
  OMetaException.throw();
}

instance_method _lookahead: fun(fn) {
// runs fn(), restores input when success, returns fn's result
  var input = @input;
  var res = fn();
  @input = input;
  return res;
}

instance_method _or: fun(fns) {
// return result of the first fn() success or fails if no success.
  var input = @input;
  fns.each(fun (fn) {
    try {
      ^ fn();
    } catch(OMetaException e) {
       @input = input;
    }
  });
  OMetaException.throw();
}

instance_method _opt: fun(fn) {
//if fn() fails, restore state, else return fn() result
  var input = @input;
  try {
    return fn();
  } catch(OMetaException e) {
    @input = input;
    return null;
  }
}

instance_method _many: fun(fn, first) {
  var res = null;
  if (first) {
    res = [first];
  } else {
    res = [];
  }
  while (true) {
    var input = @input;
    try {
      res.append(fn());
    } catch(OMetaException e) {
      @input = input;
      return res;
    }
  }
}

instance_method _many1: fun(fn) {
  return this._many(fn, fn());
}

instance_method _repeat: fun(n, fn) {
  var res = [];
  n.times(fun(i) {
    var input = @input;
    try {
      res.append(fn());
    } catch(OMetaException e) {
      @input = input;
      e.throw();
    }
  });
  return res;
}

instance_method _form: fun(fn) {
//very confused about this one
  var r = this._apply(:anything);
  this._pred([String, List].has(Mirror.vtFor(r))); //enumerable?sequenceable?
  var input = @input;
  @input = OMetaStream.with_data(r);
  var res = fn();
  this._apply(:end);
  @input = input;
  return res;
}

// *************************** basic rules

instance_method anything: fun() {
  var head = @input.head();
  @input = @input.tail();
  return head;
}

instance_method end: fun() {
  return this._not(fun() { this._apply(:anything) });
}

instance_method apply: fun() {
  var r = this._apply(:anything);
  return this._apply(r);
}

instance_method exactly: fun() {
  var wanted = this._apply(:anything);
  if (wanted == this._apply(:anything)) {
     return wanted;
  } else {
    OMetaException.throw();
  }
}


instance_method number: fun() {
  var r = this._apply(:anything);
  this._pred(Mirror.vtFor(r) == Number);
  return r;
}

instance_method string: fun() {
  var r = this._apply(:anything);
  this._pred(Mirror.vtFor(r) == String);
  return r;
}

instance_method symbol: fun() {
  var r = this._apply(:anything);
  this._pred(Mirror.vtFor(r) == Symbol);
  return r;
}

instance_method char: fun() {
  var r = this._apply(:anything);
  this._pred(Mirror.vtFor(r) == String and r.size() == 1);
  return r;
}

instance_method space: fun() {
  var r = this._apply(:char);
  this._pred(r.onlySpaces());
  return r;
}

instance_method spaces: fun() {
  return this._many(fun() { this._apply(:space) }, null);
}

instance_method digit: fun() {
  var r = this._apply(:char);
  this._pred(r.onlyDigits());
  return r;
}

instance_method digits: fun() {
  var ds = this._many1(fun() { this._apply(:digit) });
  return ds.join("");
}

instance_method lower: fun() {
  var r = this._apply(:char);
  this._pred(r.isLower());
  return r;
}

instance_method upper: fun() {
  var r = this._apply(:char);
  this._pred(r.isUpper());
  return r;
}

instance_method letter: fun() {
  return this._or([fun() { this._apply(:lower) },
                   fun() { this._apply(:upper) }]);
}

instance_method letter_or_digit: fun() {
  return this._or([fun() { this._apply(:letter) },
                   fun() { this._apply(:digit) }]);
}

instance_method first_and_rest: fun() {
  var first = this._apply(:anything);
  var rest = this._apply(:anything);
  return this._many(fun() { this._apply(rest) }, this._apply(first));
}

instance_method seq: fun() {
  var xs = this._apply(:anything);
  xs.each(fun(x) {
    this._apply_with_args(:exactly, [x]);
  });
  return xs;
}

instance_method token: fun() {
  var tk = this._apply(:anything);
  this._apply(:spaces);
  return this._apply_with_args(:seq, [tk]);
}

instance_method identifier_first: fun() {
  return this._or([fun() { this._apply_with_args(:exactly, ["_"]) },
                   fun() { this._apply(:letter) }]);
}

instance_method identifier_rest: fun() {
  return this._or([fun() { this._apply_with_args(:exactly, ["_"]) },
                   fun() { this._apply(:letter_or_digit) }]);
}

instance_method identifier: fun() {
  this._apply(:spaces);
  var ident = this._apply_with_args(:first_and_rest, [:identifier_first, :identifier_rest]);
  return ident.join("");
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



instance_method until: fun() {
  var brks = this._apply(:anything);
  var r = null;
  var acc = [];
  while (true) {
    r = this._lookahead(fun() { this._apply(:anything) });
    if (brks.find(r) >= 0) {
      return acc;
    }
    acc.append(r);
    this._apply(:anything);
  }
}

end

parse: fun(data, cls, rule) {
  var parser = cls.new(OMetaStream.with_data(data));
  parser.prepend_input(rule);
  try {
    return [null, parser.apply()];
  } catch(OMetaException e) {
    return [parser.get_error, null];
  }
}

.endcode
