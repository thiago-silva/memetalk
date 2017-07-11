import pyutils
from pyutils import bits
from pyutils import vmemory
from pdb import set_trace as br


class CompVirtualMemory(vmemory.VirtualMemory):
    def __init__(self):
        super(CompVirtualMemory, self).__init__()
        self.ext_ref_table = []
        self.int_ref_table = []
        self.symb_table = []
        self.string_table = {}

    def external_references(self):
        return [(x[0], self.physical_address(x[1])) for x in self.ext_ref_table]

    def internal_references(self):
        return [(x[0], self.physical_address(x[1])) for x in self.int_ref_table]

    def external_names(self):
        return sorted(set([x[0] for x in self.ext_ref_table] + [x[0] for x in self.symb_table]))

    def reloc_table(self):
        return [self.physical_address(entry) for entry in self.cells if type(entry) == vmemory.PointerCell]

    def append_external_ref(self, name, label=None):
        oop = self.append_int(0xAAAA, label)
        self.ext_ref_table.append((name, oop))
        return oop

    def append_internal_ref(self, name, label=None):
        oop = self.append_int(0xCCCC, label)
        self.int_ref_table.append((name, oop))
        return oop

    def append_object_instance(self):
        self.append_int(pyutils.FRAME_TYPE_OBJECT)
        self.append_int(2 * bits.WSIZE)

        oop = self.append_external_ref('Object') # vt
        self.append_null()                    # delegate: end of chain of delegation
        return oop

    def append_symbol_instance(self, string):
        oop = self.append_int(0xBBBB)
        self.symb_table.append((string, oop))
        return oop

    def append_string_instance(self, string):
        if string in self.string_table:
            return self.string_table[string]
        else:
            delegate = self.append_object_instance()

            self.append_int(pyutils.FRAME_TYPE_BVAR_OBJECT)
            self.append_int((3 * bits.WSIZE) + bits.string_block_size(string + "\0"))

            oop = self.append_external_ref('String')
            self.append_pointer_to(delegate)        # delegate
            self.append_int(len(string))
            self.append_string(string)

            self.string_table[string] = oop
            return oop

    def append_sym_to_string_dict(self, pydict):
        pairs_oop = []
        for key, val in iter(sorted(pydict.items())):
            key_oop = self.append_string_instance(key)
            val_oop = self.append_symbol_instance(val)
            pairs_oop.append((key_oop, val_oop))
        return self.append_dict_with_pairs(pairs_oop)

    def append_dict_prologue(self, size,  frame_oop):
        delegate = self.append_object_instance()

        self.append_int(pyutils.FRAME_TYPE_DVAR_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('Dictionary')  # vt
        self.append_pointer_to(delegate)              # delegate
        self.append_int(size)                         # dict length
        if frame_oop is None:
            self.append_null()
        else:
            self.append_pointer_to(frame_oop)
        return oop

    def append_empty_list(self):
        delegate = self.append_object_instance()

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('List')         # vt
        self.append_pointer_to(delegate)               # delegate
        self.append_int(0)                             # len
        self.append_null()                             # frame
        return oop


    # used internally to create class fields list, etc.
    def append_list_of_strings(self, lst):
        oops_elements = [self.append_string_instance(string) for string in lst]
        delegate = self.append_object_instance()

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in oops_elements:         # .. elements
                oops.append(self.append_pointer_to(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('List')    # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop

    def append_list_of_symbols(self, lst):
        oops_elements = [self.append_symbol_instance(string) for string in lst]
        delegate = self.append_object_instance()

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in oops_elements:         # .. elements
                oops.append(self.append_pointer_to(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('List')    # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop

    # used internally to create class fields list, etc.
    def append_list_of_ints(self, lst):
        delegate = self.append_object_instance()

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in lst:         # .. elements
                oops.append(self.append_tagged_int(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('List')    # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop

    def append_list_of_oops_for_labels(self, lst):
        delegate = self.append_object_instance()

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for label in lst:         # .. elements
                oops.append(self.append_label_ref(label))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_external_ref('List')    # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop
