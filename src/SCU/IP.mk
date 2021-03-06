local_dir	:= src/SCU
lib_dir		:= $(local_dir)
lib_name	:= #??.a

local_lib	:= #$(lib_dir)/$(lib_name)

# compile source
local_src	:= scu-ctrl.c
lib_src		:= 

ifeq "y" "$(CONFIG_CDK)"
local_src	+= cdk-scu-ctrl.c cdk-scu-menu.c
endif

ifeq "y" "$(CONFIG_PC9002)"
local_src	+= cdk-scu-ctrl.c cdk-scu-menu.c
endif

ifeq "y" "$(CONFIG_SCDK)"
local_src	+= cdk-scu-ctrl.c cdk-scu-menu.c
endif

ifeq "y" "$(CONFIG_SQ8000)"
local_src	+= sq8000-scu-ctrl.c sq8000-scu-menu.c
endif

include build/common.mk
