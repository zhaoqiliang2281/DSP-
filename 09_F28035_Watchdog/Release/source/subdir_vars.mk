################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../source/DSP2803x_CSMPasswords.asm \
../source/DSP2803x_CodeStartBranch.asm \
../source/DSP2803x_DBGIER.asm \
../source/DSP2803x_DisInt.asm \
../source/DSP2803x_usDelay.asm 

C_SRCS += \
../source/DSP2803x_Adc.c \
../source/DSP2803x_Comp.c \
../source/DSP2803x_CpuTimers.c \
../source/DSP2803x_DefaultIsr.c \
../source/DSP2803x_ECan.c \
../source/DSP2803x_ECap.c \
../source/DSP2803x_EPwm.c \
../source/DSP2803x_EQep.c \
../source/DSP2803x_GlobalVariableDefs.c \
../source/DSP2803x_Gpio.c \
../source/DSP2803x_I2C.c \
../source/DSP2803x_Lin.c \
../source/DSP2803x_MemCopy.c \
../source/DSP2803x_OscComp.c \
../source/DSP2803x_PieCtrl.c \
../source/DSP2803x_PieVect.c \
../source/DSP2803x_Sci.c \
../source/DSP2803x_Spi.c \
../source/DSP2803x_SysCtrl.c \
../source/DSP2803x_TempSensorConv.c 

C_DEPS += \
./source/DSP2803x_Adc.d \
./source/DSP2803x_Comp.d \
./source/DSP2803x_CpuTimers.d \
./source/DSP2803x_DefaultIsr.d \
./source/DSP2803x_ECan.d \
./source/DSP2803x_ECap.d \
./source/DSP2803x_EPwm.d \
./source/DSP2803x_EQep.d \
./source/DSP2803x_GlobalVariableDefs.d \
./source/DSP2803x_Gpio.d \
./source/DSP2803x_I2C.d \
./source/DSP2803x_Lin.d \
./source/DSP2803x_MemCopy.d \
./source/DSP2803x_OscComp.d \
./source/DSP2803x_PieCtrl.d \
./source/DSP2803x_PieVect.d \
./source/DSP2803x_Sci.d \
./source/DSP2803x_Spi.d \
./source/DSP2803x_SysCtrl.d \
./source/DSP2803x_TempSensorConv.d 

OBJS += \
./source/DSP2803x_Adc.obj \
./source/DSP2803x_CSMPasswords.obj \
./source/DSP2803x_CodeStartBranch.obj \
./source/DSP2803x_Comp.obj \
./source/DSP2803x_CpuTimers.obj \
./source/DSP2803x_DBGIER.obj \
./source/DSP2803x_DefaultIsr.obj \
./source/DSP2803x_DisInt.obj \
./source/DSP2803x_ECan.obj \
./source/DSP2803x_ECap.obj \
./source/DSP2803x_EPwm.obj \
./source/DSP2803x_EQep.obj \
./source/DSP2803x_GlobalVariableDefs.obj \
./source/DSP2803x_Gpio.obj \
./source/DSP2803x_I2C.obj \
./source/DSP2803x_Lin.obj \
./source/DSP2803x_MemCopy.obj \
./source/DSP2803x_OscComp.obj \
./source/DSP2803x_PieCtrl.obj \
./source/DSP2803x_PieVect.obj \
./source/DSP2803x_Sci.obj \
./source/DSP2803x_Spi.obj \
./source/DSP2803x_SysCtrl.obj \
./source/DSP2803x_TempSensorConv.obj \
./source/DSP2803x_usDelay.obj 

ASM_DEPS += \
./source/DSP2803x_CSMPasswords.d \
./source/DSP2803x_CodeStartBranch.d \
./source/DSP2803x_DBGIER.d \
./source/DSP2803x_DisInt.d \
./source/DSP2803x_usDelay.d 

