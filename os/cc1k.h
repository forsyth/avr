/*
 *
 * Authors:		Phil Buonadonna
 * Date last modified:  $Id: CC1000Const.h,v 1.15.2.5 2003/09/19 00:48:29 philipb Exp $
 *
 */

/**
 * @author Phil Buonadonna
 */
/* Constants defined for CC1K */
/* Register addresses */

#define CC1K_MAIN            0x00
#define CC1K_FREQ_2A         0x01
#define CC1K_FREQ_1A         0x02
#define CC1K_FREQ_0A         0x03
#define CC1K_FREQ_2B         0x04
#define CC1K_FREQ_1B         0x05
#define CC1K_FREQ_0B         0x06
#define CC1K_FSEP1           0x07
#define CC1K_FSEP0           0x08
#define CC1K_CURRENT         0x09
#define CC1K_FRONT_END       0x0A //10
#define CC1K_PA_POW          0x0B //11
#define CC1K_PLL             0x0C //12
#define CC1K_LOCK            0x0D //13
#define CC1K_CAL             0x0E //14
#define CC1K_MODEM2          0x0F //15
#define CC1K_MODEM1          0x10 //16
#define CC1K_MODEM0          0x11 //17
#define CC1K_MATCH           0x12 //18
#define CC1K_FSCTRL          0x13 //19
#define CC1K_FSHAPE7         0x14 //20
#define CC1K_FSHAPE6         0x15 //21
#define CC1K_FSHAPE5         0x16 //22
#define CC1K_FSHAPE4         0x17 //23
#define CC1K_FSHAPE3         0x18 //24
#define CC1K_FSHAPE2         0x19 //25
#define CC1K_FSHAPE1         0x1A //26
#define CC1K_FSDELAY         0x1B //27
#define CC1K_PRESCALER       0x1C //28
#define CC1K_TEST6           0x40 //64
#define CC1K_TEST5           0x41 //66
#define CC1K_TEST4           0x42 //67
#define CC1K_TEST3           0x43 //68
#define CC1K_TEST2           0x44 //69
#define CC1K_TEST1           0x45 //70
#define CC1K_TEST0           0x46 //71

// MAIN Register Bit Posititions
#define CC1K_RXTX		7
#define CC1K_F_REG		6
#define CC1K_RX_PD		5
#define CC1K_TX_PD		4
#define CC1K_FS_PD		3
#define CC1K_CORE_PD		2
#define CC1K_BIAS_PD		1
#define CC1K_RESET_N		0

// CURRENT Register Bit Positions
#define CC1K_VCO_CURRENT	4
#define CC1K_LO_DRIVE		2
#define CC1K_PA_DRIVE		0

// FRONT_END Register Bit Positions
#define CC1K_BUF_CURRENT	5
#define CC1K_LNA_CURRENT	3
#define CC1K_IF_RSSI		1
#define CC1K_XOSC_BYPASS	0

// PA_POW Register Bit Positions
#define CC1K_PA_HIGHPOWER	4
#define CC1K_PA_LOWPOWER	0

// PLL Register Bit Positions
#define CC1K_EXT_FILTER		7
#define CC1K_REFDIV		3
#define CC1K_ALARM_DISABLE	2
#define CC1K_ALARM_H		1
#define CC1K_ALARM_L		0

// LOCK Register Bit Positions
#define CC1K_LOCK_SELECT	4
#define CC1K_PLL_LOCK_ACCURACY	3
#define CC1K_PLL_LOCK_LENGTH	2
#define CC1K_LOCK_INSTANT	1
#define CC1K_LOCK_CONTINUOUS	0

// CAL Register Bit Positions
#define CC1K_CAL_START		7
#define CC1K_CAL_DUAL		6
#define CC1K_CAL_WAIT		5
#define CC1K_CAL_CURRENT	4
#define CC1K_CAL_COMPLETE	3
#define CC1K_CAL_ITERATE	0

// MODEM2 Register Bit Positions
#define CC1K_PEAKDETECT		7
#define CC1K_PEAK_LEVEL_OFFSET	0

// MODEM1 Register Bit Positions
#define CC1K_MLIMIT		5
#define CC1K_LOCK_AVG_IN	4
#define CC1K_LOCK_AVG_MODE	3
#define CC1K_SETTLING		1
#define CC1K_MODEM_RESET_N	0

// MODEM0 Register Bit Positions
#define CC1K_BAUDRATE		4
#define CC1K_DATA_FORMAT	2
#define CC1K_XOSC_FREQ		0

// MATCH Register Bit Positions
#define CC1K_RX_MATCH		4
#define CC1K_TX_MATCH		0

