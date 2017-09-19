from . import bits
from . import opcode
from . import (OrderedDict,
               FRAME_TYPE_OBJECT,
               FRAME_TYPE_LITERAL_FRAME,
               FRAME_TYPE_BYTECODE_FRAME,
               FRAME_TYPE_EXCEPTIONS_FRAME)

from . import (behavior_label,
               cclass_label,
               class_label,
               cfun_label,
               fun_label,
               cmod_label,
               mod_label,
               closure_name)

from pdb import set_trace as br

    #!(fnobj.current_bytecode_pos()):bpos or fnobj.update_line_mapping(bpos, ast)
def emitter(fn):
    def wrapper(self, *args):
        ast = args[0]
        bpos = self.current_bytecode_pos()
        res = fn(self, *args)
        self.update_line_mapping(bpos, ast)
        return res
    return wrapper

class Entry(object):
    def __init__(self):
        self.oop = None

        self._label_counter = 0
    # def get_oop(self, vmem, dont_load=True):
    #     if self.oop is None:
    #         if dont_load:
    #             raise Exception('get_oop called prior to having an oop')
    #         self.oop = self.fill(vmem)
    #     return self.oop
    def label_counter(self):
        ret = self._label_counter
        self._label_counter += 1
        return ret

class Object(Entry):
    def __init__(self, name, imod):
        super(Object, self).__init__()
        self.name = name
        self.slots = []
        self.imod = imod

    def label(self):
        return self.name

    def add_slot_literal_array(self, name, value):
        if len(value) > 0:
            raise Exception('todo')
        self.add_slot_empty_list(name)

    def add_slot_literal_dict(self, name, value):
        if len(value) > 0:
            raise Exception('todo')
        self.add_slot_empty_dict(name)

    def add_slot_ref(self, name, value):
        self.slots.append({'type': 'ref', 'name': name, 'value': value})

    def add_slot_literal_string(self, name, value):
        self.slots.append({'type': 'string', 'name': name, 'value': value})

    def add_slot_empty_dict(self, name):
        # TODO: this will become dict
        self.slots.append({'type': 'empty_dict', 'name': name})

    def add_slot_empty_list(self, name):
        # TODO: this will become list
        self.slots.append({'type': 'empty_list', 'name': name})

    def add_slot_literal_null(self, name):
        self.slots.append({'type': 'null', 'name': name})

    def add_slot_literal_num(self, name, value):
        self.slots.append({'type': 'int', 'name': name, 'value': value})

    def fill(self, vmem):
        # synth necessary objects:
        refs_to_literals = {}
        for idx, slot in enumerate(self.slots[1:]):
            if slot['type'] == 'string':
                refs_to_literals[idx] = vmem.append_string_instance(slot['value'])
            elif slot['type'] == 'empty_list':
                refs_to_literals[idx] = vmem.append_empty_list()
            elif slot['type'] == 'empty_dict':
                refs_to_literals[idx] = vmem.append_empty_dict()
            elif slot['type'] == 'dict':
                fun_dict = {}
                for name, cfun in slot['value'].items():
                    cfun.fill(vmem)
                    fun_dict[name] = Function(self.imod, cfun)
                refs_to_literals[idx] = vmem.append_sym_dict_emiting_entries(fun_dict)
        # emit our object

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(len(self.slots) * bits.WSIZE)

        oop = vmem.append_label_ref(self.slots[0]['value'], self.name)

        for idx, slot in enumerate(self.slots[1:]):
            if slot['type'] == 'ref':
                vmem.append_label_ref(slot['value'])
            elif slot['type'] == 'null':
                vmem.append_null()
            elif slot['type'] == 'string':
                vmem.append_pointer_to(refs_to_literals[idx])
            elif slot['type'] == 'empty_list':
                vmem.append_pointer_to(refs_to_literals[idx])
            elif slot['type'] == 'empty_dict':
                vmem.append_pointer_to(refs_to_literals[idx])
            elif slot['type'] == 'dict':
                vmem.append_pointer_to(refs_to_literals[idx])
            elif slot['type'] == 'int':
                vmem.append_tagged_int(slot['value'])
            else:
                raise Exception('TODO')
        self.oop = oop
        return oop

## these need to know the CompiledFunction owner (which s Object_CompiledClass)

class Object_ObjectBehavior(Object):
    def new_ctor(self, name):
        owner = self.imod.object_by_name('Object_CompiledClass')
        d = [s for s in self.slots if s['type'] == 'dict']
        if len(d) == 0:
            dslot = {"type":"dict", name: "dict", 'value':{}}
            self.slots.insert(2,dslot) #dict is slot in idx==2
        else:
            dslot = d[0]

        fn = CompiledFunction(self.imod.cmod, owner, name, ctor=True)
        dslot['value'][name] = fn
        return fn

    def new_function(self, name, params):
        owner = self.imod.object_by_name('Object_CompiledClass')
        d = [s for s in self.slots if s['type'] == 'dict']
        if len(d) == 0:
            dslot = {"type":"dict", name: "dict", 'value':{}}
            self.slots.insert(2,dslot) #dict is slot in idx==2
        else:
            dslot = d[0]

        fn = CompiledFunction(self.imod.cmod, owner, name, params)
        dslot['value'][name] = fn
        return fn

