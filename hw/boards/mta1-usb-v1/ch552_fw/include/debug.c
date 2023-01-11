/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 DEBUG Interface
                     CH554 main frequency modification, delay function definition
                     Serial port 0 and serial port 1 initialization
                     Serial port 0 and serial port 1 transceiver subfunctions
                     Watchdog initialization			 
*******************************************************************************/

#include <stdint.h>

#include "ch554.h"
#include "debug.h"

/*******************************************************************************
* Function Name  : CfgFsys( )
* Description  : CH554 clock selection and configuration function, Fsys 6MHz is used by default, FREQ_SYS can be passed
                 CLOCK_CFG configuration, the formula is as follows:
                 Fsys = (Fosc * 4 / (CLOCK_CFG & MASK_SYS_CK_SEL); the specific clock needs to be configured by yourself
*******************************************************************************/ 
void	CfgFsys( )  
{
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
//     CLOCK_CFG |= bOSC_EN_XT;                          // Enable external crystal
//     CLOCK_CFG & = ~ bOSC_EN_INT;                      // Turn off the internal crystal

#if FREQ_SYS == 32000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x07;  // 32MHz
#elif FREQ_SYS == 24000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x06;  // 24MHz	
#elif FREQ_SYS == 16000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x05;  // 16MHz	
#elif FREQ_SYS == 12000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x04;  // 12MHz
#elif FREQ_SYS == 6000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x03;  // 6MHz	
#elif FREQ_SYS == 3000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x02;  // 3MHz	
#elif FREQ_SYS == 750000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x01;  // 750KHz	
#elif FREQ_SYS == 187500
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x00;  // 187.5MHz		
#else
	#warning FREQ_SYS invalid or not set
#endif

	SAFE_MOD = 0x00;
}



/*******************************************************************************
* Function Name  : mDelayus(UNIT16 n)
* Description    : us delay function
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/ 
void	mDelayuS( uint16_t n )  // Delay in uS
{
#ifdef	FREQ_SYS
#if		FREQ_SYS <= 6000000
		n >>= 2;
#endif
#if		FREQ_SYS <= 3000000
		n >>= 2;
#endif
#if		FREQ_SYS <= 750000
		n >>= 4;
#endif
#endif
	while ( n ) {  // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
		++ SAFE_MOD;  // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef	FREQ_SYS
#if		FREQ_SYS >= 14000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 16000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 18000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 20000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 22000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 24000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 26000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 28000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 30000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 32000000
		++ SAFE_MOD;
#endif
#endif
		-- n;
	}
}

/*******************************************************************************
* Function Name  : mDelayms(UNIT16 n)
* Description    : ms delay function
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/
void	mDelaymS( uint16_t n )                                                  // Delay in mS
{
	while ( n ) {
#ifdef	DELAY_MS_HW
		while ( ( TKEY_CTRL & bTKC_IF ) == 0 );
		while ( TKEY_CTRL & bTKC_IF );
#else
		mDelayuS( 1000 );
#endif
		-- n;
	}
}                                         

#if SDCC < 370
void putchar(char c)
{
    while (!TI); /* assumes UART is initialized */
    TI = 0;
    SBUF = c;
}

char getchar() {
    while(!RI); /* assumes UART is initialized */
    RI = 0;
    return SBUF;
}
#else
int putchar(int c)
{
    while (!TI); /* assumes UART is initialized */
    TI = 0;
    SBUF = c & 0xFF;

    return c;
}

int getchar() {
    while(!RI); /* assumes UART is initialized */
    RI = 0;
    return SBUF;
}
#endif
