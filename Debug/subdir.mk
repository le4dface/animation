################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../FileIO.cpp \
../G308_Skeleton.cpp \
../main.cpp \
../quaternion.cpp 

OBJS += \
./FileIO.o \
./G308_Skeleton.o \
./main.o \
./quaternion.o 

CPP_DEPS += \
./FileIO.d \
./G308_Skeleton.d \
./main.d \
./quaternion.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