class Object_Object(Object):
    def new_function(self, name, params):
        owner = self.imod.object_by_name('Object_CompiledClass')
        d = [s for s in self.slots if s['type'] == 'dict']
        if len(d) == 0:
            dslot = {"type":"dict", name: "dict", 'value':{}}
            self.slots.insert(2,dslot) #dict is slot in idx==2
        else:
            dslot = d[0]

        fn = CompiledFunction(self.imod.cmod, self, name, params)
        dslot['value'][name] = fn
        return fn

class Behavior(Entry):
    def __init__(self, name, parent_name): #, dictionary):
        super(Behavior, self).__init__()
        self.name = name
        self.parent_name = parent_name
        self.parent_label = behavior_label(parent_name)
        self.dictionary = {}
        # if parent_name != 'Object':
        #     br()
        #     raise Exception('TODO')

    def label(self):
        return behavior_label(self.name)

    def fill(self, vmem):
        oop_dict = vmem.append_sym_dict_emiting_entries(self.dictionary)

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(4 * bits.WSIZE)

        oop = vmem.append_label_ref('Behavior', self.label())       # vt
        vmem.append_label_ref(self.parent_label)                  # delegate
        vmem.append_pointer_to(oop_dict)                          # dict: "own methods"
        vmem.append_int(256)                                      # size: 256 = dummy flag: "I am a behavior. no more fields after this"
        # no compiled_class
        self.oop = oop
        return oop


class Class(Entry):
    def __init__(self, imod, cclass):
        super(Class, self).__init__()
        self.imod = imod
        self.behavior = Behavior(cclass.name, cclass.super_name)
        self.cclass = cclass
        self.dictionary = {}

    def label(self):
        return class_label(self.cclass.name)

    def new_instance_method(self, name):
        cfun = self.cclass.new_instance_method(name)
        fun = Function(self.imod, cfun)
        self.dictionary[name] = fun
        return fun

    def new_class_method(self, name):
        cfun = self.cclass.new_class_method(name)
        fun = Function(self.imod, cfun)
        self.behavior.dictionary[name] = fun
        return fun

    def new_ctor(self, name):
        cfun = self.cclass.new_ctor(name)
        fun = Function(self.imod, cfun)
        self.behavior.dictionary[name] = fun
        return fun

    def fill(self, vmem):
        oop_vt = self.behavior.fill(vmem)

        oop_dict = vmem.append_sym_dict_emiting_entries(self.dictionary)

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(5 * bits.WSIZE)

        # oop = vmem.append_label_ref(behavior_label(self.cclass.name),  self.label())  # vt
        oop = vmem.append_pointer_to(oop_vt, self.label()) # vt
        vmem.append_label_ref(class_label(self.cclass.super_name))                    # delegate
        vmem.append_pointer_to(oop_dict)                                              # dict: "methods"
        vmem.append_int(len(self.cclass.fields))                                      # payload
        vmem.append_label_ref(cclass_label(self.cclass.name))                       # compiled_class
        # vmem.append_pointer_to(self.cclass.oop)                                   # <-
        self.oop = oop
        return oop



class CompiledClass(Entry):
    def __init__(self, cmod, name, super_name, fields):
        super(CompiledClass, self).__init__()
        self.cmod = cmod
        self.name = name
        self.super_name = super_name
        self.fields = fields
        self.instance_methods = {}
        self.class_methods = {}

    def label(self):
        return cclass_label(self.name) #cmod.label() + '_' + self.name + "_CompiledClass"

    def new_ctor(self, name):
        fn = CompiledFunction(self.cmod, self, name, ctor=True)
        self.class_methods[name] = fn
        return fn

    def new_instance_method(self, name):
        fn = CompiledFunction(self.cmod, self, name)
        self.instance_methods[name] = fn
        return fn

    def new_class_method(self, name):
        fn = CompiledFunction(self.cmod, self, name)
        self.class_methods[name] = fn
        return fn

    def fill(self, vmem):
        # vt: CompiledClass
        # delegate: ...
        # module: ...
        # name: ...
        # super_class_name: ...
        # fields
        # methods
        # class_methods ...

        delegate = vmem.append_object_instance()
        oop_name = vmem.append_string_instance(self.name)
        oop_super = vmem.append_string_instance(self.super_name)
        oop_fields = vmem.append_list_of_strings(self.fields)
        oop_methods = vmem.append_sym_dict_emiting_entries(self.instance_methods)
        oop_class_methods = vmem.append_sym_dict_emiting_entries(self.class_methods)

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(8 * bits.WSIZE)

        oop = vmem.append_external_ref('CompiledClass', self.label()) # vt: CompiledClass
        vmem.append_pointer_to(delegate)
        vmem.append_pointer_to(oop_name)
        vmem.append_pointer_to(oop_super)
        vmem.append_label_ref(self.cmod.label()) ####
        vmem.append_pointer_to(oop_fields)
        vmem.append_pointer_to(oop_methods)
        vmem.append_pointer_to(oop_class_methods)
        self.oop = oop
        return oop


