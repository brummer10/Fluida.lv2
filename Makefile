
SUBDIR := Fluida

.PHONY: $(SUBDIR) libxputty  recurse

$(MAKECMDGOALS) recurse: $(SUBDIR)

clean:

libxputty:
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

$(SUBDIR): libxputty
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

