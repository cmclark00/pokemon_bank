/**
 * @file tusb_config.h
 * @brief TinyUSB configuration for Pokemon Bank
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUSB_MCU          OPT_MCU_RP2040
#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#define CFG_TUSB_OS           OPT_OS_NONE

// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
// #define CFG_TUSB_DEBUG        0

// Memory section
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN    __attribute__ ((aligned(4)))

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_ENDPOINT0_SIZE    64

//--------------------------------------------------------------------
// CLASS CONFIGURATION
//--------------------------------------------------------------------

// CDC FIFO size
#define CFG_TUD_CDC              1
#define CFG_TUD_CDC_RX_BUFSIZE   256
#define CFG_TUD_CDC_TX_BUFSIZE   256

// For WebUSB support (optional, for future WebUI)
#define CFG_TUD_VENDOR           0

#ifdef __cplusplus
}
#endif

#endif // TUSB_CONFIG_H
