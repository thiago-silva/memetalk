.license
.endlicense

.preamble()

.code

// -- module functions --

main: fun() {
  return X.throw + X.new(2).x;// == 12, "Testing class method");
}

// -- module classes --

class X
fields: x;
init new: fun(x) {
  @x = x;
}

instance_method x: fun() {
  return @x;
}

class_method throw: fun() {
  return 10;
}

end //class_method1:X


.end
