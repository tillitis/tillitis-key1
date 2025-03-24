#include <stdint.h>

#include "ch554.h"

uint8_t write_code_flash(uint16_t address, uint16_t data)
{
    uint8_t ret = 0;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;        // Enter Safe mode
    GLOBAL_CFG |= bCODE_WE; // Enable flash-ROM write
    SAFE_MOD = 0;           // Exit Safe mode

    // Set address
    ROM_ADDR = address;
    ROM_DATA = data;

    // Check that write operation address is valid
    if ((ROM_STATUS & bROM_ADDR_OK) == 0) {
        ret |= 0x01;
    }

    if (ROM_CTRL & bROM_ADDR_OK) {
        E_DIS = 1; // Disable global interrupts to prevent Flash operation from being interrupted
        ROM_CTRL = ROM_CMD_WRITE; // Write
        E_DIS = 0; // Enable global interrupts
    }

    // Check operation command error
    if (ROM_STATUS & bROM_CMD_ERR) {
        ret |= 0x02;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;         // Enter Safe mode
    GLOBAL_CFG &= ~bCODE_WE; // Disable flash-ROM write
    SAFE_MOD = 0;            // Exit Safe mode

    return ret;
}

uint8_t write_data_flash(uint8_t address, uint8_t data)
{
    uint8_t ret = 0;

    EA = 0; // Disable global interrupts to prevent Flash operation from being interrupted

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;        // Enter Safe mode
    GLOBAL_CFG |= bDATA_WE; // Enable Data-Flash write
    SAFE_MOD = 0;           // Exit Safe mode

    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    ROM_ADDR_L = address << 1;
    ROM_DATA_L = data;

    // Check that write operation address is valid
    if ((ROM_STATUS & bROM_ADDR_OK) == 0) {
        ret |= 0x01;
    }

    if (ROM_CTRL & bROM_ADDR_OK) {
        ROM_CTRL = ROM_CMD_WRITE;  // Write
    }

    // Check operation command error
    if (ROM_STATUS & bROM_CMD_ERR) {
        ret |= 0x02;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;         // Enter Safe mode
    GLOBAL_CFG &= ~bDATA_WE; // Disable Data-Flash write
    SAFE_MOD = 0;            // Exit Safe mode

    EA = 1; // Enable global interrupts

    return ret;
}
