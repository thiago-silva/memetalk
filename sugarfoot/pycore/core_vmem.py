import pyutils
from pyutils import vmemory
from pyutils import bits
from . import utils
from pdb import set_trace as br

class CoreVirtualMemory(vmemory.VirtualMemory):
    def __init__(self):
        self.string_table = {}
        self.symb_table = []
        self.int_ref_table = []
        super(CoreVirtualMemory, self).__init__()

    def index_for(self, name):
        return self.base + self.index[name]

    def external_names(self):
        return sorted(set([x[0] for x in self.symb_table]))

    def append_external_ref(self, name, label):
        return self.append_label_ref(name, label) # core is self contained and does not require external names

    def append_internal_ref(self, name, label=None):
        oop = self.append_int(0xCCCC, label)
        self.int_ref_table.append((name, oop))
        return oop

    def append_object_instance(self):
        self.append_int(pyutils.FRAME_TYPE_OBJECT)
        self.append_int(2 * bits.WSIZE)

        oop = self.append_label_ref('Object') # vt
        self.append_null()                    # delegate: end of chain of delegation
        return oop

    def append_string_instance(self, string):
        if string in self.string_table:
            return self.string_table[string]
        else:
            delegate = self.append_object_instance() # Assumed to be object! if source change, this breaks

            self.append_int(pyutils.FRAME_TYPE_BVAR_OBJECT)
            self.append_int((3 * bits.WSIZE) + bits.string_block_size(string + "\0"))

            oop = self.append_label_ref(utils.class_label('String'))   # vt
            self.append_pointer_to(delegate)        # delegate
            self.append_int(len(string))
            self.append_string(string)

            self.string_table[string] = oop
            return oop

    def append_symbol_instance(self, string):
        # We only need a dummy/word-sized placeholder here.
        # (actually, not event this. We only need to return a dummy cell. All we need
        #  its the PointerCell's pointing to our dummy to construct the symbols_reference())
        # In any case, the full data is only used by dump to display the text of the symbol
        delegate = self.append_object_instance() # Assumed to be object! if source change, this breaks

        self.append_int(pyutils.FRAME_TYPE_BVAR_OBJECT)
        self.append_int((3 * bits.WSIZE) + bits.string_block_size(string + "\0"))

        oop = self.append_label_ref(utils.class_label('Symbol'))   # vt
        self.append_pointer_to(delegate)                           # delegate
        self.append_int(len(string))
        self.append_string(string)

        self.symb_table.append((string, oop))
        return oop

    def append_dict_prologue(self, size, frame_oop):
        delegate = self.append_object_instance()        # Assumed to be object! if source change, this breaks

        self.append_int(pyutils.FRAME_TYPE_DVAR_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_label_ref(utils.class_label('Dictionary')) # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(size)                     # dict length
        if frame_oop is None:
            self.append_null()
        else:
            self.append_pointer_to(frame_oop)
        return oop

    def append_empty_list(self):
        delegate = self.append_object_instance()                 # Assumed to be object! if source change, this breaks

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_label_ref(utils.class_label('List'))  # vt
        self.append_pointer_to(delegate)                        # delegate
        self.append_int(0)                                      # len
        self.append_null()                                      # frame
        return oop


    # used internally to create class fields list, etc.
    def append_list_of_strings(self, lst):
        oops_elements = [self.append_string_instance(string) for string in lst]
        delegate = self.append_object_instance()  # Assumed to be object! if source change, this breaks

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in oops_elements:         # .. elements
                oops.append(self.append_pointer_to(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_label_ref(utils.class_label('List'))  # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop

    def append_list_of_symbols(self, lst):
        oops_elements = [self.append_symbol_instance(string) for string in lst]
        delegate = self.append_object_instance()  # Assumed to be object! if source change, this breaks

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in oops_elements:         # .. elements
                oops.append(self.append_pointer_to(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_label_ref(utils.class_label('List'))  # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop


    # used internally to create class fields list, etc.
    def append_list_of_ints(self, lst):
        delegate = self.append_object_instance()  # Assumed to be object! if source change, this breaks

        if len(lst) > 0:
            self.append_int(pyutils.FRAME_TYPE_ELEMENTS)
            self.append_int(len(lst) * bits.WSIZE)
            oops = []
            for oop_element in lst:         # .. elements
                oops.append(self.append_tagged_int(oop_element))

        self.append_int(pyutils.FRAME_TYPE_LIST_OBJECT)
        self.append_int(4 * bits.WSIZE)

        oop = self.append_label_ref(utils.class_label('List'))  # vt
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

        oop = self.append_label_ref(utils.class_label('List'))  # vt
        self.append_pointer_to(delegate)          # delegate
        self.append_int(len(lst))                 # len
        if len(lst) > 0:
            self.append_pointer_to(oops[0])
        else:
            self.append_null()
        return oop
