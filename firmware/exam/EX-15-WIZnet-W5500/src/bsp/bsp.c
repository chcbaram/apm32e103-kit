#include "bsp.h"
#include "hw_def.h"


volatile static uint32_t systick_ms = 0;

extern void swtimerISR(void);


void SysTick_Handler(void)
{
  systick_ms++;
  swtimerISR();
}




bool bspInit(void)
{
  uint32_t prioritygroup;


  SysTick_Config(SystemCoreClock / 1000U);


  prioritygroup = NVIC_GetPriorityGrouping();
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(prioritygroup, 0, 0));


  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);

  GPIO_Config_T   GPIO_InitStructure;

  GPIO_InitStructure.mode  = GPIO_MODE_OUT_OD;
  GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
  GPIO_InitStructure.pin   = GPIO_PIN_12;
  GPIO_Config(GPIOA, &GPIO_InitStructure);

  GPIO_WriteBitValue(GPIOA, GPIO_PIN_12, _DEF_LOW);

  return true;
}

void delay(uint32_t time_ms)
{
  uint32_t pre_time = systick_ms;

  while(systick_ms-pre_time < time_ms);
}

uint32_t millis(void)
{
  return systick_ms;
}

void assert_failed(uint8_t* file, uint32_t line)
{
  char *name_buf;

  if (strrchr((char *) file,'/') == NULL) 
  {
    name_buf = strrchr((char *)file,'\\')+1;
  }
  else 
  {
    name_buf = strrchr((char *)file,'/')+1;
  }

  printf("assert_faile() - %s : %d", name_buf, (int)line);
}
