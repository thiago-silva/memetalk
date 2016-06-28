push_local = 2
push_literal = 3
push_field = 4
push_this = 6
push_module = 7
push_bin = 8
push_fp = 9
push_context = 10
pop_local = 21
pop_field = 22
pop = 24
ret_this =  30
ret_top =  31
send =  40
call =  41
super_send =  42
super_ctor_send =  43
jz = 50
jmp = 51
jmpb = 52
exit = 6


start = [opcode+:x] -> x

opcode = [push_local :x] [pop_local :y] -> self.i.emit_x(self,'X_LOAD_AND_STORE_LOCAL_LO', x, y)
       | [push_literal :x] [pop_local :y] -> self.i.emit_x(self, 'X_LOAD_AND_STORE_LOCAL_LI', x, y)
       | [push_field :x] [pop_local :y]:b -> self.i.emit_x(self, 'X_LOAD_AND_STORE_LOCAL_FI', x, y, a=a, b=b)

       | [push_local :x] [pop_field :y] -> self.i.emit_x(self, 'X_LOAD_AND_STORE_FIELD_LO', x, y)
       | [push_literal :x] [pop_field :y] -> self.i.emit_x(self, 'X_LOAD_AND_STORE_FIELD_LI', x, y)
       | [push_field :x] [pop_field :y] -> self.i.emit_x(self, 'X_LOAD_AND_STORE_FIELD_FI', x, y)

       | [push_local :x] [ret_top :y] -> self.i.emit_x(self, 'X_LOAD_AND_RETURN_LOCAL', x)
       | [push_literal :x] [ret_top :y] -> self.i.emit_x(self, 'X_LOAD_AND_RETURN_LITERAL', x)
       | [push_field :x] [ret_top :y] -> self.i.emit_x(self, 'X_LOAD_AND_RETURN_FIELD', x)

       | [push_local :x] [jz :y] -> self.i.emit_x(self, 'X_LOAD_AND_JZ_LO', x, y)
       | [push_literal :x] [jz :y] -> self.i.emit_x(self, 'X_LOAD_AND_JZ_LI', x, y)
       | [push_field :x] [jz :y] -> self.i.emit_x(self, 'X_LOAD_AND_JZ_FI', x, y)

       | [push_module :x] [push_literal :y] [send :z] -> self.i.emit_x(self,'X_SEND_M', x, y, z)
       | [push_local :x] [push_literal :y] [send :z] -> self.i.emit_x(self,'X_SEND_LO', x, y, z)
       | [push_field :x] [push_literal :y] [send :z] -> self.i.emit_x(self,'X_SEND_FI', x, y, z)
       | [push_literal :x] [push_literal :y] [send :z] -> self.i.emit_x(self,'X_SEND_LI', x, y, z)
       | [:x :y] -> [self.i.encode(x, y)]
