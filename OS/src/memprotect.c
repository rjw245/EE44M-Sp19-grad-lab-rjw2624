#include <stdint.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "memprotect.h"

int MemProtect_CfgRegion(void *base,
                         uint8_t size_log2,
                         unsigned int access)
{
  if (size_log2 > 0x20 || size_log2 < 5)
    return -1;

  NVIC_MPU_BASE_R = ((uint32_t)base) & 0xFFFFFFE0;
  NVIC_MPU_ATTR_R = ((size_log2 - 1) << 1) | (access << 24) | (1 << 18) | (1 << 17);
  NVIC_MPU_BASE_R |= (1 << 4);
  return 0;
}

