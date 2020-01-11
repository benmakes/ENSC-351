################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Kvm.cpp \
../src/Medium.cpp \
../src/main3B.cpp 

OBJS += \
./src/Kvm.o \
./src/Medium.o \
./src/main3B.o 

CPP_DEPS += \
./src/Kvm.d \
./src/Medium.d \
./src/main3B.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/mnt/hgfs/VMsf/workspace-cpp-OxygenSeptember2/Ensc351xmodLib" -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -Wno-unknown-pragmas -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


