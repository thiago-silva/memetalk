import numbers
from pdb import set_trace as br

# bytecode
#
# word: 32 bits
#
#  - [op: 8 bits] [args: 24 bits]
#
# Groups:
#
# 1 pushes: 8
# 2 pops: 5
# 3 returns: 6
# 4 calls: 4
# 5 jumps: 3
# 6 exits: 1

WORD_SIZE = 4

opcode_mapping = {
    # push
    # "push_param": 1,
    "push_local": 2,
    "push_literal": 3,
    "push_field": 4,

    "push_this": 6,
    "push_module": 7,

    "push_bin": 8, # true:1/false:2/null: 0
    "push_fp": 9,

    "push_context": 10,

    # pop
    # "pop_param": 20,
    "pop_local": 21,
    "pop_field": 22,
    "pop": 24,
    # ret
    "ret_this":  30,
    "ret_top":  31,
    # "ret_bin":  32,
    # "ret_param":  33,
    # "ret_local":  34,
    # "ret_field":  35,
    # calls
    "send":  40,
    "call":  41,
    "super_send":  42,
    "super_ctor_send":  43,
    #
    # jumps
    "jz": 50,
    "jmp": 51,
    "jmpb": 52,
    # ...
    "exit": 60
    }

def op_name(op):
    return [k for k,v in opcode_mapping.items() if op == v][0]

def encode(op, arg):
    return (op << 24) + arg

def decode(word):
    return ((0xFF000000 & word) >> 24), (0xFFFFFF & word)


class Ref(object):
    def __init__(self, bytecodes, late_pos):
        self.bytecodes = bytecodes
        self.idx = late_pos
        self.target = None

    def __str__(self):
        return str(self())

    def __repr__(self):
        return str(self())

    def __call__(self):
        return self.bytecodes[self.idx].pos()

    def update(self):
        if self.target is None and self.idx < len(self.bytecodes):
            self.target = self.bytecodes[self.idx]

class Label(object):
    def __init__(self, bytecodes):
        self.bytecodes = bytecodes
        self.target = None
        if len(self.bytecodes) > 0:
            self.base = self.bytecodes[-1]
        else:
            self.base = 0

    def base_pos(self):
        if hasattr(self.base, 'pos'):
            return self.base.pos()
        else:
            return self.base()

    # def as_initial(self):
    #     self.pos = 0
    #     return self

    def as_current(self):
        # points to the next instruction to be added to bytecodes
        # [to keep compatibility with the current behavior]
        if len(self.bytecodes) > 0:
            self.target = self.bytecodes[-1]
        else:
            self.target = 0
        return self

    def __call__(self):
        if isinstance(self.target, int):
            return self.target # likey 0
        else:
            return self.target.pos() - self.base_pos()


class Bytecodes(object):
    def __init__(self):
        self.lst = []
        self.refs = []

    def append(self, name, arg):
        if isinstance(arg, numbers.Number):
            self.lst.append(Bytecode(self, name, lambda: arg))
        elif callable(arg):
            self.lst.append(Bytecode(self, name, arg))
        else:
            raise Exception("Unsupported arg type")
        for ref in self.refs:
            ref.update()

    def __getitem__(self, idx):
        return self.lst[idx]

    def __len__(self):
        return len(self.lst)

    def __iter__(self):
        return iter(self.lst)

    def index(self, obj):
        return self.lst.index(obj)

    def words(self):
        return [x() for x in self.lst]

    def new_relative_label(self):
        return Label(self)

    def ref_for_initial(self):
        ref = Ref(self, late_pos=0)
        self.refs.append(ref)
        return ref

    def ref_for_next_pos(self):
        ref = Ref(self, late_pos=len(self))
        self.refs.append(ref)
        return ref


    # def replace(self, pos, other_bytecodes):
    #     # -replace cell's data in [pos] with bytecodes[0]
    #     #   so labels pointing to it are preserved
    #     # -insert into [pos+1] bytecodes[1:]

    #     self.lst[pos].replace(other_bytecodes[0])
    #     self.lst[pos+1:pos+1] = other_bytecodes[1:]


class Bytecode(object):
    def __init__(self, bytecodes, name, arg):
        self.bytecodes = bytecodes
        self.name = name
        self.arg = arg

    # def replace(self, other):
    #     self.name = other.name
    #     self.arg = other.arg

    def pos(self):
        return self.bytecodes.index(self)

    def __call__(self):
        global opcode_mapping
        for k,v in opcode_mapping.iteritems():
            if k == self.name:
                return encode(v, self.arg())
        raise Exception("Opcode not found for",self.name)
