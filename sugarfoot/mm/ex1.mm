.preamble()

.code

class OMetaStream
fields: memo, idx, hd, tl, data;
init with_data: fun(data) {
  @memo = {};
  @idx = 0;
  @data = data;
  @hd = null;
  @tl = null;
}

instance_method memo: fun() {
  return @memo;
}
end

main: fun() {
   return 99;
}

.endcode
