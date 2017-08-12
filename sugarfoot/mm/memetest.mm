.license
.endlicense

.preamble(io)
 io: meme:io;
.code


//outside of Exception hierarchy, so
//the catch blocks of exception tests won't suppress it
class AssertionException
  fields: message;
  init new: fun(message) {
    @message = message;
  }
  instance_method message: fun() {
    return @message;
  }
  instance_method throw: fun() {
    <primitive "exception_throw">
  }
  instance_method message: fun() {
    return @message;
  }
  instance_method stack_trace: fun() {
    return @message;
  }
  instance_method toString: fun() {
    return this.message.toString();
  }
  class_method throw: fun(msg) {
    this.new(msg).throw;
  }
end

class Test
fields: current_file, debugger_tests, test_idx;
instance_method test_files: fun() {
  <primitive "test_files">
}

instance_method test_import: fun(filepath, args) {
  <primitive "test_import">
}

instance_method assert: fun(x,desc) {
  if (!x) {
    if (desc) {
      AssertionException.throw("assertion failed: '" + desc + "'");
    } else {
      AssertionException.throw("assertion failed");
    }
  }
}

instance_method assertEqual: fun(x,y, desc) {
  if (x != y) {
    if (!desc) {
      desc = "";
    }
    desc = desc + " " + x.toString + " != " + y.toString;
    AssertionException.throw("assertion failed: " + desc);
  }
}

instance_method assertLocation: fun(proc, expected_mod_fun, expected_loc) {
  var frame = proc.frames[0];
  var cf = frame.cp().compiledFunction();
  var loc = cf.source_location_for(frame);
  this.assertEqual(expected_mod_fun, cf.fullName, null);
  this.assertEqual(loc, expected_loc, null);
}

instance_method do_test: fun(name, f) {
  try {
    io.print("test:executing " + name);
    f();
    io.print("test:passed " + name);
  } catch(AssertionException e) {
    io.print(e.message + " on " + name);
    Exception.throw("test:interrupted");
  }
}

instance_method test_file: fun(mmc_test_file) {
  @current_file = mmc_test_file;
  var m = this.test_import(mmc_test_file, [this]);
  this.do_test(mmc_test_file, fun() {  m.main(); });
}

instance_method start: fun() {
  this.test_files().each(fun(_, mmc_test_file) {
    this.test_file(mmc_test_file)
  });
}

instance_method process_paused: fun() {
  var f = @debugger_tests[@test_idx];
  var name = @current_file + "[step " + @test_idx.toString + "]";
  @test_idx = @test_idx + 1;
  this.do_test(name, f);
  return :resume;
}

instance_method set_debugger_tests: fun(tests) {
  @test_idx = 0;
  @debugger_tests = tests;
}

end

main: fun() {
  Test.new.start();
  return 0;
}
.end
