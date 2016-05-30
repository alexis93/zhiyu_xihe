################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/gnss_config.c \
../src/gnss_mempool.c \
../src/gnss_queue.c \
../src/transfer.c 

OBJS += \
./src/gnss_config.o \
./src/gnss_mempool.o \
./src/gnss_queue.o \
./src/transfer.o 

C_DEPS += \
./src/gnss_config.d \
./src/gnss_mempool.d \
./src/gnss_queue.d \
./src/transfer.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/jin/workspace/transfer_server/src" -O0 -g3 -Wall -m32 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