// FSCTLR Register Bit Positions
#define CC1K_DITHER1		3
#define CC1K_DITHER0		2
#define CC1K_SHAPE		1
#define CC1K_FS_RESET_N		0

// PRESCALER Register Bit Positions
#define CC1K_PRE_SWING		6
#define CC1K_PRE_CURRENT	4
#define CC1K_IF_INPUT		3
#define CC1K_IF_FRONT		2

/* 
 * CC1K Register Parameters Table
 *
 * This table follows the same format order as the CC1K register 
 * set EXCEPT for the last entry in the table which is the 
 * CURRENT register value for TX mode.
 */
#define CC1K_433_002_MHZ	0x00
#define CC1K_915_998_MHZ	0x01
#define CC1K_434_845_MHZ        0x02
#define CC1K_914_077_MHZ        0x03
#define CC1K_315_178_MHZ        0x04

#ifdef CC1K_DEFAULT_FREQ
#define CC1K_DEF_PRESET (CC1K_DEFAULT_FREQ)
#endif
#ifdef CC1K_MANUAL_FREQ
#define CC1K_DEF_FREQ (CC1K_MANUAL_FREQ)
#endif

#ifndef CC1K_DEF_PRESET
#define CC1K_DEF_PRESET	(CC1K_914_077_MHZ)
#endif 

