	
	STRIP ?= strip
	# check if user is root
	user = $(shell whoami)
	ifeq ($(user),root)
	INSTALL_DIR = /usr/lib/lv2
	else 
	INSTALL_DIR = ~/.lv2
	endif

	# check CPU and supported optimization flags
	ifneq ($(shell cat /proc/cpuinfo | grep sse3 ) , )
		SSE_CFLAGS = -msse3 -mfpmath=sse -mfxsr
	else ifneq ($(shell cat /proc/cpuinfo | grep sse2 ) , )
		SSE_CFLAGS = -msse2 -mfpmath=sse -mfxsr
	else ifneq ($(shell cat /proc/cpuinfo | grep sse ) , )
		SSE_CFLAGS = -msse -mfpmath=sse -mfxsr
		else ifneq ($(shell cat /proc/cpuinfo | grep ARM ) , )
		ifneq ($(shell cat /proc/cpuinfo | grep ARMv7 ) , )
			ifneq ($(shell cat /proc/cpuinfo | grep vfpd32 ) , )
				SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3 
			else ifneq ($(shell cat /proc/cpuinfo | grep vfpv3 ) , )
				SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3
			endif
		else
			ARMCPU = "YES"
		endif
	else
		SSE_CFLAGS =
	endif

	# set bundle name
	NAME = Fluida
	BUNDLE = $(NAME).lv2
	VER = 0.9

	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	SCALA_DIR := ./libscala-file/

	# set compile flags
ifeq ($(TARGET), Linux)
	CXXFLAGS += -D_FORTIFY_SOURCE=2 -I. -I./dsp -I./plugin -fPIC -DPIC -O2 -Wall -funroll-loops \
	-fstack-protector -ffast-math -fomit-frame-pointer -fstrength-reduce \
	`pkg-config --cflags --libs fluidsynth` \
	-fdata-sections -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,--exclude-libs,ALL $(SSE_CFLAGS)
	CFLAGS += -fPIC -DPIC -O2 -Wall
	LDFLAGS += -I. -I$(SCALA_DIR) -lm -pthread -lpthread -shared -lm -Wl,-z,noexecstack -Wl,--no-undefined \
	-fvisibility=hidden `pkg-config --cflags --libs fluidsynth`
	GUI_LDFLAGS +=  -I$(HEADER_DIR) -Wl,-z,noexecstack -Wl,--no-undefined -fvisibility=hidden \
	-L. $(LIB_DIR)libxputty.a -shared `pkg-config --cflags --libs cairo x11` -lm  -Wl,-z,nodelete 
else ifeq ($(TARGET), Windows)
	CXXFLAGS += -D_FORTIFY_SOURCE=2 -I. -I./dsp -I./plugin -fPIC -DPIC -O2 -Wall -funroll-loops \
	-fstack-protector -ffast-math -fomit-frame-pointer -fstrength-reduce \
	`$(PKGCONFIG) $(PKGCONFIG_FLAGS)  --cflags --libs fluidsynth` \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL $(SSE_CFLAGS)
	CFLAGS += -fPIC -DPIC -O2 -Wall
	LDFLAGS += -I. -I$(SCALA_DIR) `$(PKGCONFIG) $(PKGCONFIG_FLAGS)  --cflags --libs fluidsynth` \
	-shared -lm $(PAWPAW_LFLAGS)

	GUI_LDFLAGS += -I./gui -I./plugin -I$(HEADER_DIR) -static-libgcc -static-libstdc++ \
	`$(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags --libs cairo ` \
	-L. $(LIB_DIR)libxputty.$(STATIC_LIB_EXT) -shared  -lm $(PAWPAW_LFLAGS)

	TTLUPDATE = sed -i '/lv2:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/manifest.ttl
	TTLUPDATEGUI = sed -i '/a guiext:X11UI/ s/X11UI/WindowsUI/ ; /guiext:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/$(NAME).ttl
endif
	# invoke build files
	OBJECTS = fluida.cpp XSynth.cpp $(SCALA_DIR)scala_scl.cpp $(SCALA_DIR)scala_kbm.cpp
	GUI_OBJECTS = fluida_ui.c
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : $(HEADER_DIR)*.h mod all clean install uninstall 

all : check $(NAME)
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp ./*.ttl ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEGUI)
	$(QUIET)mv ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "sorry, build failed$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

mod : check clean nogui
	@mkdir -p ../bin/$(BUNDLE)
	@cp -R ./MOD/* ../bin/$(BUNDLE)
	@mv ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "sorry, build failed$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

check :
ifdef ARMCPU
	@echo $(RED)ARM CPU DEDECTED, please check the optimization flags
	@echo $(NONE)
endif

clean :
	$(QUIET)rm -f *.a *.o *.$(LIB_EXT) 
	$(QUIET)rm -f $(NAME).$(LIB_EXT)
	$(QUIET)rm -rf ../bin
ifndef EXTRAQUIET
	@$(ECHO) ". ., clean up$(reset)"
endif

dist-clean :
	$(QUIET)rm -f *.a *.o *.$(LIB_EXT) 
	$(QUIET)rm -f $(NAME).$(LIB_EXT)
	$(QUIET)rm -rf ../bin
ifndef EXTRAQUIET
	@$(ECHO) ". ., clean up$(reset)"
endif

install :
ifeq ($(TARGET), Linux)
ifneq ("$(wildcard ../bin/$(BUNDLE))","")
	@$(ECHO) "Install $(NAME).lv2$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)cp -r ../bin/$(BUNDLE)/* $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)$(B_ECHO) ". . , done$(reset)"
else
	$(QUIET)$(R_ECHO) ". . , you must build first$(reset)"
endif
else
	$(QUIET)$(R_ECHO) "Install is not implemented for windows, please copy the folder $(NAME).lv2 to Program Files/Common Files/LV2$(reset)"
endif

uninstall :
ifeq ($(TARGET), Linux)
	@$(ECHO) "Uninstall $(NAME).lv2$(reset)"
	$(QUIET)rm -rf $(INSTALL_DIR)/Xmonc.lv2
	$(QUIET)rm -rf $(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)$(B_ECHO) ". . , done$(reset)"
else
	$(QUIET)$(R_ECHO) "Uninstall is not implemented for windows, please remove the folder $(NAME).lv2 from Program Files/Common Files/LV2$(reset)"
endif
$(NAME) :
	@$(B_ECHO) "Compiling $@.$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) -std=c++11  $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).$(LIB_EXT)
	@$(B_ECHO) "Compiling $@_ui.$(LIB_EXT) $(reset)"
	$(QUIET)$(CC) $(CFLAGS) $(GUI_OBJECTS) $(GUI_LDFLAGS) -o $(NAME)_ui.$(LIB_EXT)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME).$(LIB_EXT)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME)_ui.$(LIB_EXT)

nogui :
	@$(B_ECHO) "Compiling $(NAME).$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) -std=c++11  $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).$(LIB_EXT)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME).$(LIB_EXT)

doc:
	#pass
