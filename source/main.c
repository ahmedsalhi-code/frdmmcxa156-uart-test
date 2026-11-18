/*
 * FRDM-MCXA156 - UART test
 *
 * TEST_PHASE 1 : LPUART1 (mikroBUS J5) <-> LPUART2 (Arduino J1), results printed on PuTTY (COM9)
 * TEST_PHASE 2 : LPUART0 <-> PC echo (PuTTY COM9, 115200 8N1, no flow control)
 *
 * Wiring for phase 1 (2 jumper wires, crossed):
 *   J5[4]  P3_21  LPUART1_TX  --->  J1[2]  P2_11  LPUART2_RX  (Arduino D0)
 *   J1[4]  P2_10  LPUART2_TX  --->  J5[3]  P3_20  LPUART1_RX
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_lpuart.h"
#include "fsl_clock.h"
#include "fsl_reset.h"

#define TEST_PHASE   1          /* 1 = UART<->UART, 2 = UART<->PC */

#define PC_UART      LPUART0    /* MCU-Link VCOM */
#define UART_A       LPUART1    /* mikroBUS J5 */
#define UART_B       LPUART2    /* Arduino J1 */
#define UART_BAUD    115200U
#define UART_CLK_HZ  12000000U  /* FRO12M, divider 1 */
#define RX_TIMEOUT   200000U

volatile uint8_t  sent_char     = 0;
volatile uint8_t  received_char = 0;
volatile uint32_t receive_count = 0;
volatile uint32_t error_count   = 0;

/* ------------------------------------------------------------------ */
/* UART helpers                                                        */
/* ------------------------------------------------------------------ */

/* MCXA: LPUART_Init() only opens the bus clock gate. The functional
 * clock (mux + divider) must be selected and the block released from reset. */
static void Uart_ClockAndReset(LPUART_Type *base)
{
    if (base == LPUART0)
    {
        CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
        CLOCK_AttachClk(kFRO12M_to_LPUART0);
        RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    }
    else if (base == LPUART1)
    {
        CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
        CLOCK_AttachClk(kFRO12M_to_LPUART1);
        RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);
    }
    else if (base == LPUART2)
    {
        CLOCK_SetClockDiv(kCLOCK_DivLPUART2, 1u);
        CLOCK_AttachClk(kFRO12M_to_LPUART2);
        RESET_ReleasePeripheralReset(kLPUART2_RST_SHIFT_RSTn);
    }
}

static void Uart_Init(LPUART_Type *base)
{
    lpuart_config_t cfg;

    Uart_ClockAndReset(base);
    LPUART_GetDefaultConfig(&cfg);
    cfg.baudRate_Bps = UART_BAUD;
    cfg.enableTx     = true;
    cfg.enableRx     = true;
    LPUART_Init(base, &cfg, UART_CLK_HZ);
}

static void Uart_SendChar(LPUART_Type *base, uint8_t c)
{
    LPUART_WriteBlocking(base, &c, 1);
}

static void Uart_SendString(LPUART_Type *base, const char *s)
{
    LPUART_WriteBlocking(base, (const uint8_t *)s, strlen(s));
}

/* Clear RX FIFO and error flags (an overrun blocks reception on LPUART) */
static void Uart_Flush(LPUART_Type *base)
{
    while (LPUART_GetStatusFlags(base) & kLPUART_RxDataRegFullFlag)
        (void)LPUART_ReadByte(base);
    LPUART_ClearStatusFlags(base, kLPUART_RxOverrunFlag | kLPUART_FramingErrorFlag |
                                      kLPUART_NoiseErrorFlag | kLPUART_ParityErrorFlag);
}

static int Uart_TryRead(LPUART_Type *base, uint8_t *c)
{
    for (uint32_t t = RX_TIMEOUT; t; t--)
    {
        if (LPUART_GetStatusFlags(base) & kLPUART_RxDataRegFullFlag)
        {
            *c = LPUART_ReadByte(base);
            return 1;
        }
    }
    Uart_Flush(base);
    return 0;
}

#if TEST_PHASE == 1
/* Internal loopback (LOOPS=1, RSRC=0): TX tied to RX inside the chip, no wire.
 * Proves clock + peripheral work, independently of pins and wiring. */
