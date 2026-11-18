#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*! @brief Calls initialization functions. */
void BOARD_InitBootPins(void);

/*! @brief Routes LPUART0 (VCOM), LPUART1 (mikroBUS J5) and LPUART2 (Arduino J1) pins. */
void BOARD_InitPins(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PIN_MUX_H_ */
