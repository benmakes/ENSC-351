################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part1.cpp \
../src/PeerX.cpp \
../src/SenderX.cpp \
../src/myIO.cpp 

OBJS += \
./src/Ensc351Part1.o \
./src/PeerX.o \
./src/SenderX.o \
./src/myIO.o 

CPP_DEPS += \
./src/Ensc351Part1.d \
./src/PeerX.d \
./src/SenderX.d \
./src/myIO.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	powerpc-linux-gnu-g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


