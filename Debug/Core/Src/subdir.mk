################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/SH1106.c \
../Core/Src/bldcMotor.c \
../Core/Src/controlPPM.c \
../Core/Src/flightController.c \
../Core/Src/fonts.c \
../Core/Src/freertos.c \
../Core/Src/gpsSystem.c \
../Core/Src/gyroscope.c \
../Core/Src/main.c \
../Core/Src/serialManager.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/wpManager.c \
../Core/Src/wpStorage.c 

C_DEPS += \
./Core/Src/SH1106.d \
./Core/Src/bldcMotor.d \
./Core/Src/controlPPM.d \
./Core/Src/flightController.d \
./Core/Src/fonts.d \
./Core/Src/freertos.d \
./Core/Src/gpsSystem.d \
./Core/Src/gyroscope.d \
./Core/Src/main.d \
./Core/Src/serialManager.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/wpManager.d \
./Core/Src/wpStorage.d 

OBJS += \
./Core/Src/SH1106.o \
./Core/Src/bldcMotor.o \
./Core/Src/controlPPM.o \
./Core/Src/flightController.o \
./Core/Src/fonts.o \
./Core/Src/freertos.o \
./Core/Src/gpsSystem.o \
./Core/Src/gyroscope.o \
./Core/Src/main.o \
./Core/Src/serialManager.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/wpManager.o \
./Core/Src/wpStorage.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -u _scanf_float -u _printf_float -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/SH1106.cyclo ./Core/Src/SH1106.d ./Core/Src/SH1106.o ./Core/Src/SH1106.su ./Core/Src/bldcMotor.cyclo ./Core/Src/bldcMotor.d ./Core/Src/bldcMotor.o ./Core/Src/bldcMotor.su ./Core/Src/controlPPM.cyclo ./Core/Src/controlPPM.d ./Core/Src/controlPPM.o ./Core/Src/controlPPM.su ./Core/Src/flightController.cyclo ./Core/Src/flightController.d ./Core/Src/flightController.o ./Core/Src/flightController.su ./Core/Src/fonts.cyclo ./Core/Src/fonts.d ./Core/Src/fonts.o ./Core/Src/fonts.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/gpsSystem.cyclo ./Core/Src/gpsSystem.d ./Core/Src/gpsSystem.o ./Core/Src/gpsSystem.su ./Core/Src/gyroscope.cyclo ./Core/Src/gyroscope.d ./Core/Src/gyroscope.o ./Core/Src/gyroscope.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/serialManager.cyclo ./Core/Src/serialManager.d ./Core/Src/serialManager.o ./Core/Src/serialManager.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/wpManager.cyclo ./Core/Src/wpManager.d ./Core/Src/wpManager.o ./Core/Src/wpManager.su ./Core/Src/wpStorage.cyclo ./Core/Src/wpStorage.d ./Core/Src/wpStorage.o ./Core/Src/wpStorage.su

.PHONY: clean-Core-2f-Src

