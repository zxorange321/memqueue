CROSS_COMPILE = mipsel-linux-
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP     = $(CROSS_COMPILE)g++
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
STRIP       = $(CROSS_COMPILE)strip
OBJCOPY     = $(CROSS_COMPILE)objcopy
OBJDUMP     = $(CROSS_COMPILE)objdump
AWK     = awk
MAKE	= make
RM		= rm -rf
TAR		= tar -cvf
CP		= cp -dPrvf


PROJECT_PATH:=$(shell pwd)
PROJECT_NAME:=$(notdir $(PROJECT_PATH))

INSTALL_LOCAL:=$(PROJECT_PATH)/__install
INSTALL_SYSTEM:=$(if $(INSTALL_SYSTEM), $(INSTALL_SYSTEM), $(PROJECT_PATH)/..)
INSTALL_LOCAL_PATH=$(INSTALL_LOCAL)/usr/bin
INSTALL_LOCAL_LIB=$(INSTALL_LOCAL)/usr/lib
PACKAGE_LOCAL:=$(PROJECT_PATH)/__package
VERSION   = ` date +"%Y.%m%d" `
ARCHIVE   = $(PROJECT_NAME)-$(VERSION)

exclude_dirs = __package __install
OBJ_NAME 	:= $(PROJECT_NAME)
LIBSHARED	:=
LIBSTATIC	:=
SCRIPT		:=
DEPENDLIB	:=

CFLAGS := -c -I$(strip $(INSTALL_SYSTEM))/include -I.
LINK := -L. -L$(INSTALL_SYSTEM)/salt_libconf -lconf
OFLAGS:= -o

objects	:= $(patsubst %.c,%.o,$(wildcard *.c))
objects	+= $(patsubst %.cpp,%.o,$(wildcard *.cpp))
