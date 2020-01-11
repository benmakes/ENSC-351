################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../terminal.cc 

CPP_SRCS += \
../PeerX.cpp \
../ReceiverSS.cpp \
../ReceiverX.cpp \
../SenderSS.cpp \
../SenderX.cpp \
../myIO.cpp 

CC_DEPS += \
./terminal.d 

OBJS += \
./PeerX.o \
./ReceiverSS.o \
./ReceiverX.o \
./SenderSS.o \
./SenderX.o \
./myIO.o \
./terminal.o 

CPP_DEPS += \
./PeerX.d \
./ReceiverSS.d \
./ReceiverX.d \
./SenderSS.d \
./SenderX.d \
./myIO.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

ReceiverSS.o: ../ReceiverSS.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -Wno-unknown-pragmas -MMD -MP -MF"$(@:%.o=%.d)" -MT"ReceiverSS.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

SenderSS.o: ../SenderSS.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-unused-variable -Wno-unknown-pragmas -MMD -MP -MF"$(@:%.o=%.d)" -MT"SenderSS.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/home/osboxes/git2/ensc351lib/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


