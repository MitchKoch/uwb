#pragma once

#define STM32F302x8

#define VAL_GPIOA_MODER (0xA8000000 | 0x00000004)
#define VAL_GPIOB_MODER 0x00000280
#define VAL_GPIOC_MODER 0x00000000
#define VAL_GPIOD_MODER 0x00000000
#define VAL_GPIOE_MODER 0x00000000
#define VAL_GPIOF_MODER 0x00000000
#define VAL_GPIOG_MODER 0x00000000
#define VAL_GPIOH_MODER 0x00000000

#define VAL_GPIOA_OTYPER 0x00000000
#define VAL_GPIOB_OTYPER 0x00000000
#define VAL_GPIOC_OTYPER 0x00000000
#define VAL_GPIOD_OTYPER 0x00000000
#define VAL_GPIOE_OTYPER 0x00000000
#define VAL_GPIOF_OTYPER 0x00000000
#define VAL_GPIOG_OTYPER 0x00000000
#define VAL_GPIOH_OTYPER 0x00000000

#define VAL_GPIOA_OSPEEDR 0xC0000000
#define VAL_GPIOB_OSPEEDR 0x000000C0
#define VAL_GPIOC_OSPEEDR 0x00000000
#define VAL_GPIOD_OSPEEDR 0x00000000
#define VAL_GPIOE_OSPEEDR 0x00000000
#define VAL_GPIOF_OSPEEDR 0x00000000
#define VAL_GPIOG_OSPEEDR 0x00000000
#define VAL_GPIOH_OSPEEDR 0x00000000

#define VAL_GPIOA_PUPDR 0x64000000
#define VAL_GPIOB_PUPDR 0x00000100
#define VAL_GPIOC_PUPDR 0x0C000000
#define VAL_GPIOD_PUPDR 0x0C000000
#define VAL_GPIOE_PUPDR 0x0C000000
#define VAL_GPIOF_PUPDR 0x0C000000
#define VAL_GPIOG_PUPDR 0x0C000000
#define VAL_GPIOH_PUPDR 0x0C000000

#define VAL_GPIOA_ODR 0x00000000
#define VAL_GPIOB_ODR 0x00000000
#define VAL_GPIOC_ODR 0x00000000
#define VAL_GPIOD_ODR 0x00000000
#define VAL_GPIOE_ODR 0x00000000
#define VAL_GPIOF_ODR 0x00000000
#define VAL_GPIOG_ODR 0x00000000
#define VAL_GPIOH_ODR 0x00000000

#define VAL_GPIOA_AFRL 0x00000000
#define VAL_GPIOB_AFRL 0x00000000
#define VAL_GPIOC_AFRL 0x00000000
#define VAL_GPIOD_AFRL 0x00000000
#define VAL_GPIOE_AFRL 0x00000000
#define VAL_GPIOF_AFRL 0x00000000
#define VAL_GPIOG_AFRL 0x00000000
#define VAL_GPIOH_AFRL 0x00000000

#define VAL_GPIOA_AFRH 0x00000000
#define VAL_GPIOB_AFRH 0x00000000
#define VAL_GPIOC_AFRH 0x00000000
#define VAL_GPIOD_AFRH 0x00000000
#define VAL_GPIOE_AFRH 0x00000000
#define VAL_GPIOF_AFRH 0x00000000
#define VAL_GPIOG_AFRH 0x00000000
#define VAL_GPIOH_AFRH 0x00000000

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
    #endif
    void boardInit(void);
    #ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */