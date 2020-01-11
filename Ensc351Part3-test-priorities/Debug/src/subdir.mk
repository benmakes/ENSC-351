################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part3-test.cpp \
../src/posixThread.cpp 

OBJS += \
./src/Ensc351Part3-test.o \
./src/posixThread.o 

CPP_DEPS += \
./src/Ensc351Part3-test.d \
./src/posixThread.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/mnt/hgfs/VMsf/workspace-cpp-OxygenSeptember2/Ensc351Part2SolnLib" -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -fsanitize=address       -fsanitize=leak -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


