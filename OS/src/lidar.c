
#include <stdint.h>
#include "PLL.h"
#include "I2C.h"
#include "VL53L0X.h"
#include "UART.h"

int LiDAR_Init(void)
{
     // init and wake up VL53L0X
    if (VL53L0X_Init(VL53L0X_I2C_ADDR, 0))
        return 0;
    else return -1;
}

int LiDAR_GetMeasurement(void)
{
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_getSingleRangingMeasurement(&measurement, 0);
    if (measurement.RangeStatus != 4)
    {
        return measurement.RangeMilliMeter;
    }
    else
        return -1;
}
