#ifndef __SENOKO_GG_H__
#define __SENOKO_GG_H__

int ggInit(void);
int ggRefresh(int property);
int ggManufacturer(uint8_t *manuf);
int ggPartName(uint8_t name[8]);
int ggChemistry(uint8_t *chem);
int ggSerial(uint16_t *serial);
int ggPercent(uint8_t *capacity);
int ggCellVoltage(int cell, uint16_t *voltage);
int ggMode(uint16_t *word);
int ggSetPrimary(void);
int ggSetSecondary(void);
int ggTemperature(int16_t *word);
int ggVoltage(uint16_t *word);
int ggCurrent(int16_t *word);
int ggChargingVoltage(uint16_t *word);
int ggChargingCurrent(int16_t *word);
int ggFullCapacity(uint16_t *word);
int ggDesignCapacity(uint16_t *word);
int ggCurrent(int16_t *word);
int ggAverageCurrent(int16_t *word);
int ggStatus(uint16_t *word);
int ggFirmwareVersion(uint16_t *word);
int ggState(uint16_t *word);
int ggSetLeds(int state);
int ggSetChargeControl(int state);
int ggForceDischarge(int state);
int ggSetManufacturer(uint8_t name[11]);
int ggSetChemistry(uint8_t chem[4]);
int ggSetCellCount(int cells);
int ggCellCount(uint8_t *cells);
int ggSetCapacity(int cells, uint16_t capacity);
int ggSetITEnable(void);
int ggTimeToEmpty(uint16_t *minutes);
int ggTimeToFull(uint16_t *minutes);
int ggCalibrate(int16_t voltage, int16_t current,
                uint16_t temperature, int cells);

int ggPermanentFailureFlags(uint16_t *flags);
int ggFuseFlag(uint16_t *flags);
int ggPermanentFailureVoltage(uint16_t *voltage);
int ggPermanentFailureCellVoltage(int cell, uint16_t *voltage);
int ggPermanentFailureCurrent(int16_t *current);
int ggPermanentFailureTemperature(int16_t *temperature);
int ggPermanentFailureBatteryStatus(uint16_t *stat);
int ggPermanentFailureRemainingCapacity(uint16_t *capacity);
int ggPermanentFailureChargeStatus(uint16_t *stat);
int ggPermanentFailureSafetyStatus(uint16_t *stat);
int ggPermanentFailureFlags2(uint16_t *flags);
int ggPermanentFailureReset(void);
int ggFullReset(void);

#endif /* __SENOKO_GG_H__ */
