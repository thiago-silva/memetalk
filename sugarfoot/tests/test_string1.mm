.license
.endlicense

.preamble(test, io)
 io : meme:io;

.code

main: fun() {
  test.assertEqual("abc" + "def", "abcdef", "concat");
  test.assertEqual("10a".toInteger, 10, "toInteger");
  test.assertEqual("a".toByte, 97, "toByte");

  test.assertEqual("abc" == "abc", true, "equal");
  test.assertEqual("abc" == "abce", false, "different");
  test.assertEqual("\0ab\t".size, 4, "size");
  test.assertEqual("".size, 0, "empty size");

  var res = 5.asChar + 9.asChar;
  test.assertEqual(res.size, 2, "binary data size");
  test.assertEqual("\x03".size, 1, "binary content");
  test.assertEqual("\x05\x09", res, "binary content 2");

  var r = "";
  r = r + 1.asChar;
  r = r + 2.asChar;
  r = r + 3.asChar;
  test.assertEqual("\x01\x02\x03", r, "binary content 3");

  var r2 = "";
  r2 = r2 + 0.asChar;
  r2 = r2 + 1.asChar;
  r2 = r2 + 2.asChar;
  test.assertEqual("\x00\x01\x02", r2, "binary content 3");
}

.endcode
