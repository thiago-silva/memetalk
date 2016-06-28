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
    "exit": 60,
    # X
    'X_LOAD_AND_STORE_LOCAL_LO': 70,
    'X_LOAD_AND_STORE_LOCAL_LI': 71,
    'X_LOAD_AND_STORE_LOCAL_FI': 72,
    'X_LOAD_AND_STORE_FIELD_LO': 73,
    'X_LOAD_AND_STORE_FIELD_LI': 74,
    'X_LOAD_AND_STORE_FIELD_FI': 75,
    'X_LOAD_AND_RETURN_LOCAL': 76,
    'X_LOAD_AND_RETURN_LITERAL': 77,
    'X_LOAD_AND_RETURN_FIELD': 78,
    'X_LOAD_AND_JZ_LO': 79,
    'X_LOAD_AND_JZ_LI': 80,
    'X_LOAD_AND_JZ_FI': 81,
    'X_SEND_M': 82,
    'X_SEND_LO': 83,
    'X_SEND_FI': 84,
    'X_SEND_LI': 85
    }


def op_name(op):
    return [k for k,v in opcode_mapping.items() if op == v][0]

def encode_x2(op, arg1, arg2):
    return (op << 24) + (arg1 << 8) + arg2

def encode_x3(op, arg1, arg2, arg3):
    return (op << 24) + (arg1 << 16) + (arg2 << 8) + arg3

def encode_x3_args(arg1, arg2, arg3):
    return (arg1 << 16) + (arg2 << 8) + arg3

def decode_xargs3_0(xargs):
    return  (xargs & 0xff0000) >> 16

def decode_xargs3_1(xargs):
    return  (xargs & 0xff00) >> 8

def decode_xargs3_2(xargs):
    return xargs & 0xFF



def encode(op, arg):
    return (op << 24) + arg

def decode(word):
    return ((0xFF000000 & word) >> 24), (0xFFFFFF & word)


class Ref(object):
    def __init__(self, bytecodes, target=None):
        # print 'REF', late_pos
        self.bytecodes = bytecodes
        self.target = target

    def __str__(self):
        return str(self())

    def __repr__(self):
        return str(self())

    def __call__(self):
        return self.target.pos()


class Label(object):
    def __init__(self, bytecodes, base=None):
        self.bytecodes = bytecodes
        self.target = None
        self.base = base

    def as_current(self):
        # print 'LABEL::as_current'
        # points to the next instruction to be added to bytecodes
        # [to keep compatibility with the current behavior]
        self.target = self.bytecodes[-1]
        return self

    def __call__(self):
        return self.target.pos() - self.base.pos()


class Bytecodes(object):
    def __init__(self):
        self.lst = []
        self.idx = 0

    def append(self, name, arg):
        b = self[-1]
        b.set(name, arg)
        self.idx += 1

    def at(self, idx):
        if idx < self.idx:
            return self.lst[idx]
        else:
            raise IndexError

    def __getitem__(self, idx):
        if len(self.lst) == self.idx:
            bytecode = Bytecode(self)
            self.lst.append(bytecode)
        return self.lst[idx]

    def __len__(self):
        return self.idx

    def __iter__(self):
        return iter(self.lst[0:self.idx])

    def index(self, obj):
        return self.lst.index(obj)

    def words(self):
        return [x() for x in self.lst[0:self.idx]]

    # def new_relative_label_for_current_pos(self):
    #     if len(self) > 0:
    #         lb = Label(self, late_base=len(self)-1)
    #     else:
    #         lb = Label(self, late_base=0)
    #     self.refs.append(lb)
    #     return lb

    def label_for_current(self):
        b = self[-1]
        return Label(self, base=b)

    def ref_for_initial(self):
        b = self[0]
        return Ref(self, target=b)

    def ref_for_current(self):
        b = self[-1]
        return Ref(self, target=b)

    def replace(self, target, begin, num,  op_name, arg):
        # -substitute a range of bytecodes for a single bytecode
        #  * target specifies which bytecode is replaced preserving identity
        new_instruction = Bytecode(self, op_name, arg)
        # avoid assigning so refs and labels still refer to valid cells
        self.lst[target].replace(new_instruction)
        # remove substituted instructions
        self.lst[begin:begin+num] = [self.lst[target]]

    def pretty(self):
        return ', '.join(['[{} {}]'.format(op_name(op),arg) for op, arg in self])


class ArgX(object):
    def __init__(self, x=None, y=None, z=None):
        self.x = x or (lambda: 0)
        self.y = y or (lambda: 0)
        self.z = z or (lambda: 0)

    def __call__(self):
        # print 'xargs', self.x(), self.y(), self.z()
        return encode_x3_args(self.x(), self.y(), self.z())

    def update_labels(self, bc):
        if isinstance(self.x, Label):
            self.x.base = bc
        if isinstance(self.y, Label):
            self.y.base = bc
        if isinstance(self.x, Label):
            self.z.base = bc

class Bytecode(object):
    def __init__(self, bytecodes, name=None, arg=None):
        self.bytecodes = bytecodes
        if not (name is None and arg is None):
            self.set(name, arg)

    def set(self, name, arg):
        self.name = name
        if isinstance(arg, numbers.Number):
            self.arg = lambda: arg
        elif callable(arg) or isinstance(arg, ArgX):
            self.arg = arg
        else:
            raise Exception("Unsupported arg type")

    def replace(self, other):
        self.name = other.name
        self.arg = other.arg # other.arg: ArgX
        self.arg.update_labels(self)

    def pos(self):
        return self.bytecodes.index(self)

    def __getitem__(self, idx):
        return [opcode_mapping[self.name], self.arg()][idx]

    def __len__(self):
        return 2

    def __iter__(self):
        return iter([opcode_mapping[self.name], self.arg()])

    def __call__(self):
        global opcode_mapping
        for k,v in opcode_mapping.iteritems():
            if k == self.name:
                return encode(v, self.arg())
        raise Exception("Opcode not found for",self.name)
