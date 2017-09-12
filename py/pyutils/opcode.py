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
    "new_context": 5,

    "push_this": 6,
    "push_module": 7,

    "push_bin": 8, # true:1/false:2/null: 0
    "push_fp": 9,

    "push_context": 10,

    "push_argc": 11,
    "load_argv": 12,

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


def encode(op, arg):
    return (op << 24) + arg

def decode(word):
    return ((0xFF000000 & word) >> 24), (0xFFFFFF & word)


class Label(object):
    def __init__(self, bytecodes, pos=None):
        self.bytecodes = bytecodes
        self.start = len(bytecodes)
        self.pos = pos

    def as_current(self):
        self.pos = len(self.bytecodes) - self.start
        return self

    def __call__(self):
        return self.pos

class Bytecodes(object):
    def __init__(self):
        self.lst = []

    def append(self, name, arg):
        if isinstance(arg, numbers.Number):
            self.lst.append(Bytecode(name, lambda: arg))
        elif callable(arg):
            self.lst.append(Bytecode(name, arg))
        else:
            raise Exception("Unsupported arg type")

    def words(self):
        return [x() for x in self.lst]

    def __len__(self):
        return len(self.lst)

    def __iter__(self):
        return iter(self.lst)

    def new_label(self, current=False):
        if current:
            return Label(self, len(self))
        else:
            return Label(self)


class Bytecode(object):
    def __init__(self, name, arg):
        self.name = name
        self.arg = arg

    def __call__(self):
        global opcode_mapping
        for k,v in opcode_mapping.iteritems():
            if k == self.name:
                return encode(v, self.arg())
        raise Exception("Opcode not found for",self.name)
