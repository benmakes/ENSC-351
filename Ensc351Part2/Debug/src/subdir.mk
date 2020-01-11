################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part2.cpp \
../src/Medium.cpp \
../src/PeerX.cpp \
../src/ReceiverX.cpp \
../src/SenderX.cpp \
../src/myIO.cpp 

OBJS += \
./src/Ensc351Part2.o \
./src/Medium.o \
./src/PeerX.o \
./src/ReceiverX.o \
./src/SenderX.o \
./src/myIO.o 

CPP_DEPS += \
./src/Ensc351Part2.d \
./src/Medium.d \
./src/PeerX.d \
./src/ReceiverX.d \
./src/SenderX.d \
./src/myIO.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


