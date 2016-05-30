################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/zlog/buf.c \
../src/zlog/category.c \
../src/zlog/category_table.c \
../src/zlog/conf.c \
../src/zlog/event.c \
../src/zlog/format.c \
../src/zlog/level.c \
../src/zlog/level_list.c \
../src/zlog/mdc.c \
../src/zlog/record.c \
../src/zlog/record_table.c \
../src/zlog/rotater.c \
../src/zlog/rule.c \
../src/zlog/spec.c \
../src/zlog/thread.c \
../src/zlog/zc_arraylist.c \
../src/zlog/zc_hashtable.c \
../src/zlog/zc_profile.c \
../src/zlog/zc_util.c \
../src/zlog/zlog-chk-conf.c \
../src/zlog/zlog.c 

OBJS += \
./src/zlog/buf.o \
./src/zlog/category.o \
./src/zlog/category_table.o \
./src/zlog/conf.o \
./src/zlog/event.o \
./src/zlog/format.o \
./src/zlog/level.o \
./src/zlog/level_list.o \
./src/zlog/mdc.o \
./src/zlog/record.o \
./src/zlog/record_table.o \
./src/zlog/rotater.o \
./src/zlog/rule.o \
./src/zlog/spec.o \
./src/zlog/thread.o \
./src/zlog/zc_arraylist.o \
./src/zlog/zc_hashtable.o \
./src/zlog/zc_profile.o \
./src/zlog/zc_util.o \
./src/zlog/zlog-chk-conf.o \
./src/zlog/zlog.o 

C_DEPS += \
./src/zlog/buf.d \
./src/zlog/category.d \
./src/zlog/category_table.d \
./src/zlog/conf.d \
./src/zlog/event.d \
./src/zlog/format.d \
./src/zlog/level.d \
./src/zlog/level_list.d \
./src/zlog/mdc.d \
./src/zlog/record.d \
./src/zlog/record_table.d \
./src/zlog/rotater.d \
./src/zlog/rule.d \
./src/zlog/spec.d \
./src/zlog/thread.d \
./src/zlog/zc_arraylist.d \
./src/zlog/zc_hashtable.d \
./src/zlog/zc_profile.d \
./src/zlog/zc_util.d \
./src/zlog/zlog-chk-conf.d \
./src/zlog/zlog.d 


# Each subdirectory must supply rules for building sources it contributes
src/zlog/%.o: ../src/zlog/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/jin/workspace/transfer_server/src" -O0 -g3 -Wall -m32 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


