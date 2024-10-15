# Some utility macros for manipulating awkward (whitespace) characters.
blank			:=
space			:=${blank} ${blank}

# A user defined function to search for source files below a directory
#    $1 is the directory
#    $2 is the postfix
define swildcard
$(strip $(foreach pf,${2},$(wildcard $(if $1,${1}/)*${pf})))
endef

# A user defined function to recursively search for a filename below a directory
#    $1 is the directory root of the recursive search (blank for current directory).
#    $2 is the file name to search for.
define rwildcard
$(strip $(foreach d,$(wildcard ${1}*),$(call rwildcard,${d}/,${2}) $(filter $(subst *,%,%${2}),${d})))
endef

# This table is used in converting lower case to upper case.
uppercase_table:=a,A b,B c,C d,D e,E f,F g,G h,H i,I j,J k,K l,L m,M n,N o,O p,P q,Q r,R s,S t,T u,U v,V w,W x,X y,Y z,Z

# Internal macro used for converting lower case to upper case.
#   $(1) = upper case table
#   $(2) = String to convert
define uppercase_internal
$(if $(1),$$(subst $(firstword $(1)),$(call uppercase_internal,$(wordlist 2,$(words $(1)),$(1)),$(2))),$(2))
endef

# A macro for converting a string to upper case
#   $(1) = String to convert
define uppercase
$(eval uppercase_result:=$(call uppercase_internal,$(uppercase_table),$(1)))$(uppercase_result)
endef

# Convenience function for adding build definitions
# $(eval $(call add_define,FOO)) will have:
# -DFOO if $(FOO) is empty; -DFOO=$(FOO) otherwise
define add_define
    DEFINES			+=	-D$(1)$(if $(value $(1)),=$(value $(1)),)
endef

# Convenience function for adding build definitions
# $(eval $(call add_define_val,FOO,BAR)) will have:
# -DFOO=BAR
define add_define_val
    DEFINES			+=	-D$(1)=$(2)
endef

# Convenience function for verifying option has a boolean value
# $(eval $(call assert_boolean,FOO)) will assert FOO is 0 or 1
define assert_boolean
    $(and $(patsubst 0,,$(value $(1))),$(patsubst 1,,$(value $(1))),$(error $(1) must be boolean))
endef

###
# fcheck is used to check if the content of a generated file is updated.
# Sample usage:
# define fcheck_sample
#	echo $SAMPLE
# endef
# version.h : Makefile
#	$(call fcheck,sample)
# The rule defined shall write to stdout the content of the new file.
# The existing file will be compared with the new one.
# - If no file exist it is created
# - If the content differ the new file is used
# - If they are equal no change, and no timestamp update
#
define fcheck
	$(Q)set -e;				\
	echo '  CHK     $@';			\
	mkdir -p $(dir $@);			\
	$(fcheck_$(1)) > $@.tmp;		\
	if [ -r $@ ] && cmp -s $@ $@.tmp; then	\
		echo '  rm     $@.tmp';		\
		rm -f $@.tmp;			\
	else					\
		echo '  UPD     $@';		\
		mv -f $@.tmp $@;		\
	fi
endef

#
# Generate secure partition link script tool
# $1 - ldscript
# $2 - spe.bin
# $3 - parttool
# $4 - config file
#
define fcheck_ldscript
	(set -e;			\
	 $(3) -g -f $(2) -c $(4) -o $@.ld.tmp;	\
	 cat $@.ld.tmp;			\
	 rm -f $@.ld.tmp;		\
	)
endef

#
# Generate secure partition manifest tool
# $1 - manifest
# $2 - manifest.json
# $3 - parttool
#
define fcheck_manifest
	(set -e;			\
	 $(3) -m -i $(2) -o $@.man.tmp;	\
	 cat $@.man.tmp;		\
	 rm -f $@.man.tmp;		\
	)
endef

#
# process one src file
# $1 - src file
# $2 - obj file
#
define PROCESS_SRC
	$(eval OBJS	+= $2)
	$(eval DEP_$2	:= $$(dir $2).$$(notdir $2).d)
	$(eval CLEANFILES += $(DEP_$2) $2)

ifeq ($$(filter %.c,$1),$1)
	TESTSTR	= "c file"
	CMD_$2	= CC
	FLAGS_$2	= $(COMP_CFLAGS) $(CFLAGS_$(TARGET)) $(CFLAGS_$2)
else ifeq ($$(filter %.S,$1),$1)
	TESTSTR	= "S file"
	CMD_$2	= AS
	FLAGS_$2	= $(COMP_ASFLAGS) $(ASFLAGS_$(TARGET)) $(ASFLAGS_$2)
else
	TESTSTR	:= "unknown file"
	$$(error "Unknown src $1")
