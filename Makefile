SDK_DEMO_PATH ?= .
BL_SDK_BASE ?= D:\AiPi-Open-Kits\aithinker_Ai-M6X_SDK

export BL_SDK_BASE

CHIP ?= bl616
BOARD ?= bl616dk
CROSS_COMPILE ?= riscv64-unknown-elf-

# add custom cmake definition
#cmake_definition+=-Dxxx=sss

include $(BL_SDK_BASE)/project.build