class VariableStorage(object):
    # requirements:
    #
    #  -add: requires current scope only (to check redeclaration)
    #  -index: requires stack of vars to lookup lexically enclosed vars
    #          ie. sibling closures with the same var name should use different locations
    #          for it.
    # -size/env_offset:  requires accumulated scopes

    def __init__(self, cfun, outer_cfun=None, storage=None):
        self.outer_cfun = outer_cfun
        self.parent_storage = storage
        if storage:
            self.variables = storage.variables
        else:
            self.variables = OrderedDict()
        self.variables[cfun] = []

    # def has(self, cfun, name):
    #     try:
    #         self.index(cfun, name)
    #         return True
    #     except Exception:
    #         return False

    # def defined(self, name):
    #     for _, names in self.variables.items():
    #         if name in names:
    #             return True
    #     return False

    def is_visible(self, cfun, name):
        idx = self.variables.keys().index(cfun)
        return name in self._flat(self.variables.values()[:idx+1])

    def add_names(self, cfun, names):
        for name in names:
            self.add(cfun, name)

    def add(self, cfun, name):
        if name in self.variables[cfun]:
            raise Exception('redeclaration of ' + name + " in " + cfun.name)
        self.variables[cfun].append(name)
        return self.index(cfun, name)

    def index(self, cfun, name):
        return self._index(cfun, name)

    def _index(self, cfun, name):
        if name not in self.variables[cfun]:
            if self.parent_storage is None:
                raise Exception("Undeclared " + name)
            else:
                return self.parent_storage.index(self.outer_cfun, name)
        else:
            # if len(self.variables.keys()) > 1: # if this is a closure or if this has closures
            #     # offset it to after rp/dp
            #     return lambda: 2 + self.env_offset(cfun) + self.variables[cfun].index(name)
            # else:
            return lambda: self.env_offset(cfun) + self.variables[cfun].index(name)

    def total(self):
        return len(self._flat(self.variables.values()))

    def _flat(self, lst):
        return reduce(lambda x, y: x + y, lst, [])

    def env_offset(self, cfun):
        idx = self.variables.keys().index(cfun)
        offset = len(self._flat(self.variables.values()[:idx]))
        # if self.parent_storage: # if this is a closure
        #     return 2 + offset
        # else:
        return offset

    def env_table(self):
        return self._flat(self.variables.values())

