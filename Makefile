
BLUE = "\033[1;34m"
RED =  "\033[1;31m"
NONE = "\033[0m"

SUBDIR := Fluida

.PHONY: $(SUBDIR) libxputty  recurse mod 

ifneq ($(findstring mod,${MAKECMDGOALS}),mod)
$(MAKECMDGOALS) recurse: $(SUBDIR)

check-and-reinit-submodules :
	@if git submodule status 2>/dev/null | egrep -q '^[-]|^[+]' ; then \
		echo $(RED)"INFO: Need to reinitialize git submodules"$(NONE); \
		git submodule update --init; \
		echo $(BLUE)"Done"$(NONE); \
	else echo $(BLUE)"Submodule up to date"$(NONE); \
	fi

clean:

libxputty: check-and-reinit-submodules
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

$(SUBDIR): libxputty
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

endif

mod:
	@exec $(MAKE) -j 1 -C Fluida $(MAKECMDGOALS)
