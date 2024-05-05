#ifndef PMIC_H_
#define PMIC_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_PMIC



bool pmicInit(void);
bool pmicEnableDDR(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
