#include "ch.h"
#include "hal.h"
#include "i2c.h"

#include "senoko.h"

#if !defined(HAL_USE_I2C)
#error "I2C is not enabled"
#endif

#define i2cBus (&I2CD2)

static uint8_t i2c_registers[0x23];
static uint8_t i2c_buffer[sizeof(i2c_registers)];
static int current_register = 0;
static mutex_t client_mutex;

enum client_mode {
  I2C_MODE_SLAVE,
  I2C_MODE_MASTER,
};

static enum client_mode client_mode;

static const I2CConfig senokoI2cHost = {
  OPMODE_SMBUS_HOST,
  100000,
  STD_DUTY_CYCLE,
};

static const I2CConfig senokoI2cDevice = {
  OPMODE_SMBUS_DEVICE,
  100000,
  STD_DUTY_CYCLE,
};

static const I2CConfig senokoI2cMode = {
  OPMODE_I2C,
  100000,
  STD_DUTY_CYCLE,
};

static void i2cRxFinished(I2CDriver *i2cp, size_t bytes)
{
  if (bytes) {
    current_register = i2c_buffer[0];
    i2cSlaveSetTxOffset(i2cp, i2c_buffer[0]);
  }
}

static void i2cTxFinished(I2CDriver *i2cp, size_t bytes)
{
  (void)i2cp;
  (void)bytes;
}

static void senokoI2cReinit(void)
{
  client_mode = I2C_MODE_SLAVE;
  i2cStart(i2cBus, &senokoI2cDevice);
  i2cSlaveIoTimeout(i2cBus, SENOKO_I2C_SLAVE_ADDR,

                    /* Tx buffer */
                    i2c_registers, sizeof(i2c_registers),

                    /* Rx buffer */
                    i2c_buffer, sizeof(i2c_buffer),

                    /* Event-done callbacks */
                    i2cTxFinished, i2cRxFinished,

                    /* Timeout */
                    TIME_INFINITE);
}

void senokoI2cInit(void)
{
  uint32_t reg = 0;
  int counter = 0x40;

  i2c_registers[0] = 'S';
  i2c_registers[1] = SENOKO_OS_VERSION_MAJOR;
  i2c_registers[2] = SENOKO_OS_VERSION_MINOR;
  for (reg = 3; reg < sizeof(i2c_registers); reg++)
    i2c_registers[reg] = counter++;

  chMtxObjectInit(&client_mutex);
  senokoI2cReinit();
  return;
}

/**
 * @brief   Pauses I2C slave mode to send data via the I2C bus.
 * @details Function designed to realize "read-through-write" transfer
 *          paradigm. If you want transmit data without any further read,
 *          than set @b rxbytes field to 0.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address (7 bits) without R/W bit
 * @param[in] txbuf     pointer to transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to receive buffer
 * @param[in] rxbytes   number of bytes to be received, set it to 0 if
 *                      you want transmit only
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 * @api
 */
#define I2C_MAX_TRIES 10
msg_t senokoI2cMasterTransmitTimeout(i2caddr_t addr,
                                     const uint8_t *txbuf,
                                     size_t txbytes,
                                     uint8_t *rxbuf,
                                     size_t rxbytes,
                                     systime_t timeout) {
  msg_t ret;
  uint8_t rxbuf_hack[2];
  uint8_t *rxbuf_orig;
  int rxbuf_do_hack;
  int tries;

  /* I2C locks up if we transfer only one byte, so copy at least two bytes.*/
  if (rxbytes == 1) {
    rxbytes = 2;
    rxbuf_do_hack = 1;
    rxbuf_orig = rxbuf;
    rxbuf = rxbuf_hack;
  }
  else
    rxbuf_do_hack = 0;

  /* If we're still in slave mode, temporarily flip to master mode.*/
  if (client_mode == I2C_MODE_SLAVE) {
    i2cStop(i2cBus);
    i2cStart(i2cBus, &senokoI2cDevice);
  }

  /* Work around DMA bug, where it locks up if we transfer only one byte.*/
  if (rxbytes == 1)
    rxbytes = 2;

  /* Try multiple times, since this is a multi-master system.*/
  for (tries = 0; tries < I2C_MAX_TRIES; tries++) {

    /* Perform the transaction (now operating in master mode).*/
    ret = i2cMasterTransmitTimeout(i2cBus, addr,
                                   txbuf, txbytes,
                                   rxbuf, rxbytes,
                                   timeout);
    if (ret == MSG_OK)
      break;
  }

  /* Fixup one-byte copies (if necessary).*/
  if (rxbuf_do_hack)
    rxbuf_orig[0] = rxbuf_hack[0];

  /* Return back to slave mode, if we started in slave mode.*/
  if (client_mode == I2C_MODE_SLAVE) {
    i2cStop(i2cBus);
    senokoI2cReinit();
  }

//  chMtxUnlock(&client_mutex);

  return ret;
}

void senokoI2cAcquireBus(void) {

//  chMtxLock(&client_mutex);

  client_mode = I2C_MODE_MASTER;
  i2cStop(i2cBus);
  i2cStart(i2cBus, &senokoI2cDevice);
  i2cAcquireBus(i2cBus);

//  chMtxUnlock(&client_mutex);
}

void senokoI2cReleaseBus(void) {

//  chMtxLock(&client_mutex);

  i2cReleaseBus(i2cBus);
  i2cStop(i2cBus);
  senokoI2cReinit();

//  chMtxUnlock(&client_mutex);
}