static int Uart_SelfLoopback(LPUART_Type *base)
{
    uint8_t r = 0;
    int ok;

    LPUART_EnableTx(base, false);
    LPUART_EnableRx(base, false);
    base->CTRL = (base->CTRL & ~LPUART_CTRL_RSRC_MASK) | LPUART_CTRL_LOOPS_MASK;
    LPUART_EnableTx(base, true);
    LPUART_EnableRx(base, true);

    Uart_Flush(base);
    Uart_SendChar(base, 0x5A);
    ok = Uart_TryRead(base, &r) && (r == 0x5A);

    LPUART_EnableTx(base, false);
    LPUART_EnableRx(base, false);
    base->CTRL &= ~LPUART_CTRL_LOOPS_MASK;
    LPUART_EnableTx(base, true);
    LPUART_EnableRx(base, true);
    Uart_Flush(base);
    return ok;
}

static void Report(const char *name, int ok)
{
    Uart_SendString(PC_UART, ok ? "[ OK ] " : "[FAIL] ");
    Uart_SendString(PC_UART, name);
    Uart_SendString(PC_UART, "\r\n");
}

/* Send one char on 'tx', expect it on 'rx' */
static int Uart_WireTest(LPUART_Type *tx, LPUART_Type *rx, uint8_t c)
{
    uint8_t r = 0;
    Uart_Flush(rx);
    Uart_SendChar(tx, c);
    return Uart_TryRead(rx, &r) && (r == c);
}
#endif

/* ------------------------------------------------------------------ */

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    Uart_Init(PC_UART);

#if TEST_PHASE == 1
    char msg[80];

    Uart_SendString(PC_UART, "\r\n=== FRDM-MCXA156 phase 1 : LPUART1 <-> LPUART2 ===\r\n");
    Uart_Init(UART_A);
    Uart_Init(UART_B);

    Report("LPUART1 internal loopback", Uart_SelfLoopback(UART_A));
    Report("LPUART2 internal loopback", Uart_SelfLoopback(UART_B));
    Report("wire J5[4] P3_21 TX1 -> J1[2] P2_11 RX2", Uart_WireTest(UART_A, UART_B, 'x'));
    Report("wire J1[4] P2_10 TX2 -> J5[3] P3_20 RX1", Uart_WireTest(UART_B, UART_A, 'y'));
    Uart_SendString(PC_UART, "--- running A -> B -> A ---\r\n");

    uint8_t c = 'A';
    uint8_t echo;
    while (1)
    {
        /* LPUART1 -> LPUART2 */
        sent_char = c;
        Uart_Flush(UART_B);
        Uart_SendChar(UART_A, c);
        if (Uart_TryRead(UART_B, (uint8_t *)&received_char) && received_char == sent_char)
        {
            /* LPUART2 -> LPUART1 (echo back) */
            Uart_Flush(UART_A);
            Uart_SendChar(UART_B, received_char);
            if (Uart_TryRead(UART_A, &echo) && echo == sent_char)
                receive_count++;
            else
                error_count++;
        }
        else
        {
            received_char = '.';
            error_count++;
        }

        snprintf(msg, sizeof msg, "sent=%c recv=%c ok=%lu err=%lu\r\n", sent_char,
                 (received_char >= 32 && received_char < 127) ? received_char : '.',
                 (unsigned long)receive_count, (unsigned long)error_count);
        Uart_SendString(PC_UART, msg);

        c = (c == 'Z') ? 'A' : (uint8_t)(c + 1);   /* A..Z loop */
        for (volatile uint32_t i = 0; i < 2000000; i++)
        {
        }
    }

#else
    /* ---------- PHASE 2 : LPUART0 <-> PC (PuTTY) ---------- */
    uint8_t c;
    Uart_SendString(PC_UART, "\r\nHello from FRDM-MCXA156\r\nType something:\r\n");

    while (1)
    {
        if (LPUART_ReadBlocking(PC_UART, &c, 1) != kStatus_Success)
        {
            Uart_Flush(PC_UART);
            continue;
        }
        if (c == '\r')
            Uart_SendString(PC_UART, "\r\n");
        else
            Uart_SendChar(PC_UART, c);
    }
#endif
}
