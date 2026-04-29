#ifndef __CRC8_H
#define __CRC8_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t CRC8_Update(uint8_t crc, const uint8_t *data, size_t len, uint8_t poly);
uint8_t CRC8_Calc(const uint8_t *data, size_t len, uint8_t init, uint8_t poly, uint8_t xorout);

uint8_t CRC8_SMBusPEC(const uint8_t *data, size_t len);
uint8_t CRC8_BQ769x0_ReadPEC(uint8_t addr7, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
