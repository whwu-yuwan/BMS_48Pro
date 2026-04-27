#include "crc8.h"

uint8_t CRC8_Update(uint8_t crc, const uint8_t *data, size_t len, uint8_t poly)
{
	if ((data == NULL) || (len == 0))
	{
		return crc;
	}

	for (size_t i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (uint8_t bit = 0; bit < 8; bit++)
		{
			if (crc & 0x80u)
			{
				crc = (uint8_t)((crc << 1) ^ poly);
			}
			else
			{
				crc = (uint8_t)(crc << 1);
			}
		}
	}

	return crc;
}

uint8_t CRC8_Calc(const uint8_t *data, size_t len, uint8_t init, uint8_t poly, uint8_t xorout)
{
	uint8_t crc = init;
	crc = CRC8_Update(crc, data, len, poly);
	crc ^= xorout;
	return crc;
}

uint8_t CRC8_SMBusPEC(const uint8_t *data, size_t len)
{
	return CRC8_Calc(data, len, 0x00u, 0x07u, 0x00u);
}
