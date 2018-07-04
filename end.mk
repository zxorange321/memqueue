dirs:=$(shell find . -maxdepth 1 -type d)
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))
SUBDIRS ?= $(dirs)


TARGET?= $(OBJ_NAME) $(LIBSHARED) $(LIBSTATIC)

all: $(OBJ_NAME) $(LIBSHARED) $(LIBSTATIC) subdirs


subdirs:
ifneq ($(SUBDIRS), )
	-$(call dosubdirs)
endif


ifneq ($(LIBSHARED),)
objects := $(wildcard *.c)
objects += $(wildcard *.cpp)
endif

ifneq ($(OBJ_NAME),)
$(OBJ_NAME):$(objects) subdirs 
	$(CC) -o $@ $(objects) $(LINK) 
else
ifneq ($(LIBSHARED),)
$(LIBSHARED):  subdirs
	$(CC) -o $@ $(objects)  $(LINK) -fPIC -shared
endif
ifneq ($(LIBSTATIC),)
$(LIBSTATIC):$(objects) subdirs
	$(AR) rc $@ $(objects) $(LINK) 
endif
endif

%.o: %.c
	$(CC) $(OFLAGS) $@ $(CFLAGS) $< 

%.o: %.cpp
	$(CPP) $(OFLAGS) $@ $(CFLAGS) $< 



.PHONY : clean  install strip dist distclean test
clean: 
	$(call dosubdirs, $@)
	-$(RM) *.o *.log *~ a.out $(TARGET)

install: 
	$(call dosubdirs, $@)
	mkdir -p $(INSTALL_LOCAL_PATH)
ifneq ($(TARGET),)
	-$(CP) $(TARGET) $(INSTALL_LOCAL_PATH)
endif
ifneq ($(SCRIPT),)
	-$(CP) $(SCRIPT) $(INSTALL_LOCAL_PATH)
endif
	-$(CP) $(INSTALL_LOCAL) $(INSTALL_SYSTEM)

ifneq ($(DEPENDLIB),)
	mkdir -p  $(INSTALL_LOCAL_LIB)
	-$(CP) $(DEPENDLIB)/* $(INSTALL_LOCAL_LIB)
endif
	-$(CP) $(INSTALL_LOCAL) $(INSTALL_SYSTEM)

strip: $(TARGET)
	-$(call dosubdirs, $@)
ifneq ($(strip $(TARGET)),)
	-$(STRIP) $<
endif

dist: 
	$(call dosubdirs, $@)
	mkdir -p $(PACKAGE_LOCAL)
	cd $(INSTALL_LOCAL);  $(TAR) $(PACKAGE_LOCAL)/$(ARCHIVE).tar *

distclean: clean
	$(call dosubdirs, $@)
	$(RM) $(INSTALL_LOCAL)
	$(RM) $(PACKAGE_LOCAL)

test:
	$(info $(TARGET))
	$(info $(CC))

define dosubdirs
	for dir in $(SUBDIRS);\
	do make ARCH=mips CROSS_COMPILE=$(CROSS_COMPILE)  -C $$dir $1 -j10 ||exit 1;\
	done
endef
