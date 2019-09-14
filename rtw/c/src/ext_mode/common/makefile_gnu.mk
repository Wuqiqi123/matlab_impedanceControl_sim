# File : rtw/c/src/ext_mode/common/makefile_gnu.mk
#
# Copyright 2007-2008 The MathWorks, Inc.

PRODUCT                  := simulink

build :

TARGETS = 

PREBUILD_TARGETS = ext_share.h

prebuild: $(PREBUILD_TARGETS)

ext_share.h : ../../../../../src/simulink/export/include/simulink/ext_share.h
	rm -f $@
	cp $< $@

cleanprebuild :
	rm -f $(PREBUILD_TARGETS)
