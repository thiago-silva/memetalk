.preamble(ometa,ometa_tr,ometa_base, io)
 ometa : meme:ometa;
 ometa_tr : meme:ometa_tr;
 ometa_base : meme:ometa_base;
 io : meme:io;
.code

gen: fun(grammar_in_file_path, grammar_out_file_path, OMeta, OMetaTranslator) {
  io.print("=== processing " + grammar_in_file_path);
  var text = io.read_file(grammar_in_file_path);
  var maybe_ast = ometa_base.parse(text, OMeta, :mm_module);
  //io.print(maybe_ast.toSource);
  if (maybe_ast[0]) {
    io.print("parse error: " + maybe_ast[0].toString);
  } else {
    io.print("\n\ntranslating...\n\n");
    maybe_ast = ometa_base.parse([maybe_ast[1]], OMetaTranslator, :mm_module);
    if (maybe_ast[0]) {
      io.print("translation error: " + maybe_ast[0].toString);
    } else {
      io.write_file(grammar_out_file_path, maybe_ast[1]);
      io.print("Wrote to " + grammar_out_file_path);
    }
  }
}

/*
clock_t start = clock() ;
do_some_work() ;
clock_t end = clock() ;
double elapsed_time = (end-start)/(double)CLOCKS_PER_SEC ;
*/

start_time: fun() {
  <primitive "start_time">
}
end_time: fun() {
  <primitive "end_time">
}

main: fun() {
  //warm it
  gen("mm/ometa.g", "mm/ometa1.mm", ometa.OMeta, ometa_tr.OMetaTranslator);
  gen("mm/ometa_tr.g", "mm/ometa_tr1.mm", ometa.OMeta, ometa_tr.OMetaTranslator);

  //go
  start_time();
  gen("mm/ometa.g", "mm/ometa1.mm", ometa.OMeta, ometa_tr.OMetaTranslator);
  gen("mm/ometa_tr.g", "mm/ometa_tr1.mm", ometa.OMeta, ometa_tr.OMetaTranslator);
  end_time();
}

.endcode
