.preamble(io, memetest)
 io : meme:io;
 memetest : meme:memetest;
 [Test] <= memetest;
.code

class FormatException < Exception
class_method check: fun(pred, message) {
  if (!pred) {
    this.throw(message);
  }
}
end

unpack: fun(str) {
   FormatException.check(str.size == 8, "unpack requires string of length 8");
   var res = 0;
   str.each(fun(idx, chr) {
     res = res + (chr.toByte << (idx * 8));
   });
   return res;
}

pack: fun(num) {
  //TODO: check range of unsigned num (0 < num < 18446744073709551616L)?
  var res = "";
  [0,1,2,3,4,5,6,7].each(fun(_, i) {
    var n = (num >> (i * 8)) & 0xFF;
    res = res + n.asChar;
  });
  return res;
}



// tests

main: fun() {
  var t = Test.new;
  t.assertEqual(unpack("\x08\x07\x06\x05\x04\x03\x02\x01"), 0x102030405060708, "unpack");

  var s = pack(0x102030405060708);
  t.assertEqual(s, "\x08\x07\x06\x05\x04\x03\x02\x01", "pack");
}

.endcode