#ifdef YYYY
static byte CC1K_Params[][31] = {
#else
static byte CC1K_Params[] = {
#endif
#ifdef YYYY
  // (0) 433.002 MHz channel, 19.2 Kbps data, Manchester Encoding, High Side LO
  { // MAIN   0x00
	0x31,
	// FREQ2A,FREQ1A,FREQ0A  0x01-0x03
	0x58,0x00,0x00,					
	// FREQ2B,FREQ1B,FREQ0B  0x04-0x06
	0x57,0xf6,0x85,    //XBOW
	// FSEP1, FSEP0     0x07-0x08
	0X03,0x55,
	// CURRENT (RX MODE VALUE)   0x09 (also see below)
	((4<<CC1K_VCO_CURRENT) | (1<<CC1K_LO_DRIVE)),	
	// FRONT_END  0x0a
	((1<<CC1K_IF_RSSI)),
	// PA_POW  0x0b
	((0x0<<CC1K_PA_HIGHPOWER) | (0xf<<CC1K_PA_LOWPOWER)), 
	// PLL  0x0c
	((12<<CC1K_REFDIV)),		
	// LOCK  0x0d
	((0xe<<CC1K_LOCK_SELECT)),
	// CAL  0x0e
	((1<<CC1K_CAL_WAIT) | (6<<CC1K_CAL_ITERATE)),	
	// MODEM2  0x0f
	((0<<CC1K_PEAKDETECT) | (28<<CC1K_PEAK_LEVEL_OFFSET)),
	// MODEM1  0x10
	((3<<CC1K_MLIMIT) | (1<<CC1K_LOCK_AVG_MODE) | (3<<CC1K_SETTLING) | (1<<CC1K_MODEM_RESET_N)), 
	// MODEM0  0x11
	((5<<CC1K_BAUDRATE) | (1<<CC1K_DATA_FORMAT) | (1<<CC1K_XOSC_FREQ)),
	// MATCH  0x12
	((0x7<<CC1K_RX_MATCH) | (0x0<<CC1K_TX_MATCH)),
	// FSCTRL 0x13
	((1<<CC1K_FS_RESET_N)),			
	// FSHAPE7 - FSHAPE1   0x14-0x1a
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
	// FSDELAY   0x1b
	0x00,	
	// PRESCALER    0x1c
	0x00,
	// CURRENT (TX MODE VALUE)  0x1d
	((8<<CC1K_VCO_CURRENT) | (1<<CC1K_PA_DRIVE)),
	// High side LO  0x1e (i.e. do we need to invert the data?)
	1
  },

  // (1) 914.9988 MHz channel, 19.2 Kbps data, Manchester Encoding, High Side LO
  { // MAIN   0x00 
	0x31,
	// FREQ2A,FREQ1A,FREQ0A  0x01-0x03
	0x7c,0x00,0x00,					
	// FREQ2B,FREQ1B,FREQ0B  0x04-0x06
	0x7b,0xf9,0xae,					
	// FSEP1, FSEP0     0x07-0x8
	0x02,0x38,
	// CURRENT (RX MODE VALUE)   0x09 (also see below)
	((8<<CC1K_VCO_CURRENT) | (3<<CC1K_LO_DRIVE)),
	//0x8C,	
	// FRONT_END  0x0a
	((1<<CC1K_BUF_CURRENT) | (2<<CC1K_LNA_CURRENT) | (1<<CC1K_IF_RSSI)),
	//0x32,
	// PA_POW  0x0b
	((0x8<<CC1K_PA_HIGHPOWER) | (0x0<<CC1K_PA_LOWPOWER)), 
	//0xff,
	// PLL  0xc
	((8<<CC1K_REFDIV)),		
	//0x40,
	// LOCK  0xd
	((0x1<<CC1K_LOCK_SELECT)),
	//0x10,
	// CAL  0xe
	((1<<CC1K_CAL_WAIT) | (6<<CC1K_CAL_ITERATE)),	
	//0x26,
	// MODEM2  0xf
	((1<<CC1K_PEAKDETECT) | (33<<CC1K_PEAK_LEVEL_OFFSET)),
	//0xA1,
	// MODEM1  0x10
	((3<<CC1K_MLIMIT) | (1<<CC1K_LOCK_AVG_MODE) | (3<<CC1K_SETTLING) | (1<<CC1K_MODEM_RESET_N)), 
	//0x6f, 
	// MODEM0  0x11
	((5<<CC1K_BAUDRATE) | (1<<CC1K_DATA_FORMAT) | (1<<CC1K_XOSC_FREQ)),
	//0x55,
	// MATCH 0x12
	((0x1<<CC1K_RX_MATCH) | (0x0<<CC1K_TX_MATCH)),
	//0x10,
	// FSCTRL  0x13
	((1<<CC1K_FS_RESET_N)),			
	//0x01,
	// FSHAPE7 - FSHAPE1   0x14..0x1a
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
	// FSDELAY   0x1b
	0x00,	
	// PRESCALER    0x1c
	0x00,
	// CURRENT (TX MODE VALUE)  0x1d
	((15<<CC1K_VCO_CURRENT) | (3<<CC1K_PA_DRIVE)),
	//0xf3,
	// High side LO  0x1e (i.e. do we need to invert the data?)
	1
  },

  // (2) 434.845200 MHz channel, 19.2 Kbps data, Manchester Encoding, High Side LO
  { // MAIN   0x00
	0x31,
	// FREQ2A,FREQ1A,FREQ0A  0x01-0x03
	0x51,0x00,0x00,					
	// FREQ2B,FREQ1B,FREQ0B  0x04-0x06
	0x50,0xf7,0x4F,    //XBOW
	// FSEP1, FSEP0     0x07-0x08
	0X03,0x0E,
	// CURRENT (RX MODE VALUE)   0x09 (also see below)
	((4<<CC1K_VCO_CURRENT) | (1<<CC1K_LO_DRIVE)),	
	// FRONT_END  0x0a
	((1<<CC1K_IF_RSSI)),
	// PA_POW  0x0b
	((0x0<<CC1K_PA_HIGHPOWER) | (0xf<<CC1K_PA_LOWPOWER)), 
	// PLL  0x0c
	((11<<CC1K_REFDIV)),		
	// LOCK  0x0d
	((0xe<<CC1K_LOCK_SELECT)),
	// CAL  0x0e
	((1<<CC1K_CAL_WAIT) | (6<<CC1K_CAL_ITERATE)),	
	// MODEM2  0x0f
	((1<<CC1K_PEAKDETECT) | (33<<CC1K_PEAK_LEVEL_OFFSET)),
	// MODEM1  0x10
	((3<<CC1K_MLIMIT) | (1<<CC1K_LOCK_AVG_MODE) | (3<<CC1K_SETTLING) | (1<<CC1K_MODEM_RESET_N)), 
	// MODEM0  0x11
	((5<<CC1K_BAUDRATE) | (1<<CC1K_DATA_FORMAT) | (1<<CC1K_XOSC_FREQ)),
	// MATCH  0x12
	((0x7<<CC1K_RX_MATCH) | (0x0<<CC1K_TX_MATCH)),
	// FSCTRL 0x13
	((1<<CC1K_FS_RESET_N)),			
	// FSHAPE7 - FSHAPE1   0x14-0x1a
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
	// FSDELAY   0x1b
	0x00,	
	// PRESCALER    0x1c
	0x00,
	// CURRENT (TX MODE VALUE)  0x1d
	((8<<CC1K_VCO_CURRENT) | (1<<CC1K_PA_DRIVE)),
	// High side LO  0x1e (i.e. do we need to invert the data?)
	1
  },
 
  // (3) 914.077 MHz channel, 19.2 Kbps data, Manchester Encoding, High Side LO
  { // MAIN   0x00
#endif
	0x31,
	// FREQ2A,FREQ1A,FREQ0A  0x01-0x03
	0x5c,0xe0,0x00,					
	// FREQ2B,FREQ1B,FREQ0B  0x04-0x06
	0x5c,0xdb,0x42,					
	// FSEP1, FSEP0     0x07-0x8
	0x01,0xAA,
	// CURRENT (RX MODE VALUE)   0x09 (also see below)
	((8<<CC1K_VCO_CURRENT) | (3<<CC1K_LO_DRIVE)),
	//0x8C,	
	// FRONT_END  0x0a
	((1<<CC1K_BUF_CURRENT) | (2<<CC1K_LNA_CURRENT) | (1<<CC1K_IF_RSSI)),
	//0x32,
	// PA_POW  0x0b
	((0x8<<CC1K_PA_HIGHPOWER) | (0x0<<CC1K_PA_LOWPOWER)), 
	//0xff,
	// PLL  0xc
	((6<<CC1K_REFDIV)),		
	//0x40,
	// LOCK  0xd
	Lbadman,
	//0x90,
	// CAL  0xe
	((1<<CC1K_CAL_WAIT) | (6<<CC1K_CAL_ITERATE)),	
	//0x26,
	// MODEM2  0xf
	0,
	// MODEM1  0x10
	((3<<CC1K_MLIMIT) | (1<<CC1K_LOCK_AVG_MODE) | (3<<CC1K_SETTLING) | Modemreset_n), 
	//0x6f, 
	// MODEM0  0x11
	((5<<CC1K_BAUDRATE) | (1<<CC1K_DATA_FORMAT) | (1<<CC1K_XOSC_FREQ)),
	//0x55,
	// MATCH 0x12
	((0x1<<CC1K_RX_MATCH) | (0x0<<CC1K_TX_MATCH)),
	//0x10,
	// FSCTRL  0x13
	((1<<CC1K_FS_RESET_N)),			
	//0x01,
	// FSHAPE7 - FSHAPE1   0x14..0x1a
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
	// FSDELAY   0x1b
	0x00,	
	// PRESCALER    0x1c
	0x00,
	// CURRENT (TX MODE VALUE)  0x1d
	((15<<CC1K_VCO_CURRENT) | (3<<CC1K_PA_DRIVE)),
	//0xf3,
	// High side LO  0x1e (i.e. do we need to invert the data?)
	1
#ifdef YYYY
  },

  // (4) 315.178985 MHz channel, 38.4 Kbps data, Manchester Encoding, High Side LO
  { // MAIN   0x00
	0x31,
	// FREQ2A,FREQ1A,FREQ0A  0x01-0x03
	0x45,0x60,0x00,					
	// FREQ2B,FREQ1B,FREQ0B  0x04-0x06
	0x45,0x55,0xBB,
	// FSEP1, FSEP0     0x07-0x08
	0X03,0x9C,
	// CURRENT (RX MODE VALUE)   0x09 (also see below)
	((8<<CC1K_VCO_CURRENT) | (0<<CC1K_LO_DRIVE)),	
	// FRONT_END  0x0a
	((1<<CC1K_IF_RSSI)),
	// PA_POW  0x0b
	((0x0<<CC1K_PA_HIGHPOWER) | (0xf<<CC1K_PA_LOWPOWER)), 
	// PLL  0x0c
	((13<<CC1K_REFDIV)),		
	// LOCK  0x0d
	((0xe<<CC1K_LOCK_SELECT)),
	// CAL  0x0e
	((1<<CC1K_CAL_WAIT) | (6<<CC1K_CAL_ITERATE)),	
	// MODEM2  0x0f
	((1<<CC1K_PEAKDETECT) | (33<<CC1K_PEAK_LEVEL_OFFSET)),
	// MODEM1  0x10
	((3<<CC1K_MLIMIT) | (1<<CC1K_LOCK_AVG_MODE) | (3<<CC1K_SETTLING) | (1<<CC1K_MODEM_RESET_N)), 
	// MODEM0  0x11
	((5<<CC1K_BAUDRATE) | (1<<CC1K_DATA_FORMAT) | (0<<CC1K_XOSC_FREQ)),
	// MATCH  0x12
	((0x7<<CC1K_RX_MATCH) | (0x0<<CC1K_TX_MATCH)),
	// FSCTRL 0x13
	((1<<CC1K_FS_RESET_N)),			
	// FSHAPE7 - FSHAPE1   0x14-0x1a
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
	// FSDELAY   0x1b
	0x00,	
	// PRESCALER    0x1c
	0x00,
	// CURRENT (TX MODE VALUE)  0x1d
	((8<<CC1K_VCO_CURRENT) | (1<<CC1K_PA_DRIVE)),
	// High side LO  0x1e (i.e. do we need to invert the data?)
	1
  },
#endif
};
