################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Src/Startup/startup_stm32f411retx.s 

S_DEPS += \
./Core/Src/Startup/startup_stm32f411retx.d 

OBJS += \
./Core/Src/Startup/startup_stm32f411retx.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Startup/%.o: ../Core/Src/Startup/%.s Core/Src/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -u _printf_float -u _scanf_float -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-Src-2f-Startup

clean-Core-2f-Src-2f-Startup:
	-$(RM) ./Core/Src/Startup/startup_stm32f411retx.d ./Core/Src/Startup/startup_stm32f411retx.o

.PHONY: clean-Core-2f-Src-2f-Startup

