
#ifndef PMIC_REG_H_
#define PMIC_REG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"



#define STPMIC1_TURN_ON_REG                 0x1U
#define STPMIC1_TURN_OFF_REG                0x2U
#define STPMIC1_ICC_LDO_TURN_OFF_REG        0x3U
#define STPMIC1_ICC_BUCK_TURN_OFF_REG       0x4U
#define STPMIC1_RESET_STATUS_REG            0x5U
#define STPMIC1_VERSION_STATUS_REG          0x6U
#define STPMIC1_MAIN_CONTROL_REG            0x10U
#define STPMIC1_PADS_PULL_REG               0x11U
#define STPMIC1_BUCK_PULL_DOWN_REG          0x12U
#define STPMIC1_LDO14_PULL_DOWN_REG         0x13U
#define STPMIC1_LDO56_PULL_DOWN_REG         0x14U
#define STPMIC1_VIN_CONTROL_REG             0x15U
#define STPMIC1_PONKEY_TIMER_REG            0x16U
#define STPMIC1_MASK_RANK_BUCK_REG          0x17U
#define STPMIC1_MASK_RESET_BUCK_REG         0x18U
#define STPMIC1_MASK_RANK_LDO_REG           0x19U
#define STPMIC1_MASK_RESET_LDO_REG          0x1AU
#define STPMIC1_WATCHDOG_CONTROL_REG        0x1BU
#define STPMIC1_WATCHDOG_TIMER_REG          0x1CU
#define STPMIC1_BUCK_ICC_TURNOFF_REG        0x1DU
#define STPMIC1_LDO_ICC_TURNOFF_REG         0x1EU
#define STPMIC1_BUCK_APM_CONTROL_REG        0x1FU
#define STPMIC1_BUCK1_CONTROL_REG           0x20U
#define STPMIC1_BUCK2_CONTROL_REG           0x21U
#define STPMIC1_BUCK3_CONTROL_REG           0x22U
#define STPMIC1_BUCK4_CONTROL_REG           0x23U
#define STPMIC1_VREF_DDR_CONTROL_REG        0x24U
#define STPMIC1_LDO1_CONTROL_REG            0x25U
#define STPMIC1_LDO2_CONTROL_REG            0x26U
#define STPMIC1_LDO3_CONTROL_REG            0x27U
#define STPMIC1_LDO4_CONTROL_REG            0x28U
#define STPMIC1_LDO5_CONTROL_REG            0x29U
#define STPMIC1_LDO6_CONTROL_REG            0x2AU
#define STPMIC1_BUCK1_PWRCTRL_REG           0x30U
#define STPMIC1_BUCK2_PWRCTRL_REG           0x31U
#define STPMIC1_BUCK3_PWRCTRL_REG           0x32U
#define STPMIC1_BUCK4_PWRCTRL_REG           0x33U
#define STPMIC1_VREF_DDR_PWRCTRL_REG        0x34U
#define STPMIC1_LDO1_PWRCTRL_REG            0x35U
#define STPMIC1_LDO2_PWRCTRL_REG            0x36U
#define STPMIC1_LDO3_PWRCTRL_REG            0x37U
#define STPMIC1_LDO4_PWRCTRL_REG            0x38U
#define STPMIC1_LDO5_PWRCTRL_REG            0x39U
#define STPMIC1_LDO6_PWRCTRL_REG            0x3AU
#define STPMIC1_FREQUENCY_SPREADING_REG     0x3BU
#define STPMIC1_USB_CONTROL_REG             0x40U
#define STPMIC1_ITLATCH1_REG                0x50U
#define STPMIC1_ITLATCH2_REG                0x51U
#define STPMIC1_ITLATCH3_REG                0x52U
#define STPMIC1_ITLATCH4_REG                0x53U
#define STPMIC1_ITSETLATCH1_REG             0x60U
#define STPMIC1_ITSETLATCH2_REG             0x61U
#define STPMIC1_ITSETLATCH3_REG             0x62U
#define STPMIC1_ITSETLATCH4_REG             0x63U
#define STPMIC1_ITCLEARLATCH1_REG           0x70U
#define STPMIC1_ITCLEARLATCH2_REG           0x71U
#define STPMIC1_ITCLEARLATCH3_REG           0x72U
#define STPMIC1_ITCLEARLATCH4_REG           0x73U
#define STPMIC1_ITMASK1_REG                 0x80U
#define STPMIC1_ITMASK2_REG                 0x81U
#define STPMIC1_ITMASK3_REG                 0x82U
#define STPMIC1_ITMASK4_REG                 0x83U
#define STPMIC1_ITSETMASK1_REG              0x90U
#define STPMIC1_ITSETMASK2_REG              0x91U
#define STPMIC1_ITSETMASK3_REG              0x92U
#define STPMIC1_ITSETMASK4_REG              0x93U
#define STPMIC1_ITCLEARMASK1_REG            0xA0U
#define STPMIC1_ITCLEARMASK2_REG            0xA1U
#define STPMIC1_ITCLEARMASK3_REG            0xA2U
#define STPMIC1_ITCLEARMASK4_REG            0xA3U
#define STPMIC1_ITSOURCE1_REG               0xB0U
#define STPMIC1_ITSOURCE2_REG               0xB1U
#define STPMIC1_ITSOURCE3_REG               0xB2U
#define STPMIC1_ITSOURCE4_REG               0xB3U


#ifdef __cplusplus
}
#endif

#endif