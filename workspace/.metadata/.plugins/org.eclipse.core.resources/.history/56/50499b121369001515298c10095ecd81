################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/bsp_init.c \
../src/cpu1_uc.c 

LD_SRCS += \
../src/lscript.ld 

OBJS += \
./src/bsp_init.o \
./src/cpu1_uc.o 

C_DEPS += \
./src/bsp_init.d \
./src/cpu1_uc.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo Building file: $<
	@echo Invoking: ARM gcc compiler
	arm-xilinx-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -I../../app_cpu1_bsp/ps7_cortexa9_1/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


