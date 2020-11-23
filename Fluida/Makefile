	
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
		SSE_CFLAGS = -msse3 -mfpmath=sse
	else ifneq ($(shell cat /proc/cpuinfo | grep sse2 ) , )
		SSE_CFLAGS = -msse2 -mfpmath=sse
	else ifneq ($(shell cat /proc/cpuinfo | grep sse ) , )
		SSE_CFLAGS = -msse -mfpmath=sse
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
	VER = 0.5

	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/

	# set compile flags
	CXXFLAGS += -D_FORTIFY_SOURCE=2 -I. -I./dsp -I./plugin -fPIC -DPIC -O2 -Wall -funroll-loops \
	-fstack-protector -ffast-math -fomit-frame-pointer -fstrength-reduce \
	`pkg-config --cflags --libs fluidsynth` \
	-fdata-sections -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,--exclude-libs,ALL $(SSE_CFLAGS)
	LDFLAGS += -I. -shared -lm -Wl,-z,noexecstack -fvisibility=hidden `pkg-config --cflags --libs fluidsynth`
	GUI_LDFLAGS +=  -I$(HEADER_DIR) -Wl,-z,noexecstack -fvisibility=hidden \
	-L. $(LIB_DIR)libxputty.a -shared `pkg-config --cflags --libs cairo x11` -lm 
	# invoke build files
	OBJECTS = fluida.cpp XSynth.cpp
	GUI_OBJECTS = fluida_ui.c
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : $(HEADER_DIR)*.h mod all clean install uninstall 

all : check $(NAME)
	@mkdir -p ./$(BUNDLE)
	@cp ./*.ttl ./$(BUNDLE)
	@mv ./*.so ./$(BUNDLE)
	@if [ -f ./$(BUNDLE)/$(NAME).so ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo $(RED)"sorry, build failed"; fi
	@echo $(NONE)

mod : 
	@echo "Not implemented for Gxgain"

check :
ifdef ARMCPU
	@echo $(RED)ARM CPU DEDECTED, please check the optimization flags
	@echo $(NONE)
endif

clean :
	@rm -f $(NAME).so
	@rm -rf ./$(BUNDLE)
	@echo ". ." $(BLUE)", clean up"$(NONE)

dist-clean :
	@rm -f $(NAME).so
	@rm -rf ./$(BUNDLE)
	@echo ". ." $(BLUE)", clean up"$(NONE)

install :
ifneq ("$(wildcard ./$(BUNDLE))","")
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	cp -r ./$(BUNDLE)/* $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	@echo ". ." $(BLUE)", done"$(NONE)
else
	@echo ". ." $(BLUE)", you must build first"$(NONE)
endif

uninstall :
	@rm -rf $(INSTALL_DIR)/Xmonc.lv2
	@rm -rf $(INSTALL_DIR)/$(BUNDLE)
	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) :
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).so
	$(CC) $(CXXFLAGS) -Wl,-z,nodelete $(GUI_OBJECTS) $(GUI_LDFLAGS) -o $(NAME)_ui.so
	$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME).so
	$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME)_ui.so

nogui :
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).so
	$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME).so

doc:
	#pass