class CompiledFunction(Entry):
    def __init__(self, cmod, owner, name, params = [], ctor=False, env_storage=None, is_top_level=True, outer_cfun=None, top_level_cfun=None):
        super(CompiledFunction, self).__init__()
        self.cmod = cmod

        self.name = name
        self.params = params
        self.is_ctor = ctor
        self.is_prim = False
        self.prim_name = ''
        self.owner = owner
        self.is_top_level = is_top_level
        self.outer_cfun = outer_cfun
        self.top_level_cfun = top_level_cfun
        self.text = ''
        self.start_line = -1
        self.line_mapping = {}
        self.location_mapping = {}

        if env_storage:
            self.has_env = True
            self.var_declarations = VariableStorage(self, outer_cfun, env_storage)
        else:
            self.has_env = False
            self.var_declarations = VariableStorage(self)
        self.var_declarations.add_names(self, params)

        self.literal_frame = []
        self.bytecodes = opcode.Bytecodes()
        self.exceptions_frame = []

        self.accessor_flag = 0
        self.accessor_field = 0

        self.closures = []

        self.should_wrap_catch_for_non_local_return = False

        self._label = None
        self.var_arg = False

    def set_text(self, text):
        self.text = text

    def set_line(self, head):
        self.start_line = head.start_line

    def set_params(self, params):
        self.params = params
        self.var_declarations.add_names(self, params)

    def set_vararg(self, name):
        self.var_arg = True

    def declare_var(self, name):
        self.var_declarations.add(self, name)

    def declare_vars(self, names):
        self.var_declarations.add_names(self, names)

    def set_getter(self, idx):
        self.accessor_flag = 1 # normal=0/getter=1/setter=2
        self.accessor_field = idx

    def body_processor(self):
        return self

    def uses_env(self, val):
        self.has_env = val

    def set_primitive(self, prim_name):
        self.prim_name = prim_name
        self.is_prim = True

    def label(self):
        if self._label is None:
            self._label = cfun_label(self.owner.label_counter(), self.owner.label(),
                                    self.name, isinstance(self.owner, CompiledClass))
        return self._label

    def literal_frame_label(self):
        return self.label() + '_literal_frame'

    def bytecode_label(self):
        return self.label() + '_bytecodes'

    def exceptions_frame_label(self):
        return self.label() + '_exceptions'

    def fill_literal_frame(self, vmem):
        if len(self.literal_frame) == 0:
            return 0

        # pre-append objects
        for lit in self.literal_frame:
            if lit['tag'] == 'number':
                pass
            elif lit['tag'] == 'symbol':
                lit['oop'] = vmem.append_symbol_instance(lit['value'])
            elif lit['tag'] == 'string':
                lit['oop'] = vmem.append_string_instance(lit['value'])
            elif lit['tag'] == 'cfun':
                lit['oop'] = lit['value'].fill(vmem)
            elif lit['tag'] == 'core':
                pass
            else:
                raise Exception('Todo')

        vmem.append_int(FRAME_TYPE_LITERAL_FRAME)
        vmem.append_int(len(self.literal_frame) * bits.WSIZE)

        vmem.label_current(self.literal_frame_label())

            # if lit['tag'] == 'number':
            #     oop = vmem.append_int(bits.tag(lit['value']))
            #     lit_frame.append(oop)


        # fill frame
        for lit in self.literal_frame:
            if lit['tag'] == 'number':
                vmem.append_int(bits.tag(lit['value']))
            elif lit['tag'] == 'symbol':
                vmem.append_pointer_to(lit['oop'])
            elif lit['tag'] == 'string':
                vmem.append_pointer_to(lit['oop'])
            elif lit['tag'] == 'cfun':
                vmem.append_pointer_to(lit['oop'])
            elif lit['tag'] == 'core':
                vmem.append_external_ref(lit['value']) # unusually reusing literal_frame to store core object references
            else:
                raise Exception('Todo')

        return len(self.literal_frame) * bits.WSIZE


    def fill_bytecodes(self, vmem):
        if len(self.bytecodes) == 0:
            return 0

        bytecodes = ''.join([bits.pack32(w) for w in self.bytecodes.words()])

        vmem.append_int(FRAME_TYPE_BYTECODE_FRAME)
        vmem.append_int(bits.string_block_size(bytecodes + "\0"))

        vmem.label_current(self.bytecode_label())
        vmem.append_string(bytecodes)
        return len(self.bytecodes) * opcode.WORD_SIZE

    def fill_exceptions_frame(self, vmem):
        EXCEPTION_FRAME_SIZE = 3
        if len(self.exceptions_frame) == 0:
            return 0

        type_oops = []
        for entry in self.exceptions_frame:
            if entry['type'] is None:
                type_oops.append(None)
            else:
                type_oops.append(vmem.append_string_instance(entry['type']))

        vmem.append_int(FRAME_TYPE_EXCEPTIONS_FRAME)
        vmem.append_int(len(self.exceptions_frame) * EXCEPTION_FRAME_SIZE * bits.WSIZE)

        vmem.label_current(self.exceptions_frame_label())

        for idx, entry in enumerate(self.exceptions_frame):
            vmem.append_int(entry['start'])
            vmem.append_int(entry['catch'])
            if type_oops[idx] is None:
                vmem.append_null()
            else:
                vmem.append_pointer_to(type_oops[idx])

        return len(self.exceptions_frame)

    def wrap_catch_for_non_local_return(self):
        lb_begin_try = opcode.Label(self.bytecodes, pos=0)
        lb_begin_catch = self.current_label()
        catch_type = "NonLocalReturn"

        ## taking out the value from exception object -- it is on top of stack
        idx_val = self.create_and_register_symbol_literal("value")
        self.bytecodes.append("push_literal", idx_val)
        self.bytecodes.append("send", 0)
        self.bytecodes.append("ret_top",0)
        self.add_exception_entry(lb_begin_try, lb_begin_catch, catch_type)

    def encode_header(self):
        flags = (int(self.is_top_level) << 4) + (int(self.var_arg) << 3) + (int(self.has_env) << 2) + \
                (int(self.is_prim) << 1) + int(self.is_ctor)

        header = (self.var_declarations.env_offset(self) << 15) + (len(self.params) << 10) + (self.var_declarations.total() << 5) + flags
        return header

    def fill(self, vmem):
        if self.should_wrap_catch_for_non_local_return:
            self.wrap_catch_for_non_local_return()

        oop_delegate = vmem.append_object_instance()
        oop_name = vmem.append_string_instance(self.name)
        oop_params = vmem.append_list_of_strings(self.params)
        oop_prim_name = vmem.append_symbol_instance(self.prim_name)
        oop_text = vmem.append_string_instance(self.text)
        oop_line_mappings = vmem.append_int_to_int_dict(self.line_mapping)
        oop_loc_mappings = vmem.append_int_to_int_list(self.location_mapping)
        oop_env_table  = vmem.append_list_of_symbols(self.var_declarations.env_table())

        lit_frame_size = self.fill_literal_frame(vmem)
        bytecode_size = self.fill_bytecodes(vmem)
        exception_frames = self.fill_exceptions_frame(vmem)

        oop_closures = vmem.append_list_of_oops_for_labels([x.label() for x in self.closures])

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(21 * bits.WSIZE)

        oop = vmem.append_external_ref('CompiledFunction', self.label()) # CompiledFunction vt
        vmem.append_pointer_to(oop_delegate)
        vmem.append_int(self.encode_header())

        vmem.append_pointer_to(oop_name)
        vmem.append_pointer_to(oop_params)
        # vmem.append_int(int(self.is_ctor))
        # vmem.append_int(int(self.is_prim))
        vmem.append_pointer_to(oop_prim_name) # 5

        vmem.append_int(self.accessor_flag) # normal=0/getter=1/setter=2
        vmem.append_int(self.accessor_field) # getter/setter field index

        vmem.append_label_ref(self.owner.label())

        # vmem.append_int(len(self.params))

        # vmem.append_int(self.has_env)
        # vmem.append_int(self.is_top_level)

        if self.outer_cfun is None:
            vmem.append_null()
        else:
            vmem.append_label_ref(self.outer_cfun.label())

        # local size or env size
        # vmem.append_int(self.var_declarations.total())
        # else:
        #     vmem.append_int(0) # closures do not need to allocate space in ep or stack

        # env offset
        # print 'offset:' ,self.name, self.var_declarations.env_offset(self), 'storage:', self.var_declarations.total()
        # vmem.append_int(self.var_declarations.env_offset(self))

        vmem.append_int(lit_frame_size)  # 10
        if lit_frame_size > 0:
            vmem.append_label_ref(self.literal_frame_label())
        else:
            vmem.append_null()

        vmem.append_int(bytecode_size)
        if bytecode_size > 0:
            vmem.append_label_ref(self.bytecode_label())
        else:
            vmem.append_null()

        vmem.append_int(exception_frames)
        if exception_frames > 0: # 15
            vmem.append_label_ref(self.exceptions_frame_label())
        else:
            vmem.append_null()

        vmem.append_pointer_to(oop_env_table)
        vmem.append_pointer_to(oop_text)
        vmem.append_pointer_to(oop_line_mappings)
        vmem.append_pointer_to(oop_loc_mappings)
        vmem.append_pointer_to(oop_closures)
        # vmem.append_int(self.var_arg)

        # vmem.append_label_ref(self.cmod.label())
        self.oop = oop
        return oop

    #############

    def new_closure(self, params):
        self.has_env = True
        if self.is_top_level:
            top_level_cfun = self
        else:
            top_level_cfun = self.top_level_cfun

        cfun = CompiledFunction(self.cmod, self.owner, closure_name(self.owner.label_counter()), params,
                                env_storage=self.var_declarations,
                                is_top_level=False, outer_cfun=self,
                                top_level_cfun=top_level_cfun)
        if self.is_top_level:
            self.closures.append(cfun)
        return cfun

    def add_exception_entry(self, label_begin_try, label_begin_catch, catch_type):
        self.exceptions_frame.append({
            'start': label_begin_try(),
            'catch': label_begin_catch(),
            'type': catch_type})


    def current_label(self, current=True):
        return self.bytecodes.new_label(current)

    # def identifier_is_param(self, name):
    #     return name in self.params

    # def identifier_is_decl(self, name):
    #     return name not in self.params and self.var_declarations.has(self, name)

    def identifier_in_scope(self, cfun, name):
        return self.var_declarations.is_visible(cfun, name)

    def identifier_is_module_scoped(self, name):
        # this wont work if the class or function wasn;t compiled yet
        return name in self.cmod.top_level_names

    def identifier_is_prime(self, name):
        return name in ['Object', 'String', 'List', 'Exception', 'Context', 'CompiledFunction']

    def index_for_literal(self, entry):
        if entry not in self.literal_frame:
            self.literal_frame.append(entry)
        return self.literal_frame.index(entry)

    def index_for_top_level(self, name):
        if name in self.cmod.params:
            return self.cmod.params.index(name)
        else:
            # TODO: classes.keys() is not reliable
            # -if a new class appears latter, this pos may change
            # -references to classes that werent compiled yet wont be found
            return len(self.cmod.params) + self.cmod.classes.keys().index(name)

    def create_and_register_number_literal(self, num):
        entry = {"tag": "number", "value": num}
        return self.index_for_literal(entry)

    def create_and_register_symbol_literal(self, string):
        entry = {"tag": "symbol", "value": string}
        return self.index_for_literal(entry)

    def create_and_register_string_literal(self, string):
        entry = {"tag": "string", "value": string}
        return self.index_for_literal(entry)

    def create_and_register_closure_literal(self, cfun):
        entry = {"tag": "cfun", "value": cfun}
        return self.index_for_literal(entry)

    def create_and_register_core(self, class_name):
        # unusually reusing literal_frame to store core objects
        entry = {'tag': 'core', 'value': class_name}
        return self.index_for_literal(entry)


    # emit procedures
    #!(fnobj.current_bytecode_pos()):bpos or fnobj.update_line_mapping(bpos, ast)

    @emitter
    def emit_push_var(self, _, name):
        if self.identifier_in_scope(self, name):
            idx = self.var_declarations.index(self, name)
            self.bytecodes.append("push_local", idx)
        elif self.identifier_is_module_scoped(name) or self.identifier_is_prime(name):
            idx = self.create_and_register_symbol_literal(name)
            self.bytecodes.append("push_module", 0)
            self.bytecodes.append("push_literal", idx)
            self.bytecodes.append('send', 0)
        else:
            # raise Exception('push_var: undeclared ' + name)
            # for now, lets assume its a module instead of raising,
            # to make it easy for dynamic eval() code
            idx = self.create_and_register_symbol_literal(name)
            self.bytecodes.append('push_module', 0)
            self.bytecodes.append('push_literal', idx)
            self.bytecodes.append('send', 0)

    @emitter
    def emit_local_assignment(self, _, name):
        if self.identifier_in_scope(self, name):
            idx = self.var_declarations.index(self, name)
            self.bytecodes.append("pop_local", idx)
        else:
            raise Exception('local_assignment: undeclared ' + name)

    @emitter
    def emit_return_top(self, _):
        self.bytecodes.append("ret_top",0)

    @emitter
    def emit_non_local_return(self, _):
        idx_class = self.create_and_register_symbol_literal("NonLocalReturn")
        idx_throw = self.create_and_register_symbol_literal("throw")
        self.bytecodes.append("push_module",0)
        self.bytecodes.append('push_literal', idx_class)
        self.bytecodes.append('send', 0)
        self.bytecodes.append('push_literal', idx_throw)
        self.bytecodes.append('send', 1)
        self.top_level_cfun.should_wrap_catch_for_non_local_return = True


    @emitter
    def emit_return_null(self, _):
        self.bytecodes.append('push_bin', 0)
        self.bytecodes.append("ret_top",0)

    @emitter
    def emit_end_body(self, _):
        if self.is_top_level:
            self.bytecodes.append("ret_this",0)
        else:
            self.bytecodes.append('push_bin', 0)
            self.bytecodes.append("ret_top",0)


    # @emitter
    # def emit_return_this(self, _):
    #     self.bytecodes.append("ret_this",0)

    def current_bytecode_pos(self):
        return len(self.bytecodes)

    def update_line_mapping(self, bpos, ast):
        if ast is None or not hasattr(ast, 'start_line'):
            return

        # self.location_mapping: bytecode_offset => expression region
        # self.line_mapping: bytecode_offset => lineno

        if self.outer_cfun:
            start_line = ast.start_line # - self.outer_cfun.start_line
            end_line = ast.end_line # - self.outer_cfun.start_line
        else:
            start_line = ast.start_line # - self.start_line
            end_line = ast.end_line # - self.start_line

        # print self.name, bpos, [start_line, ast.start_col, end_line, ast.end_col]
        # if bpos not in self.location_mapping:
        self.location_mapping[bpos] = [start_line, ast.start_col, end_line, ast.end_col]
        # for i in range(bpos, self.current_bytecode_pos()):
        #     if i not in self.location_mapping:
        #         self.location_mapping[i] = [start_line, ast.start_col, end_line, ast.end_col]

        self.line_mapping[bpos] = start_line

    @emitter
    def emit_var_decl(self, _, name):
        self.declare_var(name)
        idx = self.var_declarations.index(self, name)
        self.bytecodes.append("pop_local", idx)

    @emitter
    def emit_field_assignment(self, _, field):
        idx = self.owner.fields.index(field)
        self.bytecodes.append('pop_field', idx)

    @emitter
    def emit_index_assignment(self, _):
        idx_selector = self.create_and_register_symbol_literal("set")
        self.bytecodes.append('push_literal', idx_selector)
        self.bytecodes.append('send', 2)

    @emitter
    def emit_pop(self, _):
        self.bytecodes.append('pop', 0)

    @emitter
    def emit_send_or_local_call(self, _, name, arity):
        if self.identifier_in_scope(self, name):
            idx = self.var_declarations.index(self, name)
            self.bytecodes.append("push_local", idx)
            self.bytecodes.append("call", arity)
        elif self.identifier_is_module_scoped(name):
            idx = self.create_and_register_symbol_literal(name)
            self.bytecodes.append('push_module', 0)
            self.bytecodes.append('push_literal', idx)
            self.bytecodes.append('send', arity)
        else:
            # raise Exception('Undeclared ' + name)
            # for now, lets assume its a module instead of raising,
            # to make it easy for dynamic eval() code
            idx = self.create_and_register_symbol_literal(name)
            self.bytecodes.append('push_module', 0)
            self.bytecodes.append('push_literal', idx)
            self.bytecodes.append('send', arity)

    @emitter
    def emit_call(self, _, arity):
        self.bytecodes.append('call', arity)

    @emitter
    def emit_opt_send(self, ast, arity):
        # obj.send(x, [x,y,z ,...]) -> obj.x(x,y,z,...)
        self.bytecodes.append('send', arity)

    @emitter
    def emit_send(self, _, selector, arity):
        idx = self.create_and_register_symbol_literal(selector)
        self.bytecodes.append('push_literal', idx)
        self.bytecodes.append('send', arity)

    @emitter
    def emit_super_send(self, _, arity):
        self.bytecodes.append('super_send', arity)

    @emitter
    def emit_super_ctor_send(self, _, selector, arity):
        idx = self.create_and_register_symbol_literal(selector)
        self.bytecodes.append('push_literal', idx)
        self.bytecodes.append('super_ctor_send', arity)


    @emitter
    def emit_binary(self, _, selector):
        idx = self.create_and_register_symbol_literal(selector)
        self.bytecodes.append('push_literal', idx)
        self.bytecodes.append('send', 1)

    @emitter
    def emit_unary(self, _, selector):
        idx = self.create_and_register_symbol_literal(selector)
        self.bytecodes.append('push_literal', idx)
        self.bytecodes.append('send', 0)

    def emit_jz(self, lb=None):
        lb = lb or self.bytecodes.new_label()
        self.bytecodes.append('jz', lb)
        return lb

    def emit_jmp(self, lb=None):
        lb = lb or self.bytecodes.new_label()
        self.bytecodes.append('jmp', lb)
        return lb

    def emit_jmp_back(self, lb=None):
        lb = lb or self.bytecodes.new_label()
        self.bytecodes.append('jmpb', lb)
        return lb

    @emitter
    def emit_push_num_literal(self, _, num):
        if num <= 4611686018427387903:
            idx = self.create_and_register_number_literal(num)
            self.bytecodes.append("push_literal", idx)
        else:
            raise Exception('Integer is too big: ' + str(num))

    @emitter
    def emit_push_this(self, _):
        self.bytecodes.append('push_this', 0)

    @emitter
    def emit_push_str_literal(self, _, string):
        idx = self.create_and_register_string_literal(string)
        self.bytecodes.append('push_literal', idx)

    @emitter
    def emit_push_sym_literal(self, _, sym):
        idx = self.create_and_register_symbol_literal(sym)
        self.bytecodes.append('push_literal', idx)

    @emitter
    def emit_push_null(self, _):
        self.bytecodes.append('push_bin', 0)

    @emitter
    def emit_push_true(self, _):
        self.bytecodes.append('push_bin', 1)

    @emitter
    def emit_push_false(self, _):
        self.bytecodes.append('push_bin', 2)

    @emitter
    def emit_push_module(self, _):
        self.bytecodes.append('push_module', 0)

    @emitter
    def emit_push_context(self, _):
        self.bytecodes.append('push_context', 0)

    @emitter
    def emit_push_closure(self, _, fn):
        # idx_selector = self.create_and_register_symbol_literal("new_context")
        # self.bytecodes.append("push_fp", 0)
        # self.bytecodes.append("push_module", 0)
        # self.bytecodes.append('push_literal', idx_cfun)
        # self.bytecodes.append('push_literal', idx_selector)
        # self.bytecodes.append('send', 2)
        idx_cfun = self.create_and_register_closure_literal(fn)
        #idx_klass = self.create_and_register_symbol_literal("Context")
        #idx_new = self.create_and_register_symbol_literal("new")


        # self.bytecodes.append('push_literal', idx_cfun)
        # self.bytecodes.append("push_fp", 0)
        # self.bytecodes.append("push_module", 0)
        self.bytecodes.append("new_context", idx_cfun)

        # self.bytecodes.append('push_module', 0)
        # self.bytecodes.append('push_literal', idx_klass)
        # self.bytecodes.append('send', 0)

        # self.bytecodes.append('push_literal', idx_new)
        # self.bytecodes.append('send', 3)


    @emitter
    def emit_push_field(self, _, field):
        idx = self.owner.fields.index(field)
        self.bytecodes.append('push_field', idx)

    def bind_catch_var(self, name):
        if not self.identifier_in_scope(self, name):
            self.declare_var(name)
        self.emit_local_assignment(None, name)

    def emit_catch_jump(self):
        self.bytecodes.append("jmp", 0) # arg 0 will be substituted later
        # hack
        return len(self.bytecodes) - 1

    def emit_try_catch(self, lb_begin_try, lb_begin_catch, jmp_pos, catch_type):
        # hack
        blen = len(self.bytecodes)
        self.bytecodes.lst[jmp_pos].arg = lambda: blen - jmp_pos
        self.add_exception_entry(lb_begin_try, lb_begin_catch, catch_type)


    @emitter
    def emit_push_list(self, _, length):
        # idx_length = self.create_and_register_number_literal(length)
        # idx_selector = self.create_and_register_symbol_literal("new")
        # idx_append = self.create_and_register_symbol_literal("prepend")
        # idx_klass = self.create_and_register_symbol_literal("List")
        # self.bytecodes.append("push_module", 0)
        # self.bytecodes.append('push_literal', idx_klass)
        # self.bytecodes.append('send', 0)

        # self.bytecodes.append('push_literal', idx_selector)
        # self.bytecodes.append('send', 0)

        # for i in range(0, length):
        #     self.bytecodes.append('push_literal', idx_append)
        #     self.bytecodes.append('send', 1)

        ####

        # #idx_length = self.create_and_register_number_literal(length)
        # idx_klass = self.create_and_register_core("List")
        # idx_new_from_stack = self.create_and_register_symbol_literal("new_from_stack")

        # self.bytecodes.append('push_literal', idx_klass) # unusually reusing literal_frame to store core object references
        # self.bytecodes.append('push_literal', idx_new_from_stack)
        # self.bytecodes.append('send', length)

        ###

        #idx_length = self.create_and_register_number_literal(length)
        idx_klass = self.create_and_register_symbol_literal("List")
        idx_new_from_stack = self.create_and_register_symbol_literal("new_from_stack")

        self.bytecodes.append("push_module", 0)
        self.bytecodes.append('push_literal', idx_klass)
        self.bytecodes.append('send', 0)

        self.bytecodes.append('push_literal', idx_new_from_stack)
        self.bytecodes.append('send', length)

    @emitter
    def emit_push_dict(self, _, length):
        #idx_length = self.create_and_register_number_literal(length)
        idx_new_from_stack = self.create_and_register_symbol_literal("new_from_stack")
        #idx_set = self.create_and_register_symbol_literal("set")
        idx_klass = self.create_and_register_symbol_literal("Dictionary")

        self.bytecodes.append("push_module", 0)
        self.bytecodes.append('push_literal', idx_klass)
        self.bytecodes.append('send', 0)

        self.bytecodes.append('push_literal', idx_new_from_stack)
        self.bytecodes.append('send', length*2)

        # for i in range(0, length):
        #     self.bytecodes.append('push_literal', idx_set)
        #     self.bytecodes.append('send', 2)

    @emitter
    def emit_push_index(self, _):
        idx_selector = self.create_and_register_symbol_literal("index")
        self.bytecodes.append('push_literal', idx_selector)
        self.bytecodes.append('send', 1)

