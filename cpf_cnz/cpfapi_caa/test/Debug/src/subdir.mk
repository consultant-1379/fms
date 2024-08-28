################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Test.cpp \
../src/cute_fms_cpf_createfile_t1.cpp 

OBJS += \
./src/Test.o \
./src/cute_fms_cpf_createfile_t1.o 

CPP_DEPS += \
./src/Test.d \
./src/cute_fms_cpf_createfile_t1.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/root/workspace/UT_CPF/cute" -I/vobs/IO_Developments/BOOST_SDK/boost_1_44_0/include -I"/root/workspace/cpf_cnz/cpfapi_caa/inc" -I/vobs/IO_Developments/AP_SDK/Include -I/vobs/IO_Developments/ACE_SDK/ACE_wrappers -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


