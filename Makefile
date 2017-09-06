subdirs = py src memetalk

all:
	$(foreach el,$(subdirs),$(MAKE) -C $(el) all;)

clean:
	$(foreach el,$(subdirs),$(MAKE) -C $(el) clean;)

debug:
	$(foreach el,$(subdirs),$(MAKE) -C $(el) debug;)

test:
	$(foreach el,$(subdirs),$(MAKE) -C $(el) test;)

.PHONY: $(subdirs) clean