class Function(Entry):
    def __init__(self, imod, cfun):
        super(Function, self).__init__()
        self.imod = imod
        self.cfun = cfun

    def set_vararg(self, name):
        self.cfun.set_vararg(name)

    def set_params(self, params):
        self.cfun.set_params(params)

    def body_processor(self):
        return self.cfun

    def uses_env(self, val):
        self.cfun.uses_env(val)

    def set_line(self, line):
        self.cfun.set_line(line)

    def set_text(self, text):
        self.cfun.set_text(text)

    def set_primitive(self, prim_name):
        self.cfun.set_primitive(prim_name)

    def label(self):
        return fun_label(self.cfun.label())

    def fill(self, vmem):
        delegate = vmem.append_object_instance()

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(4 * bits.WSIZE)

        oop = vmem.append_label_ref('Function', self.label())   # vt
        vmem.append_pointer_to(delegate)                        # delegate
        vmem.append_label_ref(self.cfun.label())                # compiled_function
        vmem.append_label_ref(self.imod.label())                # module
        # 4: env
        self.oop = oop
        return oop


def create_module_to_string(cmod):
    cfun = CompiledFunction(cmod, cmod, 'toString', [])
    cfun.set_primitive('module_to_string')
    return cfun

class CompiledModule(Entry):
    def __init__(self, name):
        super(CompiledModule, self).__init__()
        self.name = name
        self.top_level_names = []
        self.params = []
        self.meta_vars = {}
        self.default_locations = {}
        self.imports = {}

        self.functions = {'toString': create_module_to_string(self)}
        self.classes = {}

    def add_meta(self, key, val):
        self.meta_vars[key] = val

    def set_params(self, params):
        self.params = params
        for p in params:
            self.add_top_level_name(p)

    def add_default_location(self, mod, path):
        self.default_locations[mod] = path

    def add_import(self, name, from_):
        self.imports[name] = from_
        self.add_top_level_name(name)

    def add_top_level_name(self, name):
        self.top_level_names.append(name)

    def entry_labels(self):
        return self.functions.keys() + self.classes.keys()

    def label(self):
        return cmod_label(self.name)

    def new_function(self, name, params):
        fn = CompiledFunction(self, self, name, params)
        self.functions[name] = fn
        return fn

    def new_class(self, name, parent, fields):
        klass = CompiledClass(self, name, parent, fields)
        self.classes[name] = klass
        return klass

    def fill(self, vmem):
        # first word on object table is a pointer to the CompiledModule
        vmem.append_label_ref(self.label())

        delegate = vmem.append_object_instance()
        oop_name = vmem.append_string_instance(self.name)
        oop_params = vmem.append_list_of_strings(self.params)
        oop_meta_vars = vmem.append_symbol_dict(self.meta_vars)
        oop_def_locations = vmem.append_symbol_dict(self.default_locations)
        oop_imports = vmem.append_symbol_dict(self.imports)
        oop_functions = vmem.append_sym_dict_emiting_entries(self.functions)
        oop_classes = vmem.append_sym_dict_emiting_entries(self.classes)

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int(10 * bits.WSIZE)

        oop = vmem.append_external_ref('CompiledModule', self.label()) # vt: CompiledModule
        vmem.append_pointer_to(delegate)
        vmem.append_pointer_to(oop_name)
        vmem.append_pointer_to(oop_params)
        vmem.append_pointer_to(oop_meta_vars)
        vmem.append_pointer_to(oop_def_locations)
        vmem.append_pointer_to(oop_imports)
        vmem.append_pointer_to(oop_functions)
        vmem.append_pointer_to(oop_classes)
        vmem.append_null()                        # parent_module
        self.oop = oop
        return oop

