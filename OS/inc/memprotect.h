/**
 * @file
 * @author Riley Wood (riley.wood@utexas.edu)
 * @brief Module implementing memory protection with the MPU
 * @version 0.1
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef _MEM_PROTECT_H_
#define _MEM_PROTECT_H_

#include <stdint.h>
#include "tm4c123gh6pm.h"

#define AP_PNA_UNA (0) //!< Privileged no access, unprivileged no access
#define AP_PRW_UNA (1) //!< Privileged r/w, unprivileged no access
#define AP_PRW_URO (2) //!< Privileged r/w, unprivileged read-only
#define AP_PRW_URW (3) //!< Privileged r/w, unprivileged r/w
#define AP_PRO_UNA (5) //!< Privileged read-only, unprivileged no access
#define AP_PRO_URO (6) //!< Privileged read-only, unprivileged read-only

/**
 * @brief Select a region to configure.
 * 
 * @param region Region to configure in subsequent function calls.
 */
static inline void MemProtect_SelectRegion(unsigned int region)
{
  NVIC_MPU_NUMBER_R = (region & 7);
}

/**
 * @brief Enable the memory protection unit.
 *        Call during OS initialization, after configuring all regions.
 */
static inline void MemProtect_EnableMPU(void)
{
  NVIC_MPU_CTRL_R |= 1;
}

/**
 * @brief Disable the memory protection unit.
 */
static inline void MemProtect_DisableMPU(void)
{
  NVIC_MPU_CTRL_R &= ~1;
}

/**
 * @brief Enable a specific protection region.
 *        Select a region beforehand with MemProtect_SelectRegion.
 */
static inline void MemProtect_EnableRegion(void)
{
  NVIC_MPU_ATTR_R |= 1;
}

/**
 * @brief Disable a specific protection region.
 *        Select a region beforehand with MemProtect_SelectRegion.
 */
static inline void MemProtect_DisableRegion(void)
{
  NVIC_MPU_ATTR_R &= ~1;
}

/**
 * @brief Configure one of the eight memory protection regions.
 *        Call this function before enabling the MPU.
 *        Select a region beforehand with MemProtect_SelectRegion.
 * 
 * @param base Base address of protected region. Must be aligned to size of region.
 * @param size_log2 Log base 2 of the size of the region. eg log2(256) = 8
 * @param access One of the access permissions constants, defined above.
 * @return int -1 on failure, 0 on success.
 */
int MemProtect_CfgRegion(void *base,
                         uint8_t size_log2,
                         unsigned int access);

/**
 * @brief Enable/disable subregions of a region.
 *        Each region is broken into 8 equal subregions.
 *        A 1 in the subregion_mask indicates protection for the subregion is disabled.
 *        A 0 in the subregion_mask indicates protection for the subregion is enabled.
 *        Select a region beforehand with MemProtect_SelectRegion.
 * 
 * @param subregion_mask Bitmask indicating which subregions will be enabled/disabled.
 * @return int -1 on failure, 0 on success.
 */
static inline void MemProtect_CfgSubregions(uint8_t subregion_mask)
{
  NVIC_MPU_ATTR_R = (NVIC_MPU_ATTR_R & 0xFFFF00FF) | ((subregion_mask & 0xFF) << 8);
}


#endif // _MEM_PROTECT_H_
