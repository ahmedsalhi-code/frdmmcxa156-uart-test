/***********************************************************************************************************************
 * Pin routing for FRDM-MCXA156 UART test.
 * Mirrors frdmmcxa156_uart_test.mex (BOARD_InitPins). If you regenerate from the Pins tool, the
 * .mex holds the same configuration, so the result is equivalent.
 *
 *   LPUART0  (MCU-Link VCOM -> COM9)   RX = P0_2  (Alt2)        TX = P0_3  (Alt2)
 *   LPUART1  (mikroBUS J5)             RX = P3_20 (Alt3) J5[3]  TX = P3_21 (Alt3) J5[4]
 *   LPUART2  (Arduino J1, D0/D1)       RX = P2_11 (Alt3) J1[2]  TX = P2_10 (Alt3) J1[4]
 **********************************************************************************************************************/

#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "pin_mux.h"

void BOARD_InitBootPins(void)
{
    BOARD_InitPins();
}

static void UartPin(PORT_Type *port, uint32_t pin, port_mux_t mux)
{
    const port_pin_config_t cfg = {
        kPORT_PullUp,               /* pull-up: RX line idles high even if unconnected */
        kPORT_LowPullResistor,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_OpenDrainDisable,
        kPORT_LowDriveStrength,
        kPORT_NormalDriveStrength,
        mux,
        kPORT_InputBufferEnable,    /* mandatory on MCXA, otherwise RX never sees the line */
        kPORT_InputNormal,
        kPORT_UnlockRegister,
    };
    PORT_SetPinConfig(port, pin, &cfg);
}

void BOARD_InitPins(void)
{
    /* PORT modules: clock on + out of reset */
    CLOCK_EnableClock(kCLOCK_GatePORT0);
    CLOCK_EnableClock(kCLOCK_GatePORT2);
    CLOCK_EnableClock(kCLOCK_GatePORT3);
    RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);

    /* LPUART peripherals out of reset */
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kLPUART2_RST_SHIFT_RSTn);

    /* LPUART0 - debug VCOM */
    UartPin(PORT0, 2U, kPORT_MuxAlt2);    /* P0_2  LPUART0_RXD */
    UartPin(PORT0, 3U, kPORT_MuxAlt2);    /* P0_3  LPUART0_TXD */

    /* LPUART1 - mikroBUS */
    UartPin(PORT3, 20U, kPORT_MuxAlt3);   /* P3_20 LPUART1_RXD  J5[3] */
    UartPin(PORT3, 21U, kPORT_MuxAlt3);   /* P3_21 LPUART1_TXD  J5[4] */

    /* LPUART2 - Arduino header */
    UartPin(PORT2, 11U, kPORT_MuxAlt3);   /* P2_11 LPUART2_RXD  J1[2] (D0) */
    UartPin(PORT2, 10U, kPORT_MuxAlt3);   /* P2_10 LPUART2_TXD  J1[4] (D1) */
}
