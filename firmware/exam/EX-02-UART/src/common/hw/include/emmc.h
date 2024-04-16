#ifndef EMMC_H_
#define EMMC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_EMMC


typedef enum
{
  EMMC_BOOT_1,
  EMMC_BOOT_2,
  EMMC_BOOT_USER
} EmmcBootPartition_t;

typedef struct
{
  uint32_t card_type;                    /*!< Specifies the card Type                         */
  uint32_t card_version;                 /*!< Specifies the card version                      */
  uint32_t card_class;                   /*!< Specifies the class of the card class           */
  uint32_t rel_card_Add;                 /*!< Specifies the Relative Card Address             */
  uint32_t block_numbers;                /*!< Specifies the Card Capacity in blocks           */
  uint32_t block_size;                   /*!< Specifies one block size in bytes               */
  uint32_t log_block_numbers;            /*!< Specifies the Card logical Capacity in blocks   */
  uint32_t log_block_size;               /*!< Specifies logical block size in bytes           */
  uint32_t card_size;
} emmc_info_t;


bool emmcInit(void);
bool emmcReInit(void);
bool emmcDeInit(void);
bool emmcIsInit(void);
bool emmcIsDetected(void);
bool emmcGetInfo(emmc_info_t *p_info);
bool emmcIsBusy(void);
bool emmcIsReady(uint32_t timeout);


bool emmcReadBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms);
bool emmcWriteBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms);
bool emmcEraseBlocks(uint32_t start_addr, uint32_t end_addr);

bool emmcSetBootPartition(EmmcBootPartition_t parition, bool enable);


#endif

#ifdef __cplusplus
}
#endif

#endif 