endif

	FLAGS_$2	+= -MD -MF $$(DEP_$2) -MT $$@
	FLAGS_$2	+= $(COMP_CPPFLAGS) $(CPPFLAGS_$2)

	PROCESS_$2 = $$($$(CMD_$2)) $$(FLAGS_$2) -c $$< -o $$@

$2: $1
	@set -e;	\
	mkdir -p $$(dir $2);	\
	$$(CMD_ECHO_SILENT) '  $$(CMD_$2)      $$<';	\
	$$(CMD_ECHO) $$(subst \",\\\",$$(PROCESS_$2));	\
	$$(PROCESS_$2);

-include $(DEP_$2)

endef #PROCESS_SRC

#
# $1 - library name
# $2 - objs
# $3 - lib output file
#
define PROCESS_LIB
	$(eval LIB_LIBFILE	:= $(if $(strip $3),$3,$(OUT_DIR)/$(TARGET_OUT)/lib$1.a))
	$(eval LIB_OBJS		:= $2)
	$(eval CLEANFILES	+= $(LIB_LIBFILE))
	$(eval LIBFILES		+= $(LIB_LIBFILE))
	$(eval LIBDIRS 		+= $(OUT_DIR)/$(TARGET_OUT))
	$(eval LIBNAMES		+= $(LIBNAME))
	$(eval LIBDEPS		+= $(LIB_LIBFILE))

$(LIB_LIBFILE): $(OBJS) $(DEPLIB) $(DEPADD)
	@echo '  LD      $$@'
	@mkdir -p $$(dir $$@)
	$$(Q)$$(LD) -r $(LDFLAGS_$1) -o $$@ $(OBJS) $(DEPLIB)
	@echo
	@echo "Build $$(notdir $$@) successfully"
	@echo

lib$(1): $(LIB_LIBFILE)

endef #PROCESS_LIB

#
# $1 binary name
# $2 objs
# $3 ld_src
#
define PROCESS_BIN
	$(eval $1_OBJS	:= $2)
	$(eval $1_LDSRC	:= $3)
	$(eval $1_LD	:= $(BIN_OUT)/$1.ld)
	$(eval $1_BIN	:= $(BIN_OUT)/$1.bin)
	$(eval $1_SYM	:= $(BIN_OUT)/$1.sym)
	$(eval $1_ELF	:= $(BIN_OUT)/$1.elf)
	$(eval $1_HEX	:= $(BIN_OUT)/$1.hex)
	$(eval $1_MAP	:= $(BIN_OUT)/$1.map)
	$(eval $1_DUMP	:= $(BIN_OUT)/$1.dump)
	$(eval $1_DUMPALL	:= $(BIN_OUT)/$1.dumpall)
	$(eval $1_SECTION	:= $(BIN_OUT)/$1.section)
	$(eval $1_LDFLAGS	:= $(LDFLAGS_$1) $(LNK_LDFLAGS))
	$(eval CLEANFILES	+= $($1_LD) $($1_BIN) \
				   $($1_SYM) $($1_ELF) $($1_MAP) $($1_DUMP) \
				   $($1_DUMPALL) $($1_SECTION) $($1_HEX))

$($1_LD): $($1_LDSRC)
	@set -e;	\
	mkdir -p $$(dir $$@);	\
	$$(CMD_ECHO_SILENT) '  LD      $$@';
	$$(Q)$$(AS) -P -E -D__ASSEMBLY__ $$(ASFLAGS) $$(ASFLAGS_$1) $$< -o $$@

$($1_ELF): $($1_OBJS) $($1_LD) $(DEPLIB) $(DEPADD)
	$$(Q)set -e;	\
	mkdir -p $$(dir $$@);
	@$$(CMD_ECHO_SILENT) '  LD      $$@';
	$$(Q)$$(LD) $($1_LDFLAGS) -Map=$($1_MAP) -T $($1_LD) -o $$@ --start-group \
		$($1_OBJS) $(DEPLIB) --end-group
	$$(Q)$$(OC) -O ihex $($1_ELF) $($1_HEX)

$($1_BIN): $($1_ELF)
	@$$(CMD_ECHO_SILENT) '  BIN     $$@'
	$$(Q)$$(OC) -S -O binary $$< $$@

$($1_DUMP): $($1_ELF)
	@$$(CMD_ECHO_SILENT) '  OD      $$@'
	$$(Q)$$(OD) -xd $$< > $$@

$($1_DUMPALL): $($1_ELF)
	@$$(CMD_ECHO_SILENT) '  OD      $$@'
	$$(Q)$$(OD) -D $$< > $$@

$($1_SECTION): $($1_ELF)
	@$$(CMD_ECHO_SILENT) '  OD      $$@'
	$$(Q)$$(OD) -h $$< > $$@

$($1_SYM): $($1_ELF)
	@$$(CMD_ECHO_SILENT) '  NM      $$@'
	$$(Q)$$(NM) -n $$< > $$@

.PHONY: $(1)
$(1): $($1_BIN) $($1_DUMP) $($1_DUMPALL) $($1_SECTION) $($1_SYM)
	@echo
	@echo "Build $$@ successfully"
	@echo

all: $(1)

endef #PROCESS_BIN

#
# $1 - SP name
# $2 - SP srcs
# $3 - SP extra dependency lib
# $4 - SP lib output file
# $5 - SP directory
#
define PROCESS_SP_LIB
	$(eval LIBNAME			:= $1)
	$(eval SRCS				:= $(sort $2))
	$(eval TARGET_OUT		:= sps/$1)

	$(eval MANIFEST_$(LIBNAME)		:= $(if $(strip $5),${5}/)manifest.json)
	#$(eval MANIFEST_H_$(LIBNAME)	:= $(if $(strip $5),${5}/)manifest.h)
	$(eval MANIFEST_H_$(LIBNAME)	:= $(OUT_DIR)/$(TARGET_OUT)/manifest.h)

	$(eval CFLAGS_$(LIBNAME)	:= $(SP_COM_FLAGS) $(SP_INCLUDES)	\
				   					$(SP_CFLAGS) -I$(OUT_DIR)/$(TARGET_OUT))
	$(eval ASFLAGS_$(LIBNAME)	:= $(SP_COM_FLAGS) $(SP_INCLUDES)	\
				   					$(SP_ASFLAGS) -I$(OUT_DIR)/$(TARGET_OUT))
	$(eval LDFLAGS_$(LIBNAME)	:= $(SP_COM_LDFLAGS) $(SP_LDFLAGS))

	$(eval CLEANFILES	+= $(MANIFEST_H_$(LIBNAME)))

	$(eval conf-file	:= $(MANIFEST_H_$(LIBNAME)))
	$(eval DEPADD		:= $(MANIFEST_H_$(LIBNAME)))
	$(eval DEPLIB		:= $(SPESVC_LIB) $(3))
	$(eval LIB_OUT_FILE			:= $(4))
	$(eval SP_LIB_$(LIBNAME)	:= $(LIB_OUT_FILE))
	$(eval ROT_SERVICES			+= $(if $(findstring Y, $(SP_BUILTIN)), $(LIB_OUT_FILE)))

$(MANIFEST_H_$(LIBNAME)): $(MANIFEST_$(LIBNAME)) $(PARTTOOL)
	$$(call fcheck,manifest,$(MANIFEST_$(LIBNAME)),$(PARTTOOL))

include $(mk-dir)/lib.mk

endef # PROCESS_SP_LIB

#
# $1 - SP name
# $2 - SP dependency lib
#
define PROCESS_SP_BIN
	$(eval BINARY			:= $(1))
	$(eval TARGET_OUT		:= sps/$(BINARY))
	$(eval LDSCRIPT			:= $(OUT_DIR)/$(TARGET_OUT)/$(BINARY).ld.S)
	$(eval BIN_$(BINARY)	:= $(OUT_DIR)/$(TARGET_OUT)/$(BINARY).bin)
	$(eval BUILT_IN_SPS		+= $(if $(findstring Y, $(SP_BUILTIN)), $(BIN_$(BINARY))))

	$(eval CFLAGS_$1	:= $(SP_COM_FLAGS) $(SP_INCLUDES)	\
 							$(SP_CFLAGS))
	$(eval ASFLAGS_$1	:= $(SP_COM_FLAGS) $(SP_INCLUDES)	\
 	   						$(SP_ASFLAGS))
	$(eval LDFLAGS_$1	:= $(SP_COM_LDFLAGS))

	$(eval CLEANFILES	+= $(LDSCRIPT))
	$(eval DEPADD		:= $(3))
	$(eval DEPLIB		:= $(2))
	$(eval SP_BUILTIN	:= )

include $(mk-dir)/link.mk

.PHONY: all sps append_$(BINARY)
all sps: append_$(BINARY) builtin_sps
all: sps
sps: append_$(BINARY) builtin_sps

$(LDSCRIPT): $(PARTTOOL) $(SPE_BIN)
	$$(call fcheck,ldscript,$(SPE_BIN),$(PARTTOOL),$(PARTTOOL_CONFIG))

append_$(1): $(1) $(SPE_BIN) $(PARTTOOL)
	@$(CMD_ECHO_SILENT) '  APPEND  $(BIN_$(1))'
	$(Q)$$(PARTTOOL) -a -f $(SPE_BIN) -c $(PARTTOOL_CONFIG) -i $(BIN_$(1))
	@#$$(Q)$(PARTTOOL) -l -f $(SPE_BIN) -c $(PARTTOOL_CONFIG)

endef # PROCESS_SP_BIN