OBJS__QUOTED += \
"source\DSP2803x_Adc.obj" \
"source\DSP2803x_CSMPasswords.obj" \
"source\DSP2803x_CodeStartBranch.obj" \
"source\DSP2803x_Comp.obj" \
"source\DSP2803x_CpuTimers.obj" \
"source\DSP2803x_DBGIER.obj" \
"source\DSP2803x_DefaultIsr.obj" \
"source\DSP2803x_DisInt.obj" \
"source\DSP2803x_ECan.obj" \
"source\DSP2803x_ECap.obj" \
"source\DSP2803x_EPwm.obj" \
"source\DSP2803x_EQep.obj" \
"source\DSP2803x_GlobalVariableDefs.obj" \
"source\DSP2803x_Gpio.obj" \
"source\DSP2803x_I2C.obj" \
"source\DSP2803x_Lin.obj" \
"source\DSP2803x_MemCopy.obj" \
"source\DSP2803x_OscComp.obj" \
"source\DSP2803x_PieCtrl.obj" \
"source\DSP2803x_PieVect.obj" \
"source\DSP2803x_Sci.obj" \
"source\DSP2803x_Spi.obj" \
"source\DSP2803x_SysCtrl.obj" \
"source\DSP2803x_TempSensorConv.obj" \
"source\DSP2803x_usDelay.obj" 

C_DEPS__QUOTED += \
"source\DSP2803x_Adc.d" \
"source\DSP2803x_Comp.d" \
"source\DSP2803x_CpuTimers.d" \
"source\DSP2803x_DefaultIsr.d" \
"source\DSP2803x_ECan.d" \
"source\DSP2803x_ECap.d" \
"source\DSP2803x_EPwm.d" \
"source\DSP2803x_EQep.d" \
"source\DSP2803x_GlobalVariableDefs.d" \
"source\DSP2803x_Gpio.d" \
"source\DSP2803x_I2C.d" \
"source\DSP2803x_Lin.d" \
"source\DSP2803x_MemCopy.d" \
"source\DSP2803x_OscComp.d" \
"source\DSP2803x_PieCtrl.d" \
"source\DSP2803x_PieVect.d" \
"source\DSP2803x_Sci.d" \
"source\DSP2803x_Spi.d" \
"source\DSP2803x_SysCtrl.d" \
"source\DSP2803x_TempSensorConv.d" 

ASM_DEPS__QUOTED += \
"source\DSP2803x_CSMPasswords.d" \
"source\DSP2803x_CodeStartBranch.d" \
"source\DSP2803x_DBGIER.d" \
"source\DSP2803x_DisInt.d" \
"source\DSP2803x_usDelay.d" 

C_SRCS__QUOTED += \
"../source/DSP2803x_Adc.c" \
"../source/DSP2803x_Comp.c" \
"../source/DSP2803x_CpuTimers.c" \
"../source/DSP2803x_DefaultIsr.c" \
"../source/DSP2803x_ECan.c" \
"../source/DSP2803x_ECap.c" \
"../source/DSP2803x_EPwm.c" \
"../source/DSP2803x_EQep.c" \
"../source/DSP2803x_GlobalVariableDefs.c" \
"../source/DSP2803x_Gpio.c" \
"../source/DSP2803x_I2C.c" \
"../source/DSP2803x_Lin.c" \
"../source/DSP2803x_MemCopy.c" \
"../source/DSP2803x_OscComp.c" \
"../source/DSP2803x_PieCtrl.c" \
"../source/DSP2803x_PieVect.c" \
"../source/DSP2803x_Sci.c" \
"../source/DSP2803x_Spi.c" \
"../source/DSP2803x_SysCtrl.c" \
"../source/DSP2803x_TempSensorConv.c" 

ASM_SRCS__QUOTED += \
"../source/DSP2803x_CSMPasswords.asm" \
"../source/DSP2803x_CodeStartBranch.asm" \
"../source/DSP2803x_DBGIER.asm" \
"../source/DSP2803x_DisInt.asm" \
"../source/DSP2803x_usDelay.asm" 