class CoreModule(Entry):
    # core's Module instance (conflated approach)
    def __init__(self, cmod):
        super(CoreModule, self).__init__()
        self.cmod = cmod
        self.functions = {}
        self.classes = []
        self.objects = []

    def object_by_name(self, name):
        return [obj for obj in self.objects if obj.name == name][0]

    def append_class(self, klass):
        self.classes.append(klass)

    def append_function(self, name, fun):
        self.functions[name] = fun

    def append_object(self, obj):
        self.objects.append(obj)

    def entry_labels(self):
        top_objects = ['Behavior', 'Object_Behavior', 'Object']
        res = [f.label() for f in self.functions.values()]
        res += [c.label() for c in self.classes]
        res += [o.label() for o in self.objects if o.label() in top_objects]
        return res + [self.label()]

    def label(self):
        return '@core_module'


    def create_getter(self, name, idx, vmem):
        cfun = CompiledFunction(self.cmod, self.cmod, name, [])
        cfun.set_getter(idx)
        cfun.fill(vmem)
        return Function(self, cfun)

    def emit_dict(self, vmem):
        d = dict(self.functions.items())

        idx = 4 # start after vt, delegate, dict, compiled_module

        for obj in self.objects:
            obj.fill(vmem)
            d[obj.label()] = self.create_getter(obj.label(), idx, vmem)
            idx += 1

        for klass in self.classes:
            klass.fill(vmem)
            d[klass.label()] = self.create_getter(klass.label(), idx, vmem)
            idx +=1

        return vmem.append_sym_dict_emiting_entries(d)

    def fill(self, vmem):
        oop_dict = self.emit_dict(vmem)

        # obj_oops = [o.fill(vmem) for o in self.objects]
        # class_oops = [c.fill(vmem) for c in self.classes]

        vmem.append_int(FRAME_TYPE_OBJECT)
        vmem.append_int((4 + len(self.objects) + len(self.classes)) * bits.WSIZE)

        oop = vmem.append_label_ref(self.label(), self.label()) # vt
        vmem.append_null()                                      # delegate
        vmem.append_pointer_to(oop_dict)                        # dict
        vmem.append_label_ref(self.cmod.label())                # compiled_module

        for obj in self.objects:
            vmem.append_label_ref(obj.label())

        for klass in self.classes:
            vmem.append_label_ref(klass.label())

        self.oop = oop
        return oop


class CoreCompiledModule(CompiledModule):
    def __init__(self):
        super(CoreCompiledModule, self).__init__('core')
        self.imod = CoreModule(self)

    def label(self):
        return 'CoreCompiledModule'

    def entry_labels(self):
        return self.imod.entry_labels()

    def new_function(self, name, params):
        cfun = super(CoreCompiledModule, self).new_function(name, params)
        fn = Function(self.imod, cfun)
        self.imod.append_function(name, fn)
        return fn

    def new_class(self, name, parent, fields):
        cclass = super(CoreCompiledModule, self).new_class(name, parent, fields)
        klass = Class(self.imod, cclass)
        self.imod.append_class(klass)
        return klass

    def new_object(self, name):
        if name == 'Object_Behavior':
            obj = Object_ObjectBehavior(name, self.imod)
        elif name == 'Object':
            obj = Object_Object(name, self.imod)
        else:
            obj = Object(name, self.imod)
        self.imod.append_object(obj)
        return obj

    def fill(self, vmem):
        super(CoreCompiledModule, self).fill(vmem)
        self.imod.fill(vmem)
