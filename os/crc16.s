#include "mem.h"


	DATA	crc16tab<>+0(SB)/2, $0x0000
	DATA	crc16tab<>+2(SB)/2, $0x1021
	DATA	crc16tab<>+4(SB)/2, $0x2042
	DATA	crc16tab<>+6(SB)/2, $0x3063
	DATA	crc16tab<>+8(SB)/2, $0x4084
	DATA	crc16tab<>+10(SB)/2, $0x50a5
	DATA	crc16tab<>+12(SB)/2, $0x60c6
	DATA	crc16tab<>+14(SB)/2, $0x70e7
	DATA	crc16tab<>+16(SB)/2, $0x8108
	DATA	crc16tab<>+18(SB)/2, $0x9129
	DATA	crc16tab<>+20(SB)/2, $0xa14a
	DATA	crc16tab<>+22(SB)/2, $0xb16b
	DATA	crc16tab<>+24(SB)/2, $0xc18c
	DATA	crc16tab<>+26(SB)/2, $0xd1ad
	DATA	crc16tab<>+28(SB)/2, $0xe1ce
	DATA	crc16tab<>+30(SB)/2, $0xf1ef
	DATA	crc16tab<>+32(SB)/2, $0x1231
	DATA	crc16tab<>+34(SB)/2, $0x0210
	DATA	crc16tab<>+36(SB)/2, $0x3273
	DATA	crc16tab<>+38(SB)/2, $0x2252
	DATA	crc16tab<>+40(SB)/2, $0x52b5
	DATA	crc16tab<>+42(SB)/2, $0x4294
	DATA	crc16tab<>+44(SB)/2, $0x72f7
	DATA	crc16tab<>+46(SB)/2, $0x62d6
	DATA	crc16tab<>+48(SB)/2, $0x9339
	DATA	crc16tab<>+50(SB)/2, $0x8318
	DATA	crc16tab<>+52(SB)/2, $0xb37b
	DATA	crc16tab<>+54(SB)/2, $0xa35a
	DATA	crc16tab<>+56(SB)/2, $0xd3bd
	DATA	crc16tab<>+58(SB)/2, $0xc39c
	DATA	crc16tab<>+60(SB)/2, $0xf3ff
	DATA	crc16tab<>+62(SB)/2, $0xe3de
	DATA	crc16tab<>+64(SB)/2, $0x2462
	DATA	crc16tab<>+66(SB)/2, $0x3443
	DATA	crc16tab<>+68(SB)/2, $0x0420
	DATA	crc16tab<>+70(SB)/2, $0x1401
	DATA	crc16tab<>+72(SB)/2, $0x64e6
	DATA	crc16tab<>+74(SB)/2, $0x74c7
	DATA	crc16tab<>+76(SB)/2, $0x44a4
	DATA	crc16tab<>+78(SB)/2, $0x5485
	DATA	crc16tab<>+80(SB)/2, $0xa56a
	DATA	crc16tab<>+82(SB)/2, $0xb54b
	DATA	crc16tab<>+84(SB)/2, $0x8528
	DATA	crc16tab<>+86(SB)/2, $0x9509
	DATA	crc16tab<>+88(SB)/2, $0xe5ee
	DATA	crc16tab<>+90(SB)/2, $0xf5cf
	DATA	crc16tab<>+92(SB)/2, $0xc5ac
	DATA	crc16tab<>+94(SB)/2, $0xd58d
	DATA	crc16tab<>+96(SB)/2, $0x3653
	DATA	crc16tab<>+98(SB)/2, $0x2672
	DATA	crc16tab<>+100(SB)/2, $0x1611
	DATA	crc16tab<>+102(SB)/2, $0x0630
	DATA	crc16tab<>+104(SB)/2, $0x76d7
	DATA	crc16tab<>+106(SB)/2, $0x66f6
	DATA	crc16tab<>+108(SB)/2, $0x5695
	DATA	crc16tab<>+110(SB)/2, $0x46b4
	DATA	crc16tab<>+112(SB)/2, $0xb75b
	DATA	crc16tab<>+114(SB)/2, $0xa77a
	DATA	crc16tab<>+116(SB)/2, $0x9719
	DATA	crc16tab<>+118(SB)/2, $0x8738
	DATA	crc16tab<>+120(SB)/2, $0xf7df
	DATA	crc16tab<>+122(SB)/2, $0xe7fe
	DATA	crc16tab<>+124(SB)/2, $0xd79d
	DATA	crc16tab<>+126(SB)/2, $0xc7bc
	DATA	crc16tab<>+128(SB)/2, $0x48c4
	DATA	crc16tab<>+130(SB)/2, $0x58e5
	DATA	crc16tab<>+132(SB)/2, $0x6886
	DATA	crc16tab<>+134(SB)/2, $0x78a7
	DATA	crc16tab<>+136(SB)/2, $0x0840
	DATA	crc16tab<>+138(SB)/2, $0x1861
	DATA	crc16tab<>+140(SB)/2, $0x2802
	DATA	crc16tab<>+142(SB)/2, $0x3823
	DATA	crc16tab<>+144(SB)/2, $0xc9cc
	DATA	crc16tab<>+146(SB)/2, $0xd9ed
	DATA	crc16tab<>+148(SB)/2, $0xe98e
	DATA	crc16tab<>+150(SB)/2, $0xf9af
	DATA	crc16tab<>+152(SB)/2, $0x8948
	DATA	crc16tab<>+154(SB)/2, $0x9969
	DATA	crc16tab<>+156(SB)/2, $0xa90a
	DATA	crc16tab<>+158(SB)/2, $0xb92b
	DATA	crc16tab<>+160(SB)/2, $0x5af5
	DATA	crc16tab<>+162(SB)/2, $0x4ad4
	DATA	crc16tab<>+164(SB)/2, $0x7ab7
	DATA	crc16tab<>+166(SB)/2, $0x6a96
	DATA	crc16tab<>+168(SB)/2, $0x1a71
	DATA	crc16tab<>+170(SB)/2, $0x0a50
	DATA	crc16tab<>+172(SB)/2, $0x3a33
	DATA	crc16tab<>+174(SB)/2, $0x2a12
	DATA	crc16tab<>+176(SB)/2, $0xdbfd
	DATA	crc16tab<>+178(SB)/2, $0xcbdc
	DATA	crc16tab<>+180(SB)/2, $0xfbbf
	DATA	crc16tab<>+182(SB)/2, $0xeb9e
	DATA	crc16tab<>+184(SB)/2, $0x9b79
	DATA	crc16tab<>+186(SB)/2, $0x8b58
	DATA	crc16tab<>+188(SB)/2, $0xbb3b
	DATA	crc16tab<>+190(SB)/2, $0xab1a
	DATA	crc16tab<>+192(SB)/2, $0x6ca6
	DATA	crc16tab<>+194(SB)/2, $0x7c87
	DATA	crc16tab<>+196(SB)/2, $0x4ce4
	DATA	crc16tab<>+198(SB)/2, $0x5cc5
	DATA	crc16tab<>+200(SB)/2, $0x2c22
	DATA	crc16tab<>+202(SB)/2, $0x3c03
	DATA	crc16tab<>+204(SB)/2, $0x0c60
	DATA	crc16tab<>+206(SB)/2, $0x1c41
	DATA	crc16tab<>+208(SB)/2, $0xedae
	DATA	crc16tab<>+210(SB)/2, $0xfd8f
	DATA	crc16tab<>+212(SB)/2, $0xcdec
	DATA	crc16tab<>+214(SB)/2, $0xddcd
	DATA	crc16tab<>+216(SB)/2, $0xad2a
	DATA	crc16tab<>+218(SB)/2, $0xbd0b
	DATA	crc16tab<>+220(SB)/2, $0x8d68
	DATA	crc16tab<>+222(SB)/2, $0x9d49
	DATA	crc16tab<>+224(SB)/2, $0x7e97
	DATA	crc16tab<>+226(SB)/2, $0x6eb6
	DATA	crc16tab<>+228(SB)/2, $0x5ed5
	DATA	crc16tab<>+230(SB)/2, $0x4ef4
	DATA	crc16tab<>+232(SB)/2, $0x3e13
	DATA	crc16tab<>+234(SB)/2, $0x2e32
	DATA	crc16tab<>+236(SB)/2, $0x1e51
	DATA	crc16tab<>+238(SB)/2, $0x0e70
	DATA	crc16tab<>+240(SB)/2, $0xff9f
	DATA	crc16tab<>+242(SB)/2, $0xefbe
	DATA	crc16tab<>+244(SB)/2, $0xdfdd
	DATA	crc16tab<>+246(SB)/2, $0xcffc
	DATA	crc16tab<>+248(SB)/2, $0xbf1b
	DATA	crc16tab<>+250(SB)/2, $0xaf3a
	DATA	crc16tab<>+252(SB)/2, $0x9f59
	DATA	crc16tab<>+254(SB)/2, $0x8f78
	DATA	crc16tab<>+256(SB)/2, $0x9188
	DATA	crc16tab<>+258(SB)/2, $0x81a9
	DATA	crc16tab<>+260(SB)/2, $0xb1ca
	DATA	crc16tab<>+262(SB)/2, $0xa1eb
	DATA	crc16tab<>+264(SB)/2, $0xd10c
	DATA	crc16tab<>+266(SB)/2, $0xc12d
	DATA	crc16tab<>+268(SB)/2, $0xf14e
	DATA	crc16tab<>+270(SB)/2, $0xe16f
	DATA	crc16tab<>+272(SB)/2, $0x1080
	DATA	crc16tab<>+274(SB)/2, $0x00a1
	DATA	crc16tab<>+276(SB)/2, $0x30c2
	DATA	crc16tab<>+278(SB)/2, $0x20e3
	DATA	crc16tab<>+280(SB)/2, $0x5004
	DATA	crc16tab<>+282(SB)/2, $0x4025
	DATA	crc16tab<>+284(SB)/2, $0x7046
	DATA	crc16tab<>+286(SB)/2, $0x6067
	DATA	crc16tab<>+288(SB)/2, $0x83b9
	DATA	crc16tab<>+290(SB)/2, $0x9398
	DATA	crc16tab<>+292(SB)/2, $0xa3fb
	DATA	crc16tab<>+294(SB)/2, $0xb3da
	DATA	crc16tab<>+296(SB)/2, $0xc33d
	DATA	crc16tab<>+298(SB)/2, $0xd31c
	DATA	crc16tab<>+300(SB)/2, $0xe37f
	DATA	crc16tab<>+302(SB)/2, $0xf35e
	DATA	crc16tab<>+304(SB)/2, $0x02b1
	DATA	crc16tab<>+306(SB)/2, $0x1290
	DATA	crc16tab<>+308(SB)/2, $0x22f3
	DATA	crc16tab<>+310(SB)/2, $0x32d2
	DATA	crc16tab<>+312(SB)/2, $0x4235
	DATA	crc16tab<>+314(SB)/2, $0x5214
	DATA	crc16tab<>+316(SB)/2, $0x6277
	DATA	crc16tab<>+318(SB)/2, $0x7256
	DATA	crc16tab<>+320(SB)/2, $0xb5ea
	DATA	crc16tab<>+322(SB)/2, $0xa5cb
	DATA	crc16tab<>+324(SB)/2, $0x95a8
	DATA	crc16tab<>+326(SB)/2, $0x8589
	DATA	crc16tab<>+328(SB)/2, $0xf56e
	DATA	crc16tab<>+330(SB)/2, $0xe54f
	DATA	crc16tab<>+332(SB)/2, $0xd52c
	DATA	crc16tab<>+334(SB)/2, $0xc50d
	DATA	crc16tab<>+336(SB)/2, $0x34e2
	DATA	crc16tab<>+338(SB)/2, $0x24c3
	DATA	crc16tab<>+340(SB)/2, $0x14a0
	DATA	crc16tab<>+342(SB)/2, $0x0481
	DATA	crc16tab<>+344(SB)/2, $0x7466
	DATA	crc16tab<>+346(SB)/2, $0x6447
	DATA	crc16tab<>+348(SB)/2, $0x5424
	DATA	crc16tab<>+350(SB)/2, $0x4405
	DATA	crc16tab<>+352(SB)/2, $0xa7db
	DATA	crc16tab<>+354(SB)/2, $0xb7fa
	DATA	crc16tab<>+356(SB)/2, $0x8799
	DATA	crc16tab<>+358(SB)/2, $0x97b8
	DATA	crc16tab<>+360(SB)/2, $0xe75f
	DATA	crc16tab<>+362(SB)/2, $0xf77e
	DATA	crc16tab<>+364(SB)/2, $0xc71d
	DATA	crc16tab<>+366(SB)/2, $0xd73c
	DATA	crc16tab<>+368(SB)/2, $0x26d3
	DATA	crc16tab<>+370(SB)/2, $0x36f2
	DATA	crc16tab<>+372(SB)/2, $0x0691
	DATA	crc16tab<>+374(SB)/2, $0x16b0
	DATA	crc16tab<>+376(SB)/2, $0x6657
	DATA	crc16tab<>+378(SB)/2, $0x7676
	DATA	crc16tab<>+380(SB)/2, $0x4615
	DATA	crc16tab<>+382(SB)/2, $0x5634
	DATA	crc16tab<>+384(SB)/2, $0xd94c
	DATA	crc16tab<>+386(SB)/2, $0xc96d
	DATA	crc16tab<>+388(SB)/2, $0xf90e
	DATA	crc16tab<>+390(SB)/2, $0xe92f
	DATA	crc16tab<>+392(SB)/2, $0x99c8
	DATA	crc16tab<>+394(SB)/2, $0x89e9
	DATA	crc16tab<>+396(SB)/2, $0xb98a
	DATA	crc16tab<>+398(SB)/2, $0xa9ab
	DATA	crc16tab<>+400(SB)/2, $0x5844
	DATA	crc16tab<>+402(SB)/2, $0x4865
	DATA	crc16tab<>+404(SB)/2, $0x7806
	DATA	crc16tab<>+406(SB)/2, $0x6827
	DATA	crc16tab<>+408(SB)/2, $0x18c0
	DATA	crc16tab<>+410(SB)/2, $0x08e1
	DATA	crc16tab<>+412(SB)/2, $0x3882
	DATA	crc16tab<>+414(SB)/2, $0x28a3
	DATA	crc16tab<>+416(SB)/2, $0xcb7d
	DATA	crc16tab<>+418(SB)/2, $0xdb5c
	DATA	crc16tab<>+420(SB)/2, $0xeb3f
	DATA	crc16tab<>+422(SB)/2, $0xfb1e
	DATA	crc16tab<>+424(SB)/2, $0x8bf9
	DATA	crc16tab<>+426(SB)/2, $0x9bd8
	DATA	crc16tab<>+428(SB)/2, $0xabbb
	DATA	crc16tab<>+430(SB)/2, $0xbb9a
	DATA	crc16tab<>+432(SB)/2, $0x4a75
	DATA	crc16tab<>+434(SB)/2, $0x5a54
	DATA	crc16tab<>+436(SB)/2, $0x6a37
	DATA	crc16tab<>+438(SB)/2, $0x7a16
	DATA	crc16tab<>+440(SB)/2, $0x0af1
	DATA	crc16tab<>+442(SB)/2, $0x1ad0
	DATA	crc16tab<>+444(SB)/2, $0x2ab3
	DATA	crc16tab<>+446(SB)/2, $0x3a92
	DATA	crc16tab<>+448(SB)/2, $0xfd2e
	DATA	crc16tab<>+450(SB)/2, $0xed0f
	DATA	crc16tab<>+452(SB)/2, $0xdd6c
	DATA	crc16tab<>+454(SB)/2, $0xcd4d
	DATA	crc16tab<>+456(SB)/2, $0xbdaa
	DATA	crc16tab<>+458(SB)/2, $0xad8b
	DATA	crc16tab<>+460(SB)/2, $0x9de8
	DATA	crc16tab<>+462(SB)/2, $0x8dc9
	DATA	crc16tab<>+464(SB)/2, $0x7c26
	DATA	crc16tab<>+466(SB)/2, $0x6c07
	DATA	crc16tab<>+468(SB)/2, $0x5c64
	DATA	crc16tab<>+470(SB)/2, $0x4c45
	DATA	crc16tab<>+472(SB)/2, $0x3ca2
	DATA	crc16tab<>+474(SB)/2, $0x2c83
	DATA	crc16tab<>+476(SB)/2, $0x1ce0
	DATA	crc16tab<>+478(SB)/2, $0x0cc1
	DATA	crc16tab<>+480(SB)/2, $0xef1f
	DATA	crc16tab<>+482(SB)/2, $0xff3e
	DATA	crc16tab<>+484(SB)/2, $0xcf5d
	DATA	crc16tab<>+486(SB)/2, $0xdf7c
	DATA	crc16tab<>+488(SB)/2, $0xaf9b
	DATA	crc16tab<>+490(SB)/2, $0xbfba
	DATA	crc16tab<>+492(SB)/2, $0x8fd9
	DATA	crc16tab<>+494(SB)/2, $0x9ff8
	DATA	crc16tab<>+496(SB)/2, $0x6e17
	DATA	crc16tab<>+498(SB)/2, $0x7e36
	DATA	crc16tab<>+500(SB)/2, $0x4e55
	DATA	crc16tab<>+502(SB)/2, $0x5e74
	DATA	crc16tab<>+504(SB)/2, $0x2e93
	DATA	crc16tab<>+506(SB)/2, $0x3eb2
	DATA	crc16tab<>+508(SB)/2, $0x0ed1
	DATA	crc16tab<>+510(SB)/2, $0x1ef0
/*
 * ushort crc16byte(ushort oldcrc, byte b)
 */
TEXT	crc16byte(SB), $0
	MOVW	$crc16tab<>(SB), Z
	MOVB	b+2(FP), R6
	EORB	R21, R6	/* b ^= oldcrc>>8 */
	LSLB	R6	/* b *= sizeof(ushort) */
	ADDBC	R(REGZERO), ZH	/* high-order bit from shift */
	ADDB	R6, Z
	ADDBC	R(REGZERO), ZH	/* crc16tab<>[b] */
	MOVPM	(Z)+, R18	/* crc16tab<>[b] & 0xFF */
	MOVPM	(Z), R21	/* r=(crc16tab<>[b]>>8)<<8 */
	EORB	R20, R21	/* r ^= oldcrc << 8 */
	MOVB	R18, R20	/* r |= crc16tab<>[b] & 0xFF */
	RET
	PM	crc16tab<>+0(SB), $512
