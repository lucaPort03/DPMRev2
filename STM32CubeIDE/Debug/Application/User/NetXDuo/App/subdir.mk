################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/lport/STM32Cube/Example/DPMProj/NetXDuo/App/app_netxduo.c 

OBJS += \
./Application/User/NetXDuo/App/app_netxduo.o 

C_DEPS += \
./Application/User/NetXDuo/App/app_netxduo.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/NetXDuo/App/app_netxduo.o: C:/Users/lport/STM32Cube/Example/DPMProj/NetXDuo/App/app_netxduo.c Application/User/NetXDuo/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DTX_INCLUDE_USER_DEFINE_FILE -DNX_INCLUDE_USER_DEFINE_FILE -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../../Core/Inc -I../../AZURE_RTOS/App -I../../NetXDuo/App -I../../NetXDuo/Target -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/Components/lan8742/ -I../../Middlewares/ST/netxduo/common/drivers/ethernet/ -I../../Middlewares/ST/netxduo/addons/dhcp/ -I../../Middlewares/ST/netxduo/common/inc/ -I../../Middlewares/ST/netxduo/ports/cortex_m7/gnu/inc/ -I../../Middlewares/ST/threadx/common/inc/ -I../../Middlewares/ST/threadx/ports/cortex_m7/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-User-2f-NetXDuo-2f-App

clean-Application-2f-User-2f-NetXDuo-2f-App:
	-$(RM) ./Application/User/NetXDuo/App/app_netxduo.cyclo ./Application/User/NetXDuo/App/app_netxduo.d ./Application/User/NetXDuo/App/app_netxduo.o ./Application/User/NetXDuo/App/app_netxduo.su

.PHONY: clean-Application-2f-User-2f-NetXDuo-2f-App

