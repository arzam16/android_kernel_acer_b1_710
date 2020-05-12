#include "ch703xtype.h"
#include "ch703xerr.h"
#include "ch703xdriver.h"

// MTK-Add-Start
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/div64.h>

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif

#ifdef MT6577
#include <mach/mt_devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif
// MTK-Add-End

#ifndef	NULL
#define NULL			((void *)0)
#endif

// MTK-Add-Start
extern int ch7035_i2c_read_byte(u8 addr, u8 *data);
extern int ch7035_i2c_write_byte(u8 addr, u8 data);
extern int ch7035_i2c_read_block( u8 addr, u8 *data, int len );
extern int ch7035_i2c_write_block( u8 addr, u8 *data, int len );
// MTK-Add-End

//====================== buffered i2c register ====================

#define REG_PAGE_NUM		5
#define REG_NUM_PER_PAGE	0x80
#define PAGE_REGISTER		0x03
#define HPD_DELAY_MAX    0x48

// Default register arrays
static uint8 g_DefRegMap[REG_PAGE_NUM][REG_NUM_PER_PAGE] = {
	//Page 0
	{	0x00, 0x01, 0x33, 0x00, 0x06, 0x58, 0xAC, 0xFF,  0xFF, 0x1F, 0x9E, 0x1A, 0x80, 0x20, 0x00, 0x10,
		0x60, 0x11, 0xE0, 0x0D, 0x00, 0x0A, 0x02, 0x00,  0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A,
		0x80, 0x20, 0x00, 0x10, 0x60, 0x11, 0xE0, 0x0D,  0x00, 0x0A, 0x02, 0x08, 0x00, 0x00, 0x3C, 0x00,
		0x01, 0x01, 0xC0, 0x01, 0x01, 0x80, 0x40, 0x40,  0x47, 0x88, 0x00, 0x00, 0x00, 0x86, 0x00, 0x11,

		0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x40, 0x40, 0x80, 0x00, 0x00,
		0x00, 0x1F, 0xFF, 0x00, 0x80, 0x10, 0x60, 0x00,  0x0A, 0x02, 0x08, 0x00, 0x00, 0x00, 0x40, 0x00,
		0x00, 0x00, 0x00, 0x01, 0x2D, 0x90, 0x20, 0x22,  0x44, 0x24, 0x40, 0x00, 0x10, 0x00, 0xA0, 0x4B,
		0x98, 0x01, 0x00, 0x00, 0x20, 0x80, 0x18, 0x00,  0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x0F, 0x00,
	},
	//Page 1:
	{	0x00, 0x01, 0x33, 0x01, 0x06, 0x58, 0xAC, 0x62,  0x04, 0x04, 0xC0, 0x65, 0x4A, 0x29, 0x80, 0x9C,
		0x00, 0x78, 0xD5, 0xA8, 0x93, 0x26, 0x00, 0x0E,  0xC8, 0x42, 0x6C, 0x00, 0x00, 0x00, 0xF4, 0xA0,
		0x00, 0x10, 0x07, 0xFF, 0xB4, 0x10, 0x00, 0x00,  0x15, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00,
		0x08, 0x60, 0x14, 0x20, 0x00, 0x00, 0x20, 0x00,  0x49, 0x10, 0xFF, 0xFF, 0xFF, 0x00, 0x08, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,  0x00, 0x00, 0x00, 0x7A, 0x5E, 0x6E, 0x1F, 0x1F,
		0x00, 0x00, 0x00, 0x2D, 0x40, 0x40, 0x40, 0x00,  0x00, 0x00, 0x00, 0x13, 0x02, 0xA9, 0x90, 0x50,
		0x00, 0x00, 0x09, 0x00, 0x00, 0x70, 0x00, 0x50,  0x00, 0x98, 0x00, 0x98, 0x03, 0x00, 0x00, 0x80,
	},
	//Page 2:
	{	0x00, 0x01, 0x33, 0x02, 0x06, 0x58, 0xAC, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	},
	//Page 3:
	{	0x00, 0x01, 0x33, 0x03, 0x06, 0x58, 0xAC, 0x00,  0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03, 0x19,
		0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xA5, 0x00, 0x00, 0x11, 0x00, 0x07,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
		0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0x09, 0x1D, 0x0F, 0x00, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
		0xFF, 0xF8, 0xFF, 0x00, 0x02, 0x00, 0x00, 0x00,  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	},
	//Page 4:
	{	0x00, 0x01, 0x33, 0x04, 0x06, 0x58, 0xAC, 0xFF,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x4F, 0x07, 0x4F, 0x07, 0x3B, 0x07,
		0x3B, 0x07, 0x50, 0x00, 0x50, 0x00, 0x10, 0x00,  0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x03, 0x00,  0x00, 0x00, 0x24, 0x00, 0x48, 0xFF, 0xFF, 0x7F,
		0x52, 0xE1, 0xFF, 0xFF, 0xD4, 0x60, 0xAD, 0x48,  0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x62, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	},
};

// MTK-Add-Start
uint32 os_shift_udiv(uint64 divisor, uint64 dividend, uint64 shift)
{
    uint64 result, n, mod;
    n = (uint64)(dividend) << (shift);
    mod = do_div(n, divisor);
    result = n;
    return (uint32)result;
}

uint32 os_shift_udiv_roundup(uint64 divisor, uint64 dividend, uint64 shift)
{
    uint64 result, n, mod;
    n = (((uint64)(dividend) << (shift)) + ((divisor) -1));
    mod = do_div(n, divisor);
    result = n;    
    return (uint32)result;
}
// MTK-Add-End

// Do not know how many bytes each block IO the I2C master supports,
// here simulate block IO with byte IO
// Wang Youcheng, 2011-11-28
int i2c_write_block(uint8 addr, uint8 reg, unsigned int len, uint8 *data)
{
    unsigned int    i;

    for(i = len; i != 0; i--) {
		ch7035_i2c_write_block(reg, data, 1);
        data++;
        //reg++; do not move register index, Wang Youcheng 2011-11-28
    }

    return (int)len;
}
int i2c_read_block(uint8 addr, uint8 reg, unsigned int num, uint8 *buff)
{
    unsigned int    i;

    for(i = num; i != 0; i--) {
	  	ch7035_i2c_read_block(reg, buff, 1);
        buff++;
        reg++; // move register index, Wang Youcheng 2011-11-28
    }

    return (int)num;
}

// MTK-Add-Start
//int i2c_write_block(uint8 addr, uint8 reg, unsigned int count, uint8 * data)
//{
//	ch7035_i2c_write_block(reg, data, count);
//
//	return (int)count;
//}

//int i2c_read_block(uint8 addr, uint8 reg, unsigned int num, uint8 * buff)
//{
//	ch7035_i2c_read_block(reg, buff, num);
//	
//	return (int)num;
//}

int 	i2c_write_byte(uint8 addr, uint8 reg, uint8 data)
{
	return ch7035_i2c_write_block(reg, &data, 1);
}

uint8	i2c_read_byte(uint8 addr, uint8 reg)
{
	uint8	data = 0;

	ch7035_i2c_read_block(reg, &data, 1);

	return data;
}
// MTK-Add-End

// buffered register
static uint8 g_nCurPage = 0;
static uint8 g_nCurRegmap[REG_PAGE_NUM][REG_NUM_PER_PAGE];

static uint8 iicbuf_read(uint8 index)
{
	return g_nCurRegmap[g_nCurPage][index];
}

static void iicbuf_write(uint8 index, uint8 value)
{
	if (index == PAGE_REGISTER) {
		g_nCurPage = value;
	} else {
		g_nCurRegmap[g_nCurPage][index] = value;
	}
}

static void iicbuf_reset(void)
{
	uint8 page, index;

	for(page = 0; page < REG_PAGE_NUM; ++page) {
		for(index = 0; index < REG_NUM_PER_PAGE; ++index) {
			g_nCurRegmap[page][index] = g_DefRegMap[page][index];
		}
	}
	g_nCurPage = 0; // page 0 for default
}

//======== buffered i2c register, extensive access ====================

typedef enum{
	// -----------Page 0-----------
	// 0x04:
	FPD = 0,
	// 0x07:
	I2S_PD,		PD_IO,		CEC_PD,		DRI_PD,		PDMIO,		PDPLL1,		SPDIF_PD,
	// 0x08:
	DRI_PDDRI,	PDDAC,		PANEN,
	// 0x09:
	DPD,		GCKOFF,		TV_BP,		SCLPD,		SDPD,		VGA_PD,		HDBKPD,		HDMI_PD,
	// 0x0A:
	MEMINIT,	MEMIDLE,	MEMPD,		STOP,		LVDS_PD,	HD_DVIB,	HDCP_PD,	MCU_PD,
	// 0x0B - 0x10:
	CPUEN,		HTI,		HAI,		HVAUTO,		DES,		HWI,		HOI,
	// 0x11 - 0x16:
	ITALIGN,	LOW5EN,		VTI,		VAI,		SWP_PAPB,	SWP_YC,		VWI,		VOI,
	// 0x17:
	INTLACE,	AH_LB,		HIGH,		REVERSE,	POS3X,		MULTI,
	// 0x18:
	IDF,		INTEN,		SWAP,
	// 0x19 - 0x1B:
	DBP1,		DBP0,		HPO_I,		VPO_I,		DEPO_I,		CRYS_EN,	GCLKFREQ,
	// 0x1C - 0x1D:
	CRYSFREQ,
	// 0x1E:
	I2SPOL,		I2S_SPDIFB,	INTLC,		I2S_LENGTH,	I2SFMT,
	// 0x1F - 0x24:
	YUVBPEN,	HTO,		HAO,		FMDRP,		HWO,		HOO,
	// 0x25 - 0x2A:
	IMGZOOM,	DNSMPEN,	VTO,		VAO,		PBPREN,		YC2RGB,		VWO,		VOO,
	// 0x2B:
	SYNCS,		VFMT,
	// 0x2C:
	VS_EN,		NST,		HDTVEN,		HDVDOFM,
	// 0x2D:
	WRLEN,		ROTATE,		YC2RGB_SEL,	CSSEL,
	// 0x2E:
	HFLIP,		VFLIP,		DEPO_O,		HPO_O,		VPO_O,		TE,
	// 0x2F - 0x31:
	CHG_HL,		MASKEN,		VST,		VEND,
	// 0x32 - 0x34:
	FLTBP2,		FLTBP1,		HST,		HEND,
	// 0x35:
	BRI,
	// 0x36:
	BOTM_EN,	CTA,
	// 0x37:
	HUE,
	// 0x38:
	GAP_EN,		SAT,
	// 0x39 - 0x3B:
	VP,			HP,
	// 0x3C:
	CC,
	// 0x3D:
	R3_R0,		M1M0,		C1C0,
	// 0x3E:
	PR,			SC1SC0,
	// 0x3F:
	Y1Y0,		A0,			B1B0,		S1S0,
	// 0x40:
	VIC,
	// 0x41 - 0x48:
	ET_L,		ET_U,		SB_L,		SB_U,		EL_L,		EL_U,		ER_L,		ER_U,
	// 0x49:
	CA,
	// 0x4A:
	DM_INH,		MF,			FR0,		ACP_TYPE,
	// 0x4B:
	BCH_L,		COPY,		FM_CH,		PCM_CH,		ELD_UP,		VSRSTEN,	PWM_INDEX,
	// 0x4C:
	LV_VGA_FRB,	BKLEN,		HD_LV_POL,	HD_LV_SEQ,
	// 0x4D:
	PWM_DUTY,
	// 0x4E:
	UPINT,		UINTLST,
	// 0x4F:
	DWNINT,		DINTLST,
	// 0x50:
	EDIDBNUM,
	// 0x51 - 0x52:
	DE_CPUEN,	CSYNCINS_525P,			PGM_NUM,
	// 0x54 - 0x56:
	COMP_BP,	DAC_EN_T,	HWO_HDMI,	HOO_HDMI,
	// 0x57 - 0x59:
	FLDSEN,		VWO_HDMI,	VOO_HDMI,
	// 0x5A - 0x5D:
	A1,
	// 0x5E:
	A2,
	// 0x5F:
	VSTADJ,		PSOFT,
	// 0x60 - 0x61:
	HADWSPP,	VFC1,
	// 0x62 - 0x63:
	STAT_LNE,	VFC2,
	// 0x64:
	FLDS,		BLK_H,
	// 0x65:
	FIELD_SW,	IHFSTEP,
	// 0x66:
	FIELDSW,	OHESTEP,
	// 0x67:
	FC1,		STFM,
	// 0x68:
	FC2,		LNSEL,		FLDCNTEN,
	// 0x69:
	FC3,		VFFSPP,
	// 0x6A:
	FC4,		THREN,		SWRDIM,		SFM,
	// 0x6B - 0x6C:
	FBA_INC,	SDSEL,
	// 0x6D - 0x6E:
	THRRL,		EDGE_ENH,	RGBEXP,		RGBEN,			WRFAST,
	// 0x6F - 0x70:
	VCNT,		VSMST,		SETEN,		RFLOPEN,		OCTLEN,
	// 0x71:
	HREPT,		VREPT,		STLN,
	// 0x72:
	TEBP,		UVBP,		YFBP,
	// 0x73:
	BL,
	// 0x74:
	DAT16_32B,	UBM,		HFLN_SPP,	TRUE_COM,		TRUE24,		DRMZOOM,	SPPSNS,		DM1EN,
	// 0x75:
	CHFSTP,		DACSP,
	// 0x76:
	CNESTP,		COND_SEL,
	// 0x77 - 0x78:
	CGMSDATA,	CGMSEN,
	// 0x79:
	SYNCTST,	PSEL,		BSEL,
	// 0x7A - 0x7C:
	TSTEP,		FRADJ,		SINAMP,		DCLVL,
	// 0x7D - 0x7E:
	R_INT,		HDMI_LVDS_SEL,			DE_GEN,		USE_DE,
	// 0x7F:
	V3EN,		TSYNC,		TEST,		TSTP,

	//-----------Page 1-----------
	// 0x07:
	BPCKSEL,	DRI_CMFB_EN,			CEC_PUEN,	CEC_T,		CKINV,		CK_TVINV,		DRI_CKS2,
	// 0x08:
	DACG,		DACKTST,	DEDGEB,		SYO,		DRI_IT_LVDS,	DISPON,
	// 0x09 - 0x0A:
	DIFFEN,		DPCKN4,		DPCKN7,		DPCKN8,		DPCKSEL,		DRI_CMTRIM,
	// 0x0A:
	DRI_DBEN,	DRI_DEMP,	DRI_EMP_EN,
	// 0x0B:
	DRI_DRAMP,	DRI_MODSEL,	DRI_OUTDOWN,			DRI_PSEN,		DRI_PSTRIM,
	// 0x0C:
	DRI_PLL_CP,	DRI_PLL_DIVSEL,	DRI_PLL_N1_1,	DRI_PLL_N1_0,	DRI_PLL_N3_1,	DRI_PLL_N3_0,	DRI_PLL_CKTSTEN,
	// 0x0D:
	DRI_PLL_PDO,DRI_PLL_VCOSEL1,		LV_HM_PSC,	DRI_PLL_RST,DRI_PLL_VCOSEL,	DRI_PLL_VIEN,	DRI_PLL_VOEN,	DRI_PB_SER,
	// 0x0E:
	DRI_RON,	DRI_TCFG,	GCKD,		GCKDEN,
	// 0x0F:
	GCKSEL,		DRI_CKS,	RDAC,		I2SCK_INV2,		I2SCK_INV1,	DRI_VCOMODE,
	// 0x10:
	I2S_DELAY,	I2S_RESL,	ICEN,
	// 0x11:
	INV_DE,		INV_H,		INV_V,		MCLKSEL,		DRI_VCO357SC,		OPBGTRM,		P1ATSTEN,
	// 0x12:
	VRTM,		PKINV,		PKXINV,		PLL2CP,			PLL2N5,
	// 0x13:
	PLL2R,		PLL3CP,		PLL3N6,		PLL3R,
	// 0x14:
	PLLTSTEN,	PWM_EN,		UKINV,		SDRAMCKSEC,		PLL2_SELH,
	// 0x15:
	PLL2_SELL,	DIVXTAL,	UCLKSEC,
	// 0x16:
	DRI_PLL_DIVSEL3_1,		SDRAM_CKSB,	D15_16B,	SDRAM_DQINENSEC,			SDRAM_DRIVE,	XCH,
	// 0x17:
	SDRAM_FBC,	SDRAM_EXTN,	SDRAM_INSCK,				SDRAM_RST,	SDRAM_SECH,	SDRAM_SELM,	SDRAM_TD7,
	// 0x18:
	SDRAM_TD6,	SDRAM_TD5,	SDRAM_TD4,
	// 0x19:
	SDRAM_TD3,	SDRAM_TD2,	SDRAM_TD1,
	// 0x1A:
	SEL_R,		DACSEN_TST,	SPDIF_MODE,		SPDIF_SEL,	TSTBGEN,
	// 0x1B - 0x1F:
	HAO_SCL,	VAO_SCL,	MIN_VAL,	SCAN_EN,		MAX_VAL,
	// 0x22:
	OUTEN_SPP,
	// 0x23:
	PAC8_1_ENABLE,
	// 0x24:
	PAC9_ENABLE,_VSP,		_HSP,		ZRCTS,
	// 0x28 - 0x29:
	PCLK_NUM,
	// 0x41:
	LVDS_TEST,	HSYNCP,		VSYNCP,		SMARTP,		CDETD,		SDETD,	PFORCE,
	// 0x42 - 0x48:
	PRGT1,		PRGT2,		PRGT3,		PRGT4,		PRGT5,
	// 0x4F:
	BIST_ENABLE_A,
	// 0x51 - 0x53:
	_PATTERN,
	// 0x55:
	CLKEN_DP, PWM_INV,
	// 0x57:
	MASKFM_EN,
	// 0x5B - 0x5D:
	TRAS,		TRC,		TDD,		TCAC,		TDPL,		TRP,	TRCD,		TMRD,
	// 0x5E - 0x61:
	SDTP1,		SDTP2,
	// 0x62:
	DRMTST,
	// 0x63:
	DEP,
	// 0x64:
	BC1,
	// 0x65:
	GC1,
	// 0x66:
	RC1,
	// 0x67:
	BC2,
	// 0x68:
	GC2,
	// 0x69:
	RC2,
	// 0x6A:
	TSTO,
	// 0x6B:
	VCO3CS,	ICPGBK2_0,	PDPLL2,		DRI_PD_SER,
	// 0x6C:
	PLL2N11,	PLL2N5_4,	DRI_PLL_PD,	PD_I2CM,
	// 0x6D:
	HDMI_CLP,	HDMI_RLP,	HDMI_CSP,
	// 0x6E:
	DACHEN,		VGA_DBT_EN,	HDMI_IBAS,	HDMIT,
	// 0x72:
	BIST_ERR_A,	BIST_DONE_A,
	// 0x73:
	BIST_DONE,	BIST_ERR,
	// 0x74 - 0x75:
	HTC,		TRASR,
	// 0x76 - 0x77:
	HAC,		_ERROR,		TRCR,
	// 0x78 - 0x79:
	VAC,		TDPLR,		TRCDR,
	// 0x7A - 0x7B:
	VTC,		TMRDR,		TRPR,
	// 0x7C:
	ATTACH2,	ATTACH1,	ATTACH0,	TCACR,
	// 0x7D - 0x7F:
	ERRAD,		DVALID,		DRMERR,

	//-----------Page 3-----------
	// 0x26:
	HDMI_TESTP_EN,		IRQHW_CLR,		IRQ_SEL,	VGACS_SEL,	SELR_SEL,	VGACH_SW,	DSTRG_ITV,
	// 0x27:
	IRQ_FMT,	DSGEN_PD,
	// 0x28:
	DIFF_EN,	CORREC_EN,	VGACLK_BP,	HM_LV_SEL,	HD_VGA_SEL,
	// 0x29 - 0x2A:
	THRWL,		LVDSCLK_BP,	HDTVCLK_BP,	HDMICLK_BP,	HDTV_BP,	HDMI_BP,
	// 0x2B - 0x2D:
	THRSH_DIFF,
	// 0x2E - 0x30:
	HPOS,		VPOS,
	// 0x31 - 0x32:
	GHV_DE,		GHV_DE_EN,
	// 0x71:
	CSFAIL,
	// 0x75:
	WFS_FLG,

	//-----------Page 4-----------
	// 0x10 - 0x12:
	UCLKFREQ,
	// 0x13 - 0x15:
	MCLKFREQ,
	// 0x2A - 0x2B:
	HINCA,
	// 0x2C - 0x2D:
	HINCB,
	// 0x2E - 0x2F:
	VINCA,
	// 0x30 - 0x31:
	VINCB,
	// 0x32 - 0x33:
	HDINCA,
	// 0x34 - 0x35:
	HDINCB,
	// 0x36 - 0x38:
	HINC,
	// 0x39 - 0x3B:
	VINC,
	// 0x3C - 0x3E:
	HDINC,
	// 0x50:
	DID,
	// 0x51:
	POWON,		IDBD,		VID,
	// 0x52:
	PGM_ARSTB,	MCU_ARSTB,	MCU_RETB,	RESETIB,	RESETDB,
	// 0x54:
	MCUCK_SEL1,	MCUCK_SEL0,	PDPLL0,		BGTRM,		PD_XTAL,	SPPSNS2,
	// 0x55:
	PLL1CP,		PLL1N1,		PLL1N2,
	// 0x56:
	PLL1N3,		PLL1R,		XOSC,
	// 0x57:
	I2CEC_EN,	WD_EN,		WDT_SEL,	WD_MB,		NOABT_1E,	NOABT_CHS,
	// 0x5B:
	LD_FLAG,
	// 0x60:
	A3,
	// 0x61:
	UCLKOD_SEL,	PRMSEL_S,	HPD_PD,		TM_SEL,		DIV4_PD,

	MUL_ID_END, 	// END FLAG, always be the last
}MULTI_REG_ID;

typedef struct _REG_BITS {
	uint8	reg;			// register
	uint8	startBit;		// start bit position in this register
	uint8	endBit;			//   end bit position in this register
//	uint8	stuff;
} REG_BITS;

typedef struct _MULTI_REG {
	MULTI_REG_ID	dataID;
	REG_BITS		top;
	REG_BITS		hig;
	REG_BITS		mid;
	REG_BITS		low;
	uint8			page;
}MULTI_REG;

static MULTI_REG g_MultiRegTable[MUL_ID_END] =
{
	// -----------Page 0-----------
	{FPD,				{  -1,0,0}, {  -1,0,0}, {  -1,0,0}, {0x04,5,5}, 0},
	{I2S_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,6,6}, 0},
	{PD_IO,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,5,5},	0},
	{CEC_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,4,4},	0},
	{DRI_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,3,3},	0},
	{PDMIO,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,2,2},	0},
	{PDPLL1,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,1,1},	0},
	{SPDIF_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x07,0,0},	0},
	{DRI_PDDRI,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x08,4,7},	0},
	{PDDAC,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x08,1,3},	0},
	{PANEN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x08,0,0},	0},
	{DPD,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,7,7},	0},
	{GCKOFF,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,6,6},	0},
	{TV_BP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,5,5},	0},
	{SCLPD,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,4,4},	0},
	{SDPD,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,3,3},	0},
	{VGA_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,2,2},	0},
	{HDBKPD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,1,1},	0},
	{HDMI_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x09,0,0},	0},
	{MEMINIT,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,7,7}, 0},
	{MEMIDLE,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,6,6}, 0},
	{MEMPD,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,5,5}, 0},
	{STOP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,4,4}, 0},
	{LVDS_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,3,3}, 0},
	{HD_DVIB,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,2,2}, 0},
	{HDCP_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,1,1}, 0},
	{MCU_PD,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0A,0,0}, 0},
	{CPUEN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0B,7,7}, 0},
	{HTI,				{  -1,0,0},	{  -1,0,0},	{0x0B,3,6},	{0x0D,0,7},	0},
	{HAI,				{  -1,0,0},	{  -1,0,0},	{0x0B,0,2},	{0x0C,0,7},	0},
	{HVAUTO,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0E,7,7},	0},
	{DES,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x0E,6,6}, 0},
	{HWI,				{  -1,0,0},	{  -1,0,0},	{0x0E,3,5},	{0x10,0,7},	0},
	{HOI,				{  -1,0,0},	{  -1,0,0},	{0x0E,0,2},	{0x0F,0,7}, 0},
	{ITALIGN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x11,7,7}, 0},
	{LOW5EN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x11,6,6}, 0},
	{VTI,				{  -1,0,0},	{  -1,0,0},	{0x11,3,5}, {0x13,0,7},	0},
	{VAI,				{  -1,0,0},	{  -1,0,0},	{0x11,0,2},	{0x12,0,7}, 0},
	{SWP_PAPB,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x14,7,7}, 0},
	{SWP_YC,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x14,6,6}, 0},
	{VWI,				{  -1,0,0},	{  -1,0,0},	{0x14,3,5},	{0x16,0,7},	0},
	{VOI,				{  -1,0,0},	{  -1,0,0},	{0x14,0,2},	{0x15,0,7}, 0},
	{INTLACE,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,7,7}, 0},
	{AH_LB,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,6,6}, 0},
	{HIGH,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,5,5}, 0},
	{REVERSE,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,4,4}, 0},
	{POS3X,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,2,3}, 0},
	{MULTI,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x17,0,1}, 0},
	{IDF,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x18,4,7}, 0},
	{INTEN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x18,3,3}, 0},
	{SWAP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x18,0,2}, 0},
	{DBP1,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,7,7}, 0},
	{DBP0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,6,6}, 0},
	{HPO_I,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,5,5}, 0},
	{VPO_I,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,4,4}, 0},
	{DEPO_I,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,3,3}, 0},
	{CRYS_EN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x19,2,2}, 0},
	{GCLKFREQ,			{  -1,0,0}, {0x19,0,1}, {0x1A,0,7}, {0x1B,0,7}, 0},
	{CRYSFREQ,			{  -1,0,0},	{  -1,0,0},	{0x1C,0,7},	{0x1D,0,7}, 0},
	{I2SPOL,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1E,7,7}, 0},
	{I2S_SPDIFB,		{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1E,6,6}, 0},
	{INTLC,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1E,5,5}, 0},
	{I2S_LENGTH,		{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1E,2,3}, 0},
	{I2SFMT,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1E,0,1}, 0},
	{YUVBPEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x1F,7,7}, 0},
	{HTO,				{  -1,0,0},	{  -1,0,0},	{0x1F,3,6},	{0x21,0,7}, 0},
	{HAO,				{  -1,0,0},	{  -1,0,0},	{0x1F,0,2},	{0x20,0,7}, 0},
	{FMDRP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x22,6,7}, 0},
	{HWO,				{  -1,0,0},	{  -1,0,0}, {0x22,3,5},	{0x24,0,7}, 0},
	{HOO,				{  -1,0,0}, {0x36,7,7}, {0x22,0,2}, {0x23,0,7}, 0}, // add HOO[11] - 2009.11.24
	{IMGZOOM,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x25,7,7}, 0},
	{DNSMPEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x25,6,6}, 0},
	{VTO,				{  -1,0,0},	{  -1,0,0}, {0x25,3,5},	{0x27,0,7}, 0},
	{VAO,				{  -1,0,0},	{  -1,0,0}, {0x25,0,2},	{0x26,0,7}, 0},
	{PBPREN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x28,7,7}, 0},
	{YC2RGB,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x28,6,6}, 0},
	{VWO,				{  -1,0,0},	{  -1,0,0}, {0x28,3,5}, {0x2A,0,7}, 0},
	{VOO,				{  -1,0,0}, {0x2C,7,7}, {0x28,0,2},	{0x29,0,7},	0}, // add VOO[11] - 2009.11.24
	{SYNCS,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2B,4,7}, 0},
	{VFMT,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2B,0,3}, 0},
	{VS_EN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2C,7,7}, 0},
	{NST,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2C,6,6}, 0},
	{HDTVEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2C,5,5}, 0},
	{HDVDOFM,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2C,0,4}, 0},
	{WRLEN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2D,6,7}, 0},
	{ROTATE,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2D,4,5}, 0},
	{YC2RGB_SEL,		{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2D,3,3}, 0},
	{CSSEL,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2D,0,2}, 0},
	{HFLIP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,7,7}, 0},
	{VFLIP,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,6,6}, 0},
	{DEPO_O,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,5,5}, 0},
	{HPO_O,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,4,4}, 0},
	{VPO_O,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,3,3}, 0},
	{TE,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2E,0,2}, 0},
	{CHG_HL,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2F,7,7}, 0},
	{MASKEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x2F,6,6}, 0},
	{VST,				{  -1,0,0},	{  -1,0,0}, {0x2F,3,5},	{0x31,0,7}, 0},
	{VEND,				{  -1,0,0},	{  -1,0,0}, {0x2F,0,2},	{0x30,0,7}, 0},
	{FLTBP2,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x32,7,7}, 0},
	{FLTBP1,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x32,6,6}, 0},
	{HST,				{  -1,0,0},	{  -1,0,0}, {0x32,3,5},	{0x34,0,7}, 0},
	{HEND,				{  -1,0,0},	{  -1,0,0}, {0x32,0,2},	{0x33,0,7}, 0},
	{BRI,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x35,0,7}, 0},
	{BOTM_EN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x36,7,7}, 0},
	{CTA,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x36,0,6}, 0},
	{HUE,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x37,0,6}, 0},
	{GAP_EN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x38,7,7}, 0},
	{SAT,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x38,0,6}, 0},
	{VP,				{  -1,0,0},	{  -1,0,0}, {0x39,4,7},	{0x3B,0,7}, 0},
	{HP,				{  -1,0,0},	{  -1,0,0}, {0x39,0,3},	{0x3A,0,7}, 0},
	{CC,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3C,0,6}, 0},
	{R3_R0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3D,4,7}, 0},
	{M1M0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3D,2,3}, 0},
	{C1C0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3D,0,1}, 0},
	{PR,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3E,4,7}, 0},
	{SC1SC0,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3E,2,3}, 0},
	{Y1Y0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3F,5,6}, 0},
	{A0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3F,4,4}, 0},
	{B1B0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3F,2,3}, 0},
	{S1S0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x3F,0,1}, 0},
	{VIC,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x40,0,7}, 0},
	{ET_L,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x41,0,7}, 0},
	{ET_U,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x42,0,7}, 0},
	{SB_L,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x43,0,7}, 0},
	{SB_U,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x44,0,7}, 0},
	{EL_L,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x45,0,7}, 0},
	{EL_U,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x46,0,7}, 0},
	{ER_L,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x47,0,7}, 0},
	{ER_U,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x48,0,7}, 0},
	{CA,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x49,0,7}, 0},
	{DM_INH,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4A,7,7}, 0},
	{MF,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4A,5,6}, 0},
	{FR0,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4A,4,4}, 0},
	{ACP_TYPE,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4A,0,1}, 0},
	{BCH_L,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,7,7}, 0},
	{COPY,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,6,6}, 0},
	{FM_CH,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,5,5}, 0},
	{PCM_CH,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,4,4}, 0},
	{ELD_UP,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,3,3}, 0},
	{VSRSTEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4B,2,2}, 0},
	{PWM_INDEX,			{  -1,0,0},	{  -1,0,0},	{0x7E,5,5},	{0x4B,0,1}, 0},
	{LV_VGA_FRB,		{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4C,7,7}, 0},
	{BKLEN,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4C,6,6}, 0},
	{HD_LV_POL,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4C,5,5}, 0},
	{HD_LV_SEQ,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4C,0,4}, 0},
	{PWM_DUTY,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4D,0,7}, 0},
	{UPINT,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4E,7,7}, 0},
	{UINTLST,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4E,0,6}, 0},
	{DWNINT,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4F,7,7}, 0},
	{DINTLST,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x4F,0,6}, 0},
	{EDIDBNUM,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x50,0,7}, 0},
	{DE_CPUEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x51,7,7}, 0},
	{CSYNCINS_525P,		{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x51,5,5}, 0},
	{PGM_NUM,			{  -1,0,0},	{  -1,0,0}, {0x51,0,4}, {0x52,0,7}, 0},
	{COMP_BP,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x54,7,7}, 0},
	{DAC_EN_T,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x54,6,6}, 0},
	{HWO_HDMI,			{  -1,0,0},	{  -1,0,0}, {0x54,3,5},	{0x56,0,7},	0},
	{HOO_HDMI,			{  -1,0,0},	{  -1,0,0}, {0x54,0,2},	{0x55,0,7},	0},
	{FLDSEN,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x57,7,7}, 0},
	{VWO_HDMI,			{  -1,0,0},	{  -1,0,0}, {0x57,3,5}, {0x59,0,7}, 0},
	{VOO_HDMI,			{  -1,0,0},	{  -1,0,0}, {0x57,0,2}, {0x58,0,7}, 0},
	{A1,				{0x5A,0,7}, {0x5B,0,7},	{0x5C,0,7}, {0x5D,0,7}, 0},
	{A2,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x5E,0,7}, 0},
	{VSTADJ,			{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x5F,6,7}, 0},
	{PSOFT,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x5F,0,5}, 0},
	{HADWSPP,			{  -1,0,0},	{  -1,0,0}, {0x60,0,7}, {0x61,0,2}, 0},
	{VFC1,				{  -1,0,0},	{  -1,0,0},	{  -1,0,0},	{0x61,3,7}, 0},
	{STAT_LNE,			{  -1,0,0},	{  -1,0,0}, {0x62,0,7}, {0x63,0,2}, 0},
	{VFC2,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x63,3,7}, 0},
	{FLDS,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x64,7,7}, 0},
	{BLK_H,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x64,0,6}, 0},
	{FIELD_SW,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x65,7,7}, 0},
	{IHFSTEP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x65,0,6}, 0},
	{FIELDSW,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x66,7,7}, 0},
	{OHESTEP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x66,0,6}, 0},
	{FC1,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x67,3,7}, 0},
	{STFM,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x67,0,2}, 0},
	{FC2,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x68,3,7}, 0},
	{LNSEL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x68,1,2}, 0},
	{FLDCNTEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x68,0,0}, 0},
	{FC3,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x69,3,7}, 0},
	{VFFSPP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x69,0,2}, 0},
	{FC4,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6A,3,7}, 0},
	{THREN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6A,2,2}, 0},
	{SWRDIM,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6A,1,1}, 0},
	{SFM,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6A,0,0}, 0},
	{FBA_INC,			{  -1,0,0},	{  -1,0,0}, {0x6B,0,7}, {0x6C,0,3}, 0},
	{SDSEL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6C,4,7}, 0},
	{THRRL,				{  -1,0,0},	{  -1,0,0}, {0x6D,0,7},	{0x6E,0,2}, 0},
	{EDGE_ENH,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x6E,7,7}, 0},
	{RGBEXP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x6E,6,6}, 0},
	{RGBEN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x6E,5,5}, 0},
	{WRFAST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x6E,3,3}, 0},
	{VCNT,				{  -1,0,0},	{  -1,0,0}, {0x6F,0,7}, {0x70,0,2}, 0},
	{VSMST,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x70,6,7}, 0},
	{SETEN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x70,5,5}, 0},
	{RFLOPEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x70,4,4}, 0},
	{OCTLEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x70,3,3}, 0},
	{HREPT,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x71,7,7}, 0},
	{VREPT,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x71,6,6}, 0},
	{STLN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x71,0,5}, 0},
	{TEBP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x72,7,7}, 0},
	{UVBP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x72,5,6}, 0},
	{YFBP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x72,0,4}, 0},
	{BL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x73,0,7}, 0},
	{DAT16_32B,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,7,7}, 0},
	{UBM,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,6,6}, 0},
	{HFLN_SPP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,5,5}, 0},
	{TRUE_COM,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,4,4}, 0},
	{TRUE24,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,3,3}, 0},
	{DRMZOOM,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,2,2}, 0},
	{SPPSNS,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,1,1}, 0},
	{DM1EN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x74,0,0}, 0},
	{CHFSTP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x75,3,7}, 0},
	{DACSP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x75,0,2}, 0},
	{CNESTP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x76,3,7}, 0},
	{COND_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x76,0,2}, 0},
	{CGMSDATA,			{  -1,0,0},	{  -1,0,0}, {0x77,0,7}, {0x78,0,5}, 0},
	{CGMSEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x78,6,7}, 0},
	{SYNCTST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x79,7,7}, 0},
	{PSEL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x79,3,6}, 0},
	{BSEL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x79,0,2}, 0},
	{TSTEP,				{  -1,0,0},	{  -1,0,0}, {0x7A,0,7}, {0x7B,0,1}, 0},
	{FRADJ,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7B,7,7}, 0},
	{SINAMP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7B,4,6}, 0},
	{DCLVL,				{  -1,0,0},	{  -1,0,0}, {0x7B,2,3}, {0x7C,0,7}, 0},
	{R_INT,				{  -1,0,0},	{  -1,0,0}, {0x7D,0,7}, {0x7E,0,3}, 0},
	{HDMI_LVDS_SEL, 	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7E,7,7}, 0},
	{DE_GEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7E,6,6}, 0},
	{USE_DE,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7E,4,4}, 0},
	{V3EN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7F,7,7}, 0},
	{TSYNC,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7F,5,5}, 0},
	{TEST,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7F,4,4}, 0},
	{TSTP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x7F,0,3}, 0},
	// -----------Page 1-----------
	{BPCKSEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,7,7}, 1},
	{DRI_CMFB_EN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,6,6}, 1},
	{CEC_PUEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,5,5}, 1},
	{CEC_T,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,3,4}, 1},
	{CKINV,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,2,2}, 1},
	{CK_TVINV,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,1,1}, 1},
	{DRI_CKS2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x07,0,0}, 1},
	{DACG,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,6,7}, 1},
	{DACKTST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,5,5}, 1},
	{DEDGEB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,4,4}, 1},
	{SYO,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,3,3}, 1},
	{DRI_IT_LVDS,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,1,2}, 1},
	{DISPON,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x08,0,0}, 1},
	{DIFFEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x09,7,7}, 1},
	{DPCKN4,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x09,6,6}, 1},
	{DPCKN7,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x09,5,5}, 1},
	{DPCKN8,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x09,3,4}, 1},
	{DPCKSEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x09,1,2}, 1},
	{DRI_CMTRIM,		{  -1,0,0},	{  -1,0,0}, {0x09,0,0}, {0x0A,6,7}, 1},
	{DRI_DBEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0A,5,5}, 1},
	{DRI_DEMP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0A,1,4}, 1},
	{DRI_EMP_EN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0A,0,0}, 1},
	{DRI_DRAMP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0B,5,7}, 1},
	{DRI_MODSEL,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0B,4,4}, 1},
	{DRI_OUTDOWN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0B,3,3}, 1},
	{DRI_PSEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0B,2,2}, 1},
	{DRI_PSTRIM,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0B,0,1}, 1},
	{DRI_PLL_CP,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,6,7}, 1},
	{DRI_PLL_DIVSEL,	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,5,5}, 1},
	{DRI_PLL_N1_1,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,4,4}, 1},
	{DRI_PLL_N1_0,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,3,3}, 1},
	{DRI_PLL_N3_1,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,2,2}, 1},
	{DRI_PLL_N3_0,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,1,1}, 1},
	{DRI_PLL_CKTSTEN,	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0C,0,0}, 1},
	{DRI_PLL_PDO,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,7,7}, 1},
	{DRI_PLL_VCOSEL1,	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,6,6}, 1},
	{LV_HM_PSC,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,5,5}, 1},
	{DRI_PLL_RST,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,4,4}, 1},
	{DRI_PLL_VCOSEL,	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,3,3}, 1},
	{DRI_PLL_VIEN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,2,2}, 1},
	{DRI_PLL_VOEN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,1,1}, 1},
	{DRI_PB_SER,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0D,0,0}, 1},
	{DRI_RON,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0E,7,7}, 1},
	{DRI_TCFG,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0E,6,6}, 1},
	{GCKD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0E,2,5}, 1},
	{GCKDEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0E,0,1}, 1},
	{GCKSEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,7,7}, 1},
	{DRI_CKS,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,6,6}, 1},
	{RDAC,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,3,5}, 1},
	{I2SCK_INV2,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,2,2}, 1},
	{I2SCK_INV1,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,1,1}, 1},
	{DRI_VCOMODE,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x0F,0,0}, 1},
	{I2S_DELAY,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x10,7,7}, 1},
	{I2S_RESL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x10,5,6}, 1},
	{ICEN,				{  -1,0,0}, {0x6E,0,1}, {0x10,0,4}, {0x11,7,7}, 1},
	{INV_DE,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,6,6}, 1},
	{INV_H,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,5,5}, 1},
	{INV_V,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,4,4}, 1},
	{MCLKSEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,3,3}, 1},
	{DRI_VCO357SC,		{  -1,0,0},	{  -1,0,0}, {0x11,2,2}, {0x6B,2,2}, 1},
	{OPBGTRM,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,1,1}, 1},
	{P1ATSTEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x11,0,0}, 1},
	{VRTM,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x12,6,7}, 1},
	{PKINV,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x12,5,5}, 1},
	{PKXINV,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x12,4,4}, 1},
	{PLL2CP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x12,2,3}, 1},
	{PLL2N5,			{  -1,0,0}, {0x6C,2,2}, {0x12,0,1}, {0x13,7,7}, 1},
	{PLL2R,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x13,5,6}, 1},
	{PLL3CP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x13,3,4}, 1},
	{PLL3N6,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x13,1,2}, 1},
	{PLL3R,				{  -1,0,0},	{  -1,0,0}, {0x13,0,0}, {0x14,7,7}, 1},
	{PLLTSTEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x14,5,6}, 1},
	{PWM_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x14,4,4}, 1},
	{UKINV,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x14,3,3}, 1},
	{SDRAMCKSEC,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x14,2,2}, 1},
	{PLL2_SELH,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x14,0,1}, 1},
	{PLL2_SELL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x15,6,7}, 1},
	{DIVXTAL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x15,1,5}, 1},
	{UCLKSEC,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x15,0,0}, 1},
	{DRI_PLL_DIVSEL3_1,	{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x16,5,7}, 1},
	{SDRAM_CKSB,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x16,4,4}, 1},
	{D15_16B,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x16,3,3}, 1},
	{SDRAM_DQINENSEC,	{  -1,0,0}, {  -1,0,0}, {  -1,0,0}, {0x16,2,2}, 1},
	{SDRAM_DRIVE,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x16,1,1}, 1},
	{XCH,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0},	{0x16,0,0}, 1},
	{SDRAM_FBC,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,7,7}, 1},
	{SDRAM_EXTN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,6,6}, 1},
	{SDRAM_INSCK,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,5,5}, 1},
	{SDRAM_RST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,4,4}, 1},
	{SDRAM_SECH,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,2,3}, 1},
	{SDRAM_SELM,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x17,1,1}, 1},
	{SDRAM_TD7,			{  -1,0,0},	{  -1,0,0}, {0x17,0,0}, {0x18,6,7}, 1},
	{SDRAM_TD6,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x18,4,5}, 1},
	{SDRAM_TD5,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x18,1,3}, 1},
	{SDRAM_TD4,			{  -1,0,0},	{  -1,0,0}, {0x18,0,0}, {0x19,7,7}, 1},
	{SDRAM_TD3,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x19,4,6}, 1},
	{SDRAM_TD2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x19,1,3}, 1},
	{SDRAM_TD1,			{  -1,0,0},	{  -1,0,0}, {0x19,0,0}, {0x1A,6,7}, 1},
	{SEL_R,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1A,5,5}, 1},
	{DACSEN_TST,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1A,4,4}, 1},
	{SPDIF_MODE,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1A,3,3}, 1},
	{SPDIF_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1A,2,2}, 1},
	{TSTBGEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1A,0,1}, 1},
	{HAO_SCL,			{  -1,0,0},	{  -1,0,0}, {0x1B,0,7}, {0x1C,0,2}, 1},
	{VAO_SCL,			{  -1,0,0},	{  -1,0,0}, {0x1D,0,7}, {0x1C,5,7}, 1},
	{MIN_VAL,			{  -1,0,0},	{  -1,0,0}, {0x1C,4,4}, {0x1F,3,7}, 1},
	{SCAN_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x1C,3,3}, 1},
	{MAX_VAL,			{  -1,0,0},	{  -1,0,0}, {0x1E,0,7}, {0x1F,0,2}, 1},
	{OUTEN_SPP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x22,2,2}, 1},
	{PAC8_1_ENABLE,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x23,0,7}, 1},
	{PAC9_ENABLE,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x24,7,7}, 1},
	{_VSP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x24,5,5}, 1},
	{_HSP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x24,4,4}, 1},
	{ZRCTS,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x24,1,1}, 1},
	{PCLK_NUM,			{  -1,0,0},	{  -1,0,0}, {0x28,0,7}, {0x29,0,7}, 1},
	{LVDS_TEST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,7,7}, 1},
	{HSYNCP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,6,6}, 1},
	{VSYNCP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,5,5}, 1},
	{SMARTP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,4,4}, 1},
	{CDETD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,2,2}, 1},
	{SDETD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,1,1}, 1},
	{PFORCE,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x41,0,0}, 1},
	{PRGT1,				{  -1,0,0},	{  -1,0,0}, {0x42,6,7}, {0x43,0,7}, 1},
	{PRGT2,				{  -1,0,0},	{  -1,0,0}, {0x42,4,5}, {0x44,0,7}, 1},
	{PRGT3,				{  -1,0,0},	{  -1,0,0}, {0x42,2,3}, {0x45,0,7}, 1},
	{PRGT4,				{  -1,0,0},	{  -1,0,0}, {0x42,0,1}, {0x46,0,7}, 1},
	{PRGT5,				{  -1,0,0},	{  -1,0,0}, {0x48,6,7}, {0x47,0,7}, 1},
	{BIST_ENABLE_A,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x4F,1,1}, 1},
	{_PATTERN,			{  -1,0,0},	{0x51,0,7}, {0x52,0,7}, {0x53,0,7}, 1},
	{CLKEN_DP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x55,2,2}, 1},
	{PWM_INV,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x55,0,0}, 1},
	{MASKFM_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,0,2}, 1},
	{TRAS,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5B,4,7}, 1},
	{TRC,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5B,0,3}, 1},
	{TDD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5C,4,6}, 1},
	{TCAC,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5C,2,3}, 1},
	{TDPL,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5C,0,1}, 1},
	{TRP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5D,5,7}, 1},
	{TRCD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5D,2,4}, 1},
	{TMRD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5D,0,1}, 1},
	{SDTP1,				{  -1,0,0},	{  -1,0,0}, {0x5E,0,7}, {0x5F,0,7}, 1},
	{SDTP2,				{  -1,0,0},	{  -1,0,0}, {0x60,0,7}, {0x61,0,7}, 1},
	{DRMTST,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x62,1,1}, 1},
	{DEP,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x63,1,1}, 1},
	{BC1,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x64,0,6}, 1},
	{GC1,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x65,0,6}, 1},
	{RC1,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x66,0,6}, 1},
	{BC2,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x67,0,7}, 1},
	{GC2,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x68,0,7}, 1},
	{RC2,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x69,0,7}, 1},
	{TSTO,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6A,0,7}, 1},
	{VCO3CS,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6B,6,7}, 1},
	{ICPGBK2_0,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6B,3,5}, 1},
	{PDPLL2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6B,1,1}, 1},
	{DRI_PD_SER,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6B,0,0}, 1},
	{PLL2N11,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6C,4,7}, 1},
	{PLL2N5_4,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6C,3,3}, 1},
	{DRI_PLL_PD,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6C,1,1}, 1},
	{PD_I2CM,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6C,0,0}, 1},
	{HDMI_CLP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6D,5,7}, 1},
	{HDMI_RLP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6D,2,4}, 1},
	{HDMI_CSP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6D,0,1}, 1},
	{DACHEN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6E,7,7}, 1},
	{VGA_DBT_EN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6E,6,6}, 1},
	{HDMI_IBAS,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6E,4,5}, 1},
	{HDMIT,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x6E,2,3}, 1},
	{BIST_ERR_A,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x72,7,7}, 1},
	{BIST_DONE_A,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x72,6,6}, 1},
	{BIST_DONE,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x73,4,7}, 1},
	{BIST_ERR,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x73,0,3}, 1},
	{HTC,				{  -1,0,0},	{  -1,0,0}, {0x74,0,7}, {0x75,0,3}, 1},
	{TRASR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x75,4,7}, 1},
	{HAC,				{  -1,0,0},	{  -1,0,0}, {0x76,0,7}, {0x77,0,2}, 1},
	{_ERROR,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x77,7,7}, 1},
	{TRCR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x77,3,6}, 1},
	{VAC,				{  -1,0,0},	{  -1,0,0}, {0x78,0,7}, {0x79,0,2}, 1},
	{TDPLR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x79,6,7}, 1},
	{TRCDR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x79,3,5}, 1},
	{VTC,				{  -1,0,0},	{  -1,0,0}, {0x7A,0,7}, {0x7B,0,2}, 1},
	{TMRDR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7B,6,7}, 1},
	{TRPR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7B,3,5}, 1},
	{ATTACH2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7C,6,7}, 1},
	{ATTACH1,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7C,4,5}, 1},
	{ATTACH0,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7C,2,3}, 1},
	{TCACR,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7C,0,1}, 1},
	{ERRAD,				{  -1,0,0}, {0x7D,0,7}, {0x7E,0,7}, {0x7F,0,3}, 1},
	{DVALID,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7F,7,7}, 1},
	{DRMERR,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x7F,4,6}, 1},
	// -----------Page 3-----------
	{HDMI_TESTP_EN,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,7,7}, 3},
	{IRQHW_CLR,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,6,6}, 3},
	{IRQ_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,5,5}, 3},
	{VGACS_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,4,4}, 3},
	{SELR_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,3,3}, 3},
	{VGACH_SW,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,2,2}, 3},
	{DSTRG_ITV,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x26,0,1}, 3},
	{IRQ_FMT,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x27,1,2}, 3},
	{DSGEN_PD,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x27,0,0}, 3},
	{DIFF_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x28,6,7}, 3},
	{CORREC_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x28,4,5}, 3},
	{VGACLK_BP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x28,3,3}, 3},
	{HM_LV_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x28,2,2}, 3},
	{HD_VGA_SEL,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x28,1,1}, 3},
	{THRWL,				{  -1,0,0},	{  -1,0,0}, {0x29,0,7}, {0x2A,0,2}, 3},
	{LVDSCLK_BP,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x2A,7,7}, 3},
	{HDTVCLK_BP,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x2A,6,6}, 3},
	{HDMICLK_BP,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x2A,5,5}, 3},
	{HDTV_BP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x2A,4,4}, 3},
	{HDMI_BP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x2A,3,3}, 3},
	{THRSH_DIFF,		{  -1,0,0},	{0x2B,0,7}, {0x2C,0,7}, {0x2D,0,7}, 3},
	{HPOS,				{  -1,0,0},	{  -1,0,0}, {0x2E,0,7}, {0x30,0,1}, 3},
	{VPOS,				{  -1,0,0},	{  -1,0,0}, {0x2F,0,7}, {0x30,2,3}, 3},
	{GHV_DE,			{  -1,0,0},	{  -1,0,0}, {0x31,0,7}, {0x32,0,6}, 3},
	{GHV_DE_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x32,7,7}, 3},
	{CSFAIL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x71,0,0}, 3},
	{WFS_FLG,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x75,0,1}, 3},
	// -----------Page 4-----------
	{UCLKFREQ,			{  -1,0,0},	{0x10,0,7}, {0x11,0,7},	{0x12,0,7}, 4},
	{MCLKFREQ,			{  -1,0,0},	{0x13,0,7}, {0x14,0,7},	{0x15,0,7}, 4},
	{HINCA,				{  -1,0,0},	{  -1,0,0}, {0x2A,0,7}, {0x2B,0,2}, 4},
	{HINCB,				{  -1,0,0},	{  -1,0,0}, {0x2C,0,7}, {0x2D,0,2}, 4},
	{VINCA,				{  -1,0,0},	{  -1,0,0}, {0x2E,0,7}, {0x2F,0,2}, 4},
	{VINCB,				{  -1,0,0},	{  -1,0,0}, {0x30,0,7}, {0x31,0,2}, 4},
	{HDINCA,			{  -1,0,0},	{  -1,0,0}, {0x32,0,7}, {0x33,0,2}, 4},
	{HDINCB,			{  -1,0,0},	{  -1,0,0}, {0x34,0,7}, {0x35,0,2}, 4},
	{HINC,				{  -1,0,0},	{0x36,0,4}, {0x37,0,7}, {0x38,0,7}, 4},
	{VINC,				{  -1,0,0},	{0x39,0,7}, {0x3A,0,7}, {0x3B,0,7}, 4},
	{HDINC,				{  -1,0,0},	{0x3C,0,7}, {0x3D,0,7}, {0x3E,0,7}, 4},
	{DID,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x50,0,7}, 4},
	{POWON,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x51,7,7}, 4},
	{IDBD,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x51,4,6}, 4},
	{VID,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x51,0,3}, 4},
	{PGM_ARSTB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x52,7,7}, 4},
	{MCU_ARSTB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x52,6,6}, 4},
	{MCU_RETB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x52,2,2}, 4},
	{RESETIB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x52,1,1}, 4},
	{RESETDB,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x52,0,0}, 4},
	{MCUCK_SEL1,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,7,7}, 4},
	{MCUCK_SEL0,		{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,6,6}, 4},
	{PDPLL0,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,4,4}, 4},
	{BGTRM,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,2,3}, 4},
	{PD_XTAL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,1,1}, 4},
	{SPPSNS2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x54,0,0}, 4},
	{PLL1CP,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x55,6,7}, 4},
	{PLL1N1,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x55,3,5}, 4},
	{PLL1N2,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x55,0,2}, 4},
	{PLL1N3,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x56,5,7}, 4},
	{PLL1R,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x56,3,4}, 4},
	{XOSC,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x56,0,2}, 4},
	{I2CEC_EN,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,6,7}, 4},
	{WD_EN,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,5,5}, 4},
	{WDT_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,3,4}, 4},
	{WD_MB,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,2,2}, 4},
	{NOABT_1E,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,1,1}, 4},
	{NOABT_CHS,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x57,0,0}, 4},
	{LD_FLAG,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x5B,0,7}, 4},
	{A3,				{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x60,0,7}, 4},
	{UCLKOD_SEL,		{  -1,0,0},	{  -1,0,0}, {0x61,7,7}, {0x61,2,2}, 4},
	{PRMSEL_S,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x61,6,6}, 4},
	{HPD_PD,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x61,5,5}, 4},
	{TM_SEL,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x61,3,4}, 4},
	{DIV4_PD,			{  -1,0,0},	{  -1,0,0}, {  -1,0,0}, {0x61,1,1}, 4},
};

static uint32 regbits_read(uint32 d, REG_BITS *rb)
{
	uint8 	m, n;

	n = (rb->endBit - rb->startBit + 1);
	m = ((1 << n) - 1);
	d = (d << n) | ((iicbuf_read(rb->reg) >> rb->startBit) & m);

	return d;
}

static uint32 iicbuf_read_ex(MULTI_REG_ID i)
{
	uint32 d = 0;

	iicbuf_write(PAGE_REGISTER, g_MultiRegTable[i].page);

	if (0xFF != g_MultiRegTable[i].top.reg)
		d = regbits_read(d, &g_MultiRegTable[i].top);
	if (0xFF != g_MultiRegTable[i].hig.reg)
		d = regbits_read(d, &g_MultiRegTable[i].hig);
	if (0xFF != g_MultiRegTable[i].mid.reg)
		d = regbits_read(d, &g_MultiRegTable[i].mid);
	d = regbits_read(d, &g_MultiRegTable[i].low);

	return d;
}

static uint32 regbits_write(uint32 d, REG_BITS *rb)
{
	uint8	m, n, t;

	n = (rb->endBit - rb->startBit + 1);
	m = ((1 << n) - 1) << rb->startBit;
	t = ((uint8)(d << rb->startBit) & m) | (iicbuf_read(rb->reg) & (~m));
	iicbuf_write(rb->reg, t);

	return (d >> n);
}

static void iicbuf_write_ex(MULTI_REG_ID i, uint32 val)
{
	uint32 d = val;

	iicbuf_write(PAGE_REGISTER, g_MultiRegTable[i].page);

	d = regbits_write(d, &g_MultiRegTable[i].low);
	if (0xFF != g_MultiRegTable[i].mid.reg) {
		d = regbits_write(d, &g_MultiRegTable[i].mid);
		if (0xFF != g_MultiRegTable[i].hig.reg) {
			d = regbits_write(d, &g_MultiRegTable[i].hig);
			if (0xFF != g_MultiRegTable[i].top.reg) {
				d = regbits_write(d, &g_MultiRegTable[i].top);
			}
		}
	}
}

//====================   local assist  ========================

static uint8 lnsel_saved;
static uint8 all_bypass_mode;		// indicate if all are bypass modes

static void reset_local_vars(void)
{
	lnsel_saved = 0;
	all_bypass_mode = 0;
}

#define LVDS_PWM_FREQ_NUM		8
static uint32 LVDS_PWM_FREQ[LVDS_PWM_FREQ_NUM] = {
	//unit: Hz
	100,
	200,
	2000,
	4000,
	16000,
	32000,
	64000,
	128000,
};

static uint8 pwm_freq_to_index(uint32 freq)
{
	uint8 i;

	// not compare index 0, use default if not found
	for (i = LVDS_PWM_FREQ_NUM - 1; i > 0; --i) {
		if (freq >= LVDS_PWM_FREQ[i])	// WYC_MODIFY
			break;
	}

	return i;
}

//================ device hardware operation ===============

#define FIRMWARE_LOAD_DELAY		300		// unit ms
#define FIRMWARE_RESET_DELAY	300		// unit ms
#define ENABLE_AUTOFUNC_DELAY	300		// unit ms

typedef struct _CH703x_REGCFG {
	uint8 page, reg, mask, cfg;
} CH703x_REGCFG;

#ifdef DEBUG_REG_IO		// trace register output
static int hw_WriteByte(uint8 addr, uint8 reg, uint8 val)
{
	printk("\t{ 0x%02X, 0x%02X },\n", reg, val); // MTK-Modify
	return	i2c_write_byte(addr, reg, val);
}
//#define TRACE_MSG(msg)		DBG_OUT(msg)  // MTK-Modify

#else

#define hw_WriteByte(addr, reg, val)	i2c_write_byte(addr, reg, val)
#define TRACE_MSG(msg)

#endif

// not trace register input
#define hw_ReadByte(addr, reg)			i2c_read_byte(addr, reg)

#define  hw_WritePage(addr, page)		hw_WriteByte(addr, PAGE_REGISTER, page)
#define  i2c_write_page(addr, page)		i2c_write_byte(addr, PAGE_REGISTER, page)

// ........ Trace register IO functions .............

static uint8 hw_setBits(uint8 addr, uint8 page, uint8 reg, uint8 mask, uint8 bits)
{
	uint8 t;

	i2c_write_page(addr, page);
	t = i2c_read_byte(addr, reg);
	i2c_write_byte(addr, reg, (t & mask) | bits);

	return t;
}

#define MCUAUTO_REGPAGE		0
#define MCUAUTO_REGISTER	0x4E
#define MCUAUTO_ALLBITS		MCUAUTO_SETMODE

// return old autobits
#define hw_enableMcuAutoFunc(addr, bits)	\
	MCUAUTO_ALLBITS & (~hw_setBits(addr, MCUAUTO_REGPAGE, MCUAUTO_REGISTER, \
		~MCUAUTO_ALLBITS, (~bits)&MCUAUTO_ALLBITS))

#define hw_setPWMDuty(addr, pwm)	\
	hw_setBits(addr, g_MultiRegTable[PWM_DUTY].page, \
	g_MultiRegTable[PWM_DUTY].low.reg, 0xFF, pwm)

#if 0
static void hw_Reset(uint8 addr)
{
	uint8 t, r;

	TRACE_MSG("// hw_reset\n");

	i2c_write_page(addr, MCUAUTO_REGPAGE);	// save MCU auto function setting
	t = i2c_read_byte(addr, MCUAUTO_REGISTER);

	hw_WritePage(addr, 0x04);		// reset IB and hold MCU
	r = hw_ReadByte(addr, 0x52);
	hw_WriteByte(addr, 0x52, r & 0xF9);
	hw_WriteByte(addr, 0x52, r & 0xFB);

	hw_WritePage(addr, 0x00);		// MCU clock to 27MHz
	hw_WriteByte(addr, 0x1C, 0x69);
	hw_WriteByte(addr, 0x1D, 0x78);

#if (MCUAUTO_REGPAGE != 0)			// restore MCU auto function setting
	hw_WritePage(addr, MCUAUTO_REGPAGE);
#endif
	hw_WriteByte(addr, MCUAUTO_REGISTER, t);

	hw_WritePage(addr, 0x04);		// release MCU
	hw_WriteByte(addr, 0x52, r);

	msleep(FIRMWARE_RESET_DELAY);	// wait MCU run // MTK-Modify
}
#endif

// by WYC 2012-7-6
static void hw_ResetIB(uint8 addr)
{
	uint8 	r;

	i2c_write_page(addr, MCUAUTO_REGPAGE);
	r = i2c_read_byte(addr, MCUAUTO_REGISTER);

	i2c_write_page(addr, 4);
	i2c_write_byte(addr, 0x52, 0xC3);	// reset IB and hold MCU
	i2c_write_byte(addr, 0x52, 0xC1);
	i2c_write_byte(addr, 0x52, 0xC3);

	i2c_write_page(addr, 0);
	i2c_write_byte(addr, 0x1C, 0x69);	// MCU clock to 27MHz
	i2c_write_byte(addr, 0x1D, 0x78);

	// Disable MCU auto set output mode - wyc 2012-5-25
#if (MCUAUTO_REGPAGE != 0)
	i2c_write_page(addr, MCUAUTO_REGPAGE);
#endif
	i2c_write_byte(addr, MCUAUTO_REGISTER, r|MCUAUTO_SETMODE);

	i2c_write_page(addr, 1);
	i2c_write_byte(addr, 0x1E, 9);
}

static void hw_ReleaseMCU(uint8 addr)
{
	i2c_write_page(addr, 0x04);
	i2c_write_byte(addr, 0x52, 0xC7);

	msleep(FIRMWARE_RESET_DELAY);	// wait MCU run
}

// write buffered register setting to hardware register that its value is
// different from its default.
static void hw_SetupReg(uint8 addr)
{
	uint8 page, index;

	TRACE_MSG("// hw_SetupReg\n");
	for(page = 0; page < REG_PAGE_NUM; ++page) {
		if (page == 0x02) continue; // page 2 is always same ...
		hw_WritePage(addr, page);
		for(index = 0x07; index < REG_NUM_PER_PAGE; ++index) {
			if (g_nCurRegmap[page][index] != g_DefRegMap[page][index]) {
				hw_WriteByte(addr, index, g_nCurRegmap[page][index]);
				//printk("iicbuf page=0x%x,0x%02x=0x%02x\n",page,index,g_nCurRegmap[page][index]); // MTK-Modify
			}
		}
	}
}

// ........ Not Trace register IO functions .............

//          DID   RID IDBD I2C  Output channels      EDID read channel
// CH7033E  52     2  40   76  hdmi/vga1 vga/hdtv   DDC-hdmi/SPCM-vga(level shifter)
// CH7034E  52     2  60   75  lvds      vga/hdtv   DDC-vga
// CH7035E  52     2  00   76  hdmi                 DDC-hdmi
// CH7033F  56/5E  2  40   76  hdmi/vga1 vga/hdtv   DDC-hdmi/SPCM-vga(level shifter)
// CH7034F  56/5E  2  60   75  lvds      vga/hdtv   DDC-vga
// CH7035F  56/5E  2  00   76  hdmi                 DDC-hdmi

#ifdef  LINUX_OS
#define CH703x_I2CADDR1		0x76
#define CH703x_I2CADDR2		0x75
#endif

//#ifdef  WINDOWS_OS		// i2c address in Linux << 1
#define CH703x_I2CADDR1		0xEC
#define CH703x_I2CADDR2		0xEA
#define CH7035_I2CADDR           0xEC
//#endif
#define CH703x_I2CADDR_NUM	2

// device ID, revision ID, IDBD and available channel bits
#define CH703xE_DID			0x52
#define CH703xE_RID			2
#define CH703xF_DID			0x56	// or 0x5E
#define CH7033_IDBD			0x40
#define CH7034_IDBD			0x60
#define CH7035_IDBD			0x00

#define CH703x_DIDMASK		0xF7
#define CH703x_RID_MASK		0x0F
#define CH703x_IDBD_MASK	0x60

#define CH7033_CHANBITS		(CHANNEL_HDMI | CHANNEL_VGA | CHANNEL_HDTV | CHANNEL_VGA1)
#define CH7034_CHANBITS		(CHANNEL_LVDS | CHANNEL_VGA | CHANNEL_HDTV)
#define CH7035_CHANBITS		(CHANNEL_HDMI)

#define CH703x_ID_PAGE		4
#define CH703x_DID_REG		0x50
#define CH703x_RID_REG		0x51

typedef struct _ch703x_dev_info{
	uint8				i2cAddr;
	uint8				devID;
	uint8				revID;
	uint8				chanBits;	// available channel bits
	uint8 *				channels;	// valid output channel lists
}CH703x_DEV_INFO;

static uint8 ch7033_outchans[] =
{
	CHANNEL_HDMI, CHANNEL_VGA, CHANNEL_HDTV, CHANNEL_VGA1,
	CHANNEL_HDMI | CHANNEL_VGA, CHANNEL_HDMI | CHANNEL_HDTV,
	0x00	// end flag
};

static uint8 ch7034_outchans[] =
{
	CHANNEL_LVDS, CHANNEL_VGA, CHANNEL_HDTV,
	CHANNEL_LVDS | CHANNEL_VGA, CHANNEL_LVDS | CHANNEL_HDTV,
	0x00, // end flag
};

static uint8 ch7035_outchans[] =
{
	CHANNEL_HDMI,
	0x00	// end flag
};

// index is CH703x_DEV_TYPE
static CH703x_DEV_INFO	ch703x_devinfo[DEV_UNKNOW] =
{
	{ CH703x_I2CADDR1, CH703xE_DID, CH703xE_RID | CH7033_IDBD, CH7033_CHANBITS, ch7033_outchans },
	{ CH703x_I2CADDR2, CH703xE_DID, CH703xE_RID | CH7034_IDBD, CH7034_CHANBITS, ch7034_outchans },
	{ CH703x_I2CADDR1, CH703xE_DID, CH703xE_RID | CH7035_IDBD, CH7035_CHANBITS, ch7035_outchans },

	{ CH703x_I2CADDR1, CH703xF_DID, CH703xE_RID | CH7033_IDBD, CH7033_CHANBITS, ch7033_outchans },
	{ CH703x_I2CADDR2, CH703xF_DID, CH703xE_RID | CH7034_IDBD, CH7034_CHANBITS, ch7034_outchans },
	{ CH703x_I2CADDR1, CH703xF_DID, CH703xE_RID | CH7035_IDBD, CH7035_CHANBITS, ch7035_outchans },
};

static uint8 ch703x_i2caddr[CH703x_I2CADDR_NUM] = { CH703x_I2CADDR1, CH703x_I2CADDR2 };

static CH703x_DEV_TYPE  hw_detectChip(uint8 addr, CH703x_DEV_TYPE start, CH703x_DEV_TYPE end)
{
	uint8  dev, rev;

	i2c_write_page(addr, CH703x_ID_PAGE);
	dev = i2c_read_byte(addr, CH703x_DID_REG);
	rev = i2c_read_byte(addr, CH703x_RID_REG);

	if (dev == 0 && rev == 0)	// no such i2c device
		return DEV_UNKNOW;

	printk("Dev %X Rev %d IDBD %X at %X\n", dev, rev & CH703x_RID_MASK,
		rev & CH703x_IDBD_MASK, addr); // MTK-Modify

	dev &= CH703x_DIDMASK;
	rev &= CH703x_RID_MASK | CH703x_IDBD_MASK;
	while(start <= end) {
		if ((ch703x_devinfo[start].i2cAddr == addr) &&
			(ch703x_devinfo[start].devID   == dev)  &&
			(ch703x_devinfo[start].revID   == rev))
			return start;
		start++;
	}

	return DEV_UNKNOW;
}

static int32 hw_checkOutpuChannel(CH703x_DEV_TYPE chip, uint8 chan)
{
	uint8 *pChans;

	if (chan == 0)
		return ERR_NO_OUT_CHANNEL_SELECTED;

	for(pChans = ch703x_devinfo[chip].channels; *pChans != 0; pChans++) {
		if (*pChans == chan)
			return ERR_NO_ERROR;
	}

	return ERR_OUT_CHANNEL_NO_SUPPORT;
}

// .............. firmware management ................

static int32 hw_loadFirmware(uint8 addr, uint8 *fw, uint16 size)
{
	int		num;
	uint8	t;

//  local function, parameters are verified by caller

  	// hold on mcu
	i2c_write_page(addr, 0x04);
	t = i2c_read_byte(addr, 0x52);
//	DBG_OUT(" 4_52 %X\n", t);
	i2c_write_byte(addr, 0x52, t & 0xFB);

	i2c_write_byte(addr, 0x5b, 0x9e);
	i2c_write_byte(addr, 0x5b, 0xb3);

	// load firmware
	i2c_write_page(addr, 0x07);
	while(size > 0) {	// block by block
 		num = i2c_write_block(addr, 0x07, size, fw);
		if (num < 0)
			return ERR_I2C_BLOCK_WRITE;
//		DBG_OUT("fw %d writen\n", num);
		size -= (uint16)num;
		fw   += num;
	}

 	// run firmware
	i2c_write_page(addr, 0x04);
	i2c_write_byte(addr, 0x52, t);
	msleep(FIRMWARE_LOAD_DELAY); // MTK-Modify

	return ERR_NO_ERROR;
}

//---------------- communicate with firmware ---------------

#define MCU_DATA_PAGE			0x01
static uint8 es_map[8] = {
	0x26, 0x27, 0x42, 0x43, 0x44, 0x45, 0x46, 0x51,
};

#ifdef	ARCH_ARMv5		// embedded OS
#define	MAX_REQUEST_RETRIES		1024
#define EACH_REQUEST_SLEEP		1
#else
#define	MAX_REQUEST_RETRIES		200
#define EACH_REQUEST_SLEEP		10
#endif
static int32 fw_McuRequest(uint8 addr, uint8 reqIdx, uint8 reqCmd,
		uint8 *buf, int num)
{
	int		j, i;

	// Send Host Request
	i2c_write_page(addr, 0x00);
	if (0xFF != reqIdx) i2c_write_byte(addr, 0x50, reqIdx);
	i2c_write_byte(addr, 0x4F, reqCmd);

	// try to read
	for(j = MAX_REQUEST_RETRIES; j != 0; j--) {
		msleep(EACH_REQUEST_SLEEP); // MTK-Modify
		if (0x40 & i2c_read_byte(addr, 0x4F))	continue;	// wait

//		printk("fw_McuRequest tries %d\n", MAX_REQUEST_RETRIES - j);
		if (0x80 & i2c_read_byte(addr, 0x50)) {
			printk("fw_McuRequest MCU responce error %d\n", j); // MTK-Modify
			return ERR_MCU_RESPONCE;
		}

		// read responce data
		if (num > 0) {
			i2c_write_page(addr, MCU_DATA_PAGE);
			for(i = 0; i < num; i++) buf[i] = i2c_read_byte(addr, es_map[i]);
		}
		return ERR_NO_ERROR;
	}

	printk("fw_McuRequest timeout\n"); // MTK-Modify
	return ERR_GETRQUEST_TIMEOUT;
}

#define FW703X_CFG_SIZE		7
#define fw_getMcuVersion(addr, pCfg)		\
		fw_McuRequest(addr, 0xFF, 0x5F, (uint8 *)pCfg, FW703X_CFG_SIZE)

static int32 fw_getEdidBlock(uint8 addr, uint8 * buf, uint8 block)
{
	uint8	i;
	int32	ret = ERR_NO_ERROR;

	//block = (block & 0x80) | (block << 1);
	for(i = block + 16; block != i; block++, buf += 8) {
		ret = fw_McuRequest(addr, block, 0x41, buf, 8);
		if (ERR_NO_ERROR != ret) {
			printk("fail to get EDID %02X\n", block); // MTK-Modify
			break;
		}
	}

	return ret;
}


#define fw_acquireI2C(addr)
#define fw_releaseI2C(addr)

static uint8 fw_getConnection(uint8 addr, int delay)
{
	uint8	t;

	delay = delay;

	i2c_write_page(addr, 0x00);
	t = i2c_read_byte(addr, 0x4E);

//	DBG_OUT("fw_getConnection %X\n", t);
	return t;
}

#define fw_setChanPower(addr, chan)		fw_McuRequest(addr, chan, 0x47, NULL, 0)

//================= generate register setting table =====================

#define HS_TOLERANCE_LEVEL0			0
#define HS_TOLERANCE_LEVEL1			1
#define HS_TOLERANCE_LEVEL2			3
#define HS_TOLERANCE_LEVEL3			7

#define RST_BIT_HSYNC				0
#define RST_BIT_VSYNC				1

#define MEM_CLK_FREQ_MAX			160000	// -+wyc 166000	// 166MHz
#define FBA_INC_GATE				2048	// from design

#define THRRL_ADJUST_DEF			200
#define THRRL_ADJUST_OP1			250		// When single frame mode, use this if picture not stable
#define THRRL_ADJUST_OP2			300		// when single frame mode, use this if picture not stable

typedef struct{
	uint32 mclk_khz;		// Memory clock
	// calculate related:
	uint32 uclkod_sel;		// Clock tree path select, refer to clock tree
	uint32 dpclk_sel;		// DPCLK from GCLK or CRYSTAL
	uint32 dat16_32b;		// 1: 16 bit sdram - 0: 32 bit sdram, depend on chip type
	uint32 true24;			// true color select - 32 bits including one pixel
	uint32 true_com;		// combined true color select - 24bits including one pixel
	// filter:
	uint32 dither_filter_enable;	// indicate if enable dither filter
	uint32 hscale_ratio_gate;		// horizontal scaling filter enable control
}CH703x_PREFER_INFO;

static int32 check_input_info(CH703x_INPUT_INFO* pInput)
{
	// HTI:
	if (pInput->timing.ht == 0 || pInput->timing.ht >= 4096)
		return ERR_INPUT_HTI;
	// HAI:
	if (pInput->timing.ha == 0 || pInput->timing.ha >= 2048)
		return ERR_INPUT_HAI;
	// HOI:
	if (pInput->timing.ho >= 2048)
		return ERR_INPUT_HOI;
	if (pInput->if_type == IF_CPU) {
		if(pInput->timing.ho != 0)
			return ERR_INPUT_HOI;
	} else {
		if(pInput->timing.ho == 0)
			return ERR_INPUT_HOI;
	}
	// HWI:
	if (pInput->timing.hw >= 2048)
		return ERR_INPUT_HWI;
	if (pInput->if_type == IF_CPU) {
		if (pInput->timing.hw != 1)
			return ERR_INPUT_HWI;
	} else {
		if (pInput->timing.hw == 0)
			return ERR_INPUT_HWI;
	}
	// VTI:
	if (pInput->timing.vt == 0 || pInput->timing.vt >= 2048)
		return ERR_INPUT_VTI;
	// VAI:
	if (pInput->timing.va == 0 || pInput->timing.va >= 2048)
		return ERR_INPUT_VAI;
	// VOI:
	if (pInput->timing.vo >= 2048)
		return ERR_INPUT_VOI;
	if (pInput->if_type == IF_CPU) {
		if (pInput->timing.vo != 0)
			return ERR_INPUT_VOI;
	} else {
		if (pInput->timing.vo == 0)
			return ERR_INPUT_VOI;
	}
	// VWI:
	if (pInput->timing.vw >= 2048)
		return ERR_INPUT_VWI;
	if (pInput->if_type == IF_CPU) {
		if (pInput->timing.vw != 1)
			return ERR_INPUT_VWI;
	} else {
		if (pInput->timing.vw == 0)
			return ERR_INPUT_VWI;
	}

	// HT > HA + HO + HW - HT = HA
	if (pInput->if_type == IF_CPU) {
		if (pInput->timing.ht != pInput->timing.ha)
			return ERR_INPUT_HT_HA_NOEQUAL;
		if (pInput->timing.vt != pInput->timing.va)
			return ERR_INPUT_VT_VA_NOEQUAL;
	} else {
		if (pInput->timing.ht <= pInput->timing.ha + pInput->timing.ho + pInput->timing.hw)
			return ERR_INPUT_H_TIMING_NOMATCH;
		if (pInput->timing.vt <= pInput->timing.va + pInput->timing.vo + pInput->timing.vw)
			return ERR_INPUT_V_TIMING_NOMATCH;
	}

	// Need more check ???

	return ERR_NO_ERROR;
}

static int32 check_output_info(CH703x_OUTPUT_INFO* pOutput)
{
	uint8 intlc_hdmi, intlc_hdtv, intlc_vga, intlc_lvds;
	uint8 no_bypass = 0;

// -wyc, use hw_checkOutpuChannel instead
//  	if (pOutput->channel == 0)
//		return ERR_NO_OUT_CHANNEL_SELECTED;
//	if ((pOutput->channel & CHANNEL_LVDS) && (pOutput->channel & CHANNEL_HDMI))
//		return ERR_OUT_CHANNEL_NO_SUPPORT;
//	if ((pOutput->channel & CHANNEL_HDTV) && (pOutput->channel & CHANNEL_VGA))
//		return ERR_OUT_CHANNEL_NO_SUPPORT;
//	if ((pOutput->channel & CHANNEL_HDMI) && (pOutput->channel & CHANNEL_VGA1)) // wyc
//		return ERR_OUT_CHANNEL_NO_SUPPORT;

	// HTO:
	if (pOutput->timing.ht == 0 || pOutput->timing.ht >= 4096)
		return ERR_OUTPUT_HTO;
	// HAO:
	if (pOutput->timing.ha == 0 || pOutput->timing.ha >= 2048)
		return ERR_OUTPUT_HAO;
	// HOO:
	if (pOutput->timing.ho == 0 || pOutput->timing.ho >= 2048)
		return ERR_OUTPUT_HOO;
	// HWO:
	if (pOutput->timing.hw == 0 || pOutput->timing.hw >= 2048)
		return ERR_OUTPUT_HWO;
	// VTO:
	if (pOutput->timing.vt == 0 || pOutput->timing.vt >= 2048)
		return ERR_OUTPUT_VTO;
	// VAO:
	if (pOutput->timing.va == 0 || pOutput->timing.va >= 2048)
		return ERR_OUTPUT_VAO;
	// VOO:
	if (pOutput->timing.vo == 0 || pOutput->timing.vo >= 2048)
		return ERR_OUTPUT_VOO;
	// VWO:
	if (pOutput->timing.vw == 0 || pOutput->timing.vw >= 2048)
		return ERR_OUTPUT_VWO;
	// CLOCK:
	if (pOutput->uclk_khz == 0)
		return ERR_OUTPUT_CLOCK;

	// HT > HA + HO + HW
	if (pOutput->timing.ht <= pOutput->timing.ha + pOutput->timing.ho + pOutput->timing.hw)
		return ERR_OUTPUT_H_TIMING_NOMATCH;
	// VT > VA + VO + VW
	if (pOutput->timing.vt <= pOutput->timing.va + pOutput->timing.vo + pOutput->timing.vw)
		return ERR_OUTPUT_V_TIMING_NOMATCH;

	// down sample percent:
	if (pOutput->ds_percent_h > DOWN_SCALING_MAX)
		return ERR_H_DOWN_SCALE_TOO_MUCH;
	if (pOutput->ds_percent_v > DOWN_SCALING_MAX)
		return ERR_V_DOWN_SCALE_TOO_MUCH;

	// HDTV not support down sample: - 2010.08.05
	if (pOutput->channel & CHANNEL_HDTV) {
		if (pOutput->ds_percent_h != 0 || pOutput->ds_percent_v != 0)
			return ERR_HDTV_WITH_DS;
	}

	// Interlace mode conflick or not: - 2010.08.05
	intlc_vga  = intlc_lvds = intlc_hdmi = intlc_hdtv = 0;
	if (pOutput->channel & CHANNEL_HDMI) {
		// interlace or progressive:
		switch (pOutput->hdmi_fmt.format_index) {
		case  5:
		case  6:
		case  7:
		case 10:
		case 11:
		case 20:
		case 21:
		case 22:
		case 25:
		case 26:
			intlc_hdmi = 1;
			break;
		}
	}
	if (pOutput->channel & CHANNEL_HDTV) {
		switch (pOutput->hdmi_fmt.format_index) {
		case 6:
		case 7:
		case 8:
		case 17:
			intlc_hdtv = 1;
			break;
		}
	}
	if ((pOutput->channel & CHANNEL_HDMI) && (pOutput->hdmi_fmt.bypass == 0)) {
		if ((pOutput->channel & (CHANNEL_VGA|CHANNEL_VGA1)) && (pOutput->vga_fmt.bypass == 0)) {
			if (intlc_hdmi != intlc_vga)
				return ERR_INT_PRO_CONFLICK;
		}
		if ((pOutput->channel & CHANNEL_HDTV) && (pOutput->hdtv_fmt.bypass == 0)) {
			if (intlc_hdmi != intlc_hdtv)
				return ERR_INT_PRO_CONFLICK;
		}
	}
	if ((pOutput->channel & CHANNEL_LVDS) && (pOutput->lvds_fmt.bypass == 0)) {
		if ((pOutput->channel & (CHANNEL_VGA|CHANNEL_VGA1)) && (pOutput->vga_fmt.bypass == 0)) {
			if (intlc_lvds != intlc_vga)
				return ERR_INT_PRO_CONFLICK;
		}
		if ((pOutput->channel & CHANNEL_HDTV) && (pOutput->hdtv_fmt.bypass ==0 )) {
			if (intlc_lvds != intlc_hdtv)
				return ERR_INT_PRO_CONFLICK;
		}
	}

	// init all bypass mode:
	no_bypass = 0;
	if ((pOutput->channel & CHANNEL_HDTV) && (pOutput->hdtv_fmt.bypass == 0))
		no_bypass++;
	if ((pOutput->channel & (CHANNEL_VGA|CHANNEL_VGA1)) && (pOutput->vga_fmt.bypass == 0))
		no_bypass++;
	if ((pOutput->channel & CHANNEL_HDMI) && (pOutput->hdmi_fmt.bypass == 0))
		no_bypass++;
	if ((pOutput->channel & CHANNEL_LVDS) && (pOutput->lvds_fmt.bypass == 0))
		no_bypass++;
	all_bypass_mode = no_bypass ? 0 : 1;

	// all bypass mode check:
	if (all_bypass_mode) {
		if (pOutput->ds_percent_h || pOutput->ds_percent_v)
			return ERR_BYPASS_WITH_DOWNSCALE;

		if (pOutput->rotate != ROTATE_NO)
			return ERR_BYPASS_WITH_ROTATE;

		if (pOutput->h_flip || pOutput->v_flip)
			return ERR_BYPASS_WITH_FLIP;
	}

	// Need more check ???
	if (pOutput->channel & CHANNEL_LVDS) {
		// check spwg parameters: T1 - T5:
		if (pOutput->lvds_fmt.spwg_t1 < LVDS_SPWG_T1_MIN || pOutput->lvds_fmt.spwg_t1 > LVDS_SPWG_T1_MAX)
			return ERR_LVDS_SPWG_PARAM_T1;
		if (pOutput->lvds_fmt.spwg_t2 < LVDS_SPWG_T2_MIN || pOutput->lvds_fmt.spwg_t2 > LVDS_SPWG_T2_MAX)
			return ERR_LVDS_SPWG_PARAM_T2;
		if (pOutput->lvds_fmt.spwg_t3 < LVDS_SPWG_T3_MIN || pOutput->lvds_fmt.spwg_t3 > LVDS_SPWG_T3_MAX)
			return ERR_LVDS_SPWG_PARAM_T3;
		if (pOutput->lvds_fmt.spwg_t4 < LVDS_SPWG_T4_MIN || pOutput->lvds_fmt.spwg_t4 > LVDS_SPWG_T4_MAX)
			return ERR_LVDS_SPWG_PARAM_T4;
		if (pOutput->lvds_fmt.spwg_t5 < LVDS_SPWG_T5_MIN || pOutput->lvds_fmt.spwg_t5 > LVDS_SPWG_T5_MAX)
			return ERR_LVDS_SPWG_PARAM_T5;
	}

	return ERR_NO_ERROR;
}

static int32 cal_prepare(CH703x_INPUT_INFO* pInput,
	CH703x_OUTPUT_INFO* pOutput,
	CH703x_PREFER_INFO* pPrefer)
{
	uint32 C;
	uint32 hao_t, vao_t, vai_t, val_t, r_val;
	uint32 hai_down;
	uint32 hai_sdram;
	uint32 lnsel;
	uint32 intlc;
	uint32 blk_h;
	uint32 fba_inc;
	uint32 bandwidth;
	uint32 frame_rate_in, frame_rate_out, field_rate_out;
	uint32 error_tmp;
	int32  ret;

	// reset local assist variables:
	reset_local_vars();

	// parameters directly check added: - 2010.06.29
	if (ERR_NO_ERROR != (ret = check_input_info(pInput)))
		return ret;
	if (ERR_NO_ERROR != (ret = check_output_info(pOutput)))
		return ret;
//	if (ERR_NO_ERROR != (ret = check_prefer_info(pPrefer)))
//		return ret;

	// pre - adjust: make necessary adjustion to parameters, always success!

	// pre 1. rotate and vflip not support 1:1 scaling:
	if (pOutput->rotate != ROTATE_NO || pOutput->h_flip || pOutput->v_flip) {
		if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
			if (pInput->timing.ha == pOutput->timing.va) {
				pInput->timing.ha--; // cheat it...
			}
		} else {
			if(pInput->timing.va == pOutput->timing.va) {
				pInput->timing.va--; // cheat it...
			}
		}
	}
	// pre 2. HAI and VAI should be even:
	if (pInput->timing.ha & 1) {
		pInput->timing.ha--;	// or return ch_false ???
	}
	if (pInput->timing.va & 1) {
		pInput->timing.va--;	// or return ch_false ???
	}

	// if all bypass mode, no need to check:
	if (all_bypass_mode) {
		// reset chip:
		iicbuf_reset();

		// always success:
		return ERR_NO_ERROR;
	}

	// OK! parameters directly check passed, start to check the limitations...

	// 1. bandwidth: <= 166MHz
	// 1.1 prepare HAI_DOWN...
	if (pPrefer->dat16_32b) {
		C = 100;
	} else if(pPrefer->true24) {
		C = 100;
	} else {
		C = (pPrefer->true_com) ? 75 : 50;
	}
	if (pOutput->ds_percent_h) {
		hao_t = pOutput->timing.ha * (100 - pOutput->ds_percent_h) / 100;
	} else {
		hao_t = pOutput->timing.ha;
	}
	if (pOutput->ds_percent_v) {
		vao_t = pOutput->timing.va * (100 - pOutput->ds_percent_v) / 100;
	} else {
		vao_t = pOutput->timing.va;
	}
	hao_t = hao_t + (hao_t & 1);
	vao_t = vao_t + (vao_t & 1);
	hai_down = pInput->timing.ha;
	if (hai_down > hao_t && pOutput->rotate != ROTATE_90 && pOutput->rotate != ROTATE_270) {
		hai_down = hao_t;
	}

	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
		val_t = 720;
	} else {
		if(pPrefer->dat16_32b) {
			val_t = 720;
		} else {
			if (pInput->if_type == IF_INT) {
				val_t = 720;
			} else {
				val_t = pPrefer->true24 ? 720 : 1440;
			}
		}
	}
	if (hai_down > val_t &&
		(pOutput->rotate != ROTATE_NO || pOutput->h_flip || pOutput->v_flip || pInput->if_type == IF_INT)) {
		hai_down = val_t;
	}

	// 1.2 Prepare LNSEL...
	intlc = 0; // intlc should be match because we judge it in check_output_info() already......
	if (pOutput->channel & CHANNEL_HDMI) {
		// interlace or progressive:
		switch(pOutput->hdmi_fmt.format_index) {
		case  5:
		case  6:
		case  7:
		case 10:
		case 11:
		case 20:
		case 21:
		case 22:
		case 25:
		case 26:
			intlc = 1;
			break;
		default:
			intlc = 0;
			break;
		}
	}
	if (pOutput->channel & CHANNEL_HDTV) {
		switch(pOutput->hdtv_fmt.format_index) {
		case 6:
		case 7:
		case 8:
		case 17:
			intlc = 1;
			break;
		default:
			intlc = 0;
			break;
		}
	}
	// All other output channel should be progressive:
	val_t = intlc ? (vao_t / 2) : vao_t;
	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
		if (hai_down <= val_t)
			lnsel = 2; // for remove flicker - 2010.05.25
		else
			lnsel = 1;
	} else if(pOutput->rotate == ROTATE_180 || pOutput->h_flip || pOutput->v_flip) {
		if (pInput->timing.va <= val_t)
			lnsel = 2;
		else
			lnsel = 1;
	} else {
		if (pInput->timing.va <= val_t)
			lnsel = 3;
		else
			lnsel = 1;
	}
// original code WYC_TEST
//	bandwidth = (pInput->pclk_khz + pOutput->uclk_khz * pInput->timing.ha / hao_t * (4 - lnsel)) / 85 * C;
//	if (bandwidth > MEM_CLK_FREQ_MAX) {
//		if (lnsel >= 2) {
//			return ERR_BANDWIDTH_OVERFLOW;
//		}
//		// re calculate bandwidth:
//		lnsel++;
//		bandwidth = (pInput->pclk_khz + pOutput->uclk_khz * pInput->timing.ha / hao_t * (4 - lnsel)) / 85 * C;
//		if (bandwidth > MEM_CLK_FREQ_MAX) {
//			return ERR_BANDWIDTH_OVERFLOW;
//		}
//		scale_line_adjust = 1;
//	}

	while(1) {		// WYC_TEST
		bandwidth = (pInput->pclk_khz + pOutput->uclk_khz * pInput->timing.ha / hao_t * (4 - lnsel)) / 85 * C;
		if (bandwidth <= MEM_CLK_FREQ_MAX)
			break;
		if (lnsel >= 3) {
			if (pPrefer->true24)   return ERR_NO_SUPPORT_TRUE24;	// re-calculate after disable true24
			if (pPrefer->true_com) return ERR_NO_SUPPORT_TRUECOM;	// re-calculate after disable true_com
			return ERR_BANDWIDTH_OVERFLOW;	// fail due to bandwidth limit
		}
		lnsel++;
//		DBG_OUT(" increase lnsel %d to re-calculate bandwidth\n", lnsel);
	}
	lnsel_saved = lnsel;

	// 1.4. blk_h:
	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
		val_t = 100;
	} else {
		if (pPrefer->dat16_32b) {
			val_t = 100;
		} else if(pPrefer->true24) {
			val_t = 100;
		} else {
			val_t = pPrefer->true_com ? 75 : 50;
		}
	}
	if (val_t == 75) {
		hai_sdram = (hai_down / 4) * 3 + (hai_down & 3);  // (hai_down % 4);
	} else {
		hai_sdram = hai_down * val_t / 100;
	}
	if (pOutput->rotate != ROTATE_NO || pOutput->h_flip || pOutput->v_flip) {
		blk_h = 45;
	} else if (hai_sdram <= 720 && pInput->timing.va <= 720) {
		blk_h = 45;
	} else {
		blk_h = (hai_sdram + 15) >> 4;
	}

	// 1.5 calculate FBA_INC first:
	if ( (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) &&
		 pPrefer->true24 == 0 &&
		 pPrefer->true_com == 0 &&
		 pPrefer->dat16_32b == 0) {
		vai_t = pInput->timing.va / 2;
	} else {
		vai_t = pInput->timing.va;
	}
	val_t = (vai_t + 15) >> 4;
	fba_inc = blk_h * val_t;

	// 1.6 calculate frame rate - 2010.08.05
	frame_rate_in = ((pInput->pclk_khz * 10000) / pInput->timing.ht) * 10 / pInput->timing.vt;
	frame_rate_out = ((pOutput->uclk_khz * 10000) / pOutput->timing.ht) * 10 / pOutput->timing.vt;
	field_rate_out = intlc ? (frame_rate_out * 2) : (frame_rate_out);
	r_val = frame_rate_in * 100 / field_rate_out;

	// 2. 16bit sdram:
	if (pPrefer->dat16_32b) {
		if (pPrefer->true24) {
			return ERR_NO_SUPPORT_TRUE24;
		}
		if (pPrefer->true_com) {
			return ERR_NO_SUPPORT_TRUECOM;
		}
		if (pOutput->rotate == ROTATE_NO && !pOutput->h_flip &&
		    !pOutput->v_flip && fba_inc > FBA_INC_GATE) {
			val_t = 4096 * 100 / fba_inc;
			// modify the arithmetic - 2010.08.05
			if (r_val <= 100) {
				if (val_t < (200 - r_val)) {
					return ERR_RESOLUTION_OVERFLOW;
				}
			} else {  // r_val > 100
				if (val_t < (200 - 100 * 100 / r_val)) {
					return ERR_RESOLUTION_OVERFLOW;
				}
			}
		} else if (pOutput->rotate != ROTATE_NO) { 	// rotate enabled
			if (pInput->timing.va > 720) {
				return ERR_ROTATION_WITH_VAI;
			}
		} else { // flip enabled
			if (pInput->timing.va > 720) {
				return ERR_FLIP_WITH_VAI;
			}
		}
	}

	// 3. 32bit sdram:
	if (pPrefer->dat16_32b == 0) {
		if (pOutput->rotate == ROTATE_NO && !pOutput->h_flip &&
		    !pOutput->v_flip && fba_inc > FBA_INC_GATE) {
			error_tmp = 0;
			val_t = 4096 * 100 / fba_inc;

			// modify the arithmetic - 2010.08.05
			if (r_val <= 100) {
				if (val_t < 200 - r_val) {
					error_tmp = 1;
				}
			} else {  // r_val > 100
				if (val_t < (200 - 100 * 100 / r_val)) {
					error_tmp = 1;
				}
			}
			if (error_tmp) {
				if (pPrefer->true24) {
					return ERR_NO_SUPPORT_TRUE24;
				}
				if(pPrefer->true_com) {
					return ERR_NO_SUPPORT_TRUECOM;
				}
				return ERR_RESOLUTION_OVERFLOW;
			}
		}
		if (pOutput->rotate == ROTATE_180) {
			if (pPrefer->true_com) {
				return ERR_NO_SUPPORT_TRUECOM;
			}
			if (pPrefer->true24) {
				if (pInput->timing.va > 720) {
					return ERR_NO_SUPPORT_TRUE24;
				}
			} else {
				if (pInput->timing.va > 720) {
					return ERR_ROTATION_WITH_VAI;
				}
			}
		}
		if (pOutput->h_flip || pOutput->v_flip) {
			if (pPrefer->true_com) {
				return ERR_NO_SUPPORT_TRUECOM;
			}
			if (pInput->timing.va > 720) {
				return ERR_FLIP_WITH_VAI;
			}
		}
		if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
			if (pPrefer->true_com) {
				return ERR_NO_SUPPORT_TRUECOM;
			}
			if (pPrefer->true24) {
				if (pInput->timing.va > 720) {
					return ERR_NO_SUPPORT_TRUE24;
				}
			} else {
				if (pInput->timing.va > 1440) {
					return ERR_ROTATION_WITH_VAI;
				}
			}
		}
	}

	// 4. limitation added - 2010.06.23
	if ((pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) &&
	    (pInput->timing.va > hao_t)) {
		return ERR_DOWNSCALE_WITH_ROTATE;
	}

	// 5. limitation added for interlace input: - 2010.08.05 @ modified 2010.09.09
	if (pInput->if_type == IF_INT) {
		// Interlace not support rotate
		if (pOutput->rotate != ROTATE_NO) {
			return ERR_INTERLACE_WITH_ROTATE;
		}

		// interlaced input VAI max = 720
		if (pInput->timing.va > 720) {
			return ERR_RESOLUTION_OVERFLOW;
		}
	}

	// remove the wrfast limitaion which is replaced with GAP_EN adjust......

	// Misc adjust:
	// 1. dither filter vs. true color:
	if (pPrefer->true24 || pPrefer->true_com) {
		pPrefer->dither_filter_enable = 0;
	}

	// OK! let's start to set up chip:

	// reset chip:
	iicbuf_reset();

	// stop memory first:
	iicbuf_write_ex(STOP, 1);

	return ERR_NO_ERROR;
}

static int32 set_input_info(CH703x_INPUT_INFO* pInput)
{
	uint8 intlace_reg, cpuen_reg;

	// interface type:
	switch(pInput->if_type) {
	case IF_PRO:
		intlace_reg = 0;
		cpuen_reg = 0;
		break;
	case IF_INT:
		intlace_reg = 1;
		cpuen_reg = 0;
		break;
	case IF_CPU:
		intlace_reg = 0;
		cpuen_reg = 1;
		break;
	}
	iicbuf_write_ex(INTLACE, intlace_reg);
	iicbuf_write_ex(CPUEN, cpuen_reg);
	// timing:
	iicbuf_write_ex(HTI, pInput->timing.ht);
	iicbuf_write_ex(HAI, pInput->timing.ha);
	iicbuf_write_ex(HOI, pInput->timing.ho);
	iicbuf_write_ex(HWI, pInput->timing.hw);
	iicbuf_write_ex(VTI, pInput->timing.vt);
	iicbuf_write_ex(VAI, pInput->timing.va);
	iicbuf_write_ex(VOI, pInput->timing.vo);
	iicbuf_write_ex(VWI, pInput->timing.vw);
	// pixel clock frequency:
	iicbuf_write_ex(GCLKFREQ, pInput->pclk_khz);
	// pixel format:
	iicbuf_write_ex(IDF, pInput->pixel_fmt.format);
	// if yc input:
	if (pInput->pixel_fmt.format >= 5 && pInput->pixel_fmt.format <= 7) {
		iicbuf_write_ex(YC2RGB, 1);
	}
	iicbuf_write_ex(REVERSE, pInput->pixel_fmt.bit_swap);
	iicbuf_write_ex(SWAP, pInput->pixel_fmt.byte_swap);
	iicbuf_write_ex(HIGH, pInput->pixel_fmt.byte_align);
	iicbuf_write_ex(MULTI, pInput->pixel_fmt.multiplexed);
	if (pInput->pixel_fmt.multiplexed == MULTI_2X) {
		iicbuf_write_ex(SWP_PAPB, pInput->pixel_fmt.m2x_fmt.halfword_swap);
		iicbuf_write_ex(AH_LB, pInput->pixel_fmt.m2x_fmt.halfword_align);
		iicbuf_write_ex(SWP_YC, pInput->pixel_fmt.m2x_fmt.yc_swap);
	}
	if (pInput->pixel_fmt.multiplexed == MULTI_3X) {
		iicbuf_write_ex(POS3X, pInput->pixel_fmt.m3x_fmt.byte_align);
	}
	// embedded sync:
	iicbuf_write_ex(DES, pInput->pixel_fmt.embedded_sync);
	// sync polarity:
	iicbuf_write_ex(HPO_I, pInput->hs_pol);
	iicbuf_write_ex(VPO_I, pInput->vs_pol);
	iicbuf_write_ex(DEPO_I, pInput->de_pol);
	// audio related:
	iicbuf_write_ex(I2S_SPDIFB, pInput->audio_type);
	if (pInput->audio_type == AUDIO_SPDIF) {
		iicbuf_write_ex(SPDIF_MODE, pInput->spdif_mode);
	} else { 	// if (pInput->audio_type == AUDIO_I2S) wyc
		iicbuf_write_ex(I2SPOL, pInput->i2s_pol);
		iicbuf_write_ex(I2S_LENGTH, pInput->i2s_len);
		iicbuf_write_ex(I2SFMT, pInput->i2s_fmt);
		g_nCurRegmap[1][0x4F] |= 0xC0;
	}
	// crystal frequency:
	if (pInput->crystal_used) {
		iicbuf_write_ex(CRYSFREQ, pInput->crystal_khz);
	}

	return ERR_NO_ERROR;
}

static int32 set_output_info(CH703x_OUTPUT_INFO* pOutput)
{
	uint8 hd_lv_seq, hd_lv_pol, hsyncp, vsyncp, dep;
	uint8 hpo_o, vpo_o, depo_o, vfmt;
	uint8 hd_dvib, intlc, copy, hsp, vsp, m1m0, c1c0, vic, pr;
	uint32 hao_down, vao_down;
	uint8 pwm_index;

	LVDS_FMT *pLvdsFmt = &(pOutput->lvds_fmt);
	HDMI_FMT *pHdmiFmt = &(pOutput->hdmi_fmt);
	VGA_FMT  *pVgaFmt  = &(pOutput->vga_fmt);
	HDTV_FMT *pHdtvFmt = &(pOutput->hdtv_fmt);

	// Active and Total:
	iicbuf_write_ex(HTO, pOutput->timing.ht);
	iicbuf_write_ex(HAO, pOutput->timing.ha);
	iicbuf_write_ex(VTO, pOutput->timing.vt);
	iicbuf_write_ex(VAO, pOutput->timing.va);
 	iicbuf_write_ex(UCLKFREQ, pOutput->uclk_khz);

	// timing: if all bypass (scaling enabled), write timing...
	if (!all_bypass_mode) {
		//down scaling:
		hao_down = pOutput->timing.ha * (100 - pOutput->ds_percent_h) / 100;
		hao_down = hao_down + (hao_down & 1);
		vao_down = pOutput->timing.va * (100 - pOutput->ds_percent_v) / 100;
		vao_down = vao_down + (vao_down & 1);
		if (pOutput->ds_percent_h || pOutput->ds_percent_v) {
			iicbuf_write_ex(SCAN_EN, 1);
			iicbuf_write_ex(HAO_SCL, hao_down);
			iicbuf_write_ex(VAO_SCL, vao_down);
		}
		//feature related:
		iicbuf_write_ex(ROTATE, pOutput->rotate);
		iicbuf_write_ex(HFLIP, pOutput->h_flip);
		iicbuf_write_ex(VFLIP, pOutput->v_flip);
	}

	// HDTV:
	if (pOutput->channel & CHANNEL_HDTV) {
		// Offset & sync width:
		iicbuf_write_ex(HOO, pOutput->timing.ho);
		iicbuf_write_ex(HWO, pOutput->timing.hw);
		iicbuf_write_ex(VOO, pOutput->timing.vo);
		iicbuf_write_ex(VWO, pOutput->timing.vw);
		// format:
		iicbuf_write_ex(HDTVEN, 1);
		if (pHdtvFmt->format_index > 17) {
			return ERR_HDTV_FMT_INDEX_WRONG;
		}
		iicbuf_write_ex(HDVDOFM, pHdtvFmt->format_index);
		// interlace or not:
		switch(pHdtvFmt->format_index)
		{
		case 6:
		case 7:
		case 8:
		case 17:
			intlc = 1;
			break;
		default:
			intlc = 0;
			break;
		}
		iicbuf_write_ex(INTLC, intlc);
		// channel swap:
		iicbuf_write_ex(DACSP, pHdtvFmt->channel_swap);

		// RevD added: - 2010.12.13
		iicbuf_write_ex(HD_VGA_SEL, 1);
		// bypass mode:
		iicbuf_write_ex(HDTVCLK_BP, pHdtvFmt->bypass ? 1 : 0);
		iicbuf_write_ex(HDTV_BP, pHdtvFmt->bypass ? 1 : 0);
	}

	// LVDS:
	if (pOutput->channel & CHANNEL_LVDS) {
		// bypass or not:
		iicbuf_write_ex(LV_VGA_FRB, pLvdsFmt->bypass ? 0 : 1);
		// Offset and Sync width:
		iicbuf_write_ex(HOO_HDMI, pOutput->timing.ho);
		iicbuf_write_ex(HWO_HDMI, pOutput->timing.hw);
		iicbuf_write_ex(VOO_HDMI, pOutput->timing.vo);
		iicbuf_write_ex(VWO_HDMI, pOutput->timing.vw);
		// channel swap control:
		hd_lv_seq = pLvdsFmt->channel_swap;
		iicbuf_write_ex(HD_LV_SEQ, hd_lv_seq);
		// channel polarity:
		hd_lv_pol = pLvdsFmt->channel_pol;
		iicbuf_write_ex(HD_LV_POL, hd_lv_pol);
		// Sync polarity:
		hsyncp = pLvdsFmt->hs_pol;
		vsyncp = pLvdsFmt->vs_pol;
		dep = pLvdsFmt->de_pol;
		iicbuf_write_ex(HSYNCP, hsyncp ? 0 : 1);
		iicbuf_write_ex(VSYNCP, vsyncp ? 0 : 1);
		iicbuf_write_ex(DEP, dep ? 0 : 1);
		// lvds select:
		iicbuf_write_ex(HDMI_LVDS_SEL, 0);
		// pannel enable:
		iicbuf_write_ex(PWM_EN, 1);	// 2010.06.18
		// pwm freq:
		pwm_index = pwm_freq_to_index(pLvdsFmt->pwm_freq);
		iicbuf_write_ex(PWM_INDEX, pwm_index);
		// pwm duty:
		iicbuf_write_ex(PWM_DUTY, pLvdsFmt->pwm_duty);
		iicbuf_write_ex(PWM_INV,  pLvdsFmt->pwm_inv);	// WYC_ADD
		// SPWG parameters:
		iicbuf_write_ex(PRGT1, pLvdsFmt->spwg_t1);
		iicbuf_write_ex(PRGT2, pLvdsFmt->spwg_t2);
		iicbuf_write_ex(PRGT3, pLvdsFmt->spwg_t3);
		iicbuf_write_ex(PRGT4, pLvdsFmt->spwg_t4);
		iicbuf_write_ex(PRGT5, pLvdsFmt->spwg_t5);

		// RevD added: - 2010.12.13
		iicbuf_write_ex(HM_LV_SEL, 0);
		// bypass:
		iicbuf_write_ex(LVDSCLK_BP, pLvdsFmt->bypass ? 1 : 0);
		// lvds_bypass_mode, add on 2011-11-11
		iicbuf_write_ex(DRI_RON, pLvdsFmt->bypass ? 0 : 1);
	}
	// VGA:
	if (pOutput->channel & (CHANNEL_VGA|CHANNEL_VGA1)) {
		// Offset and Sync width:
		iicbuf_write_ex(HOO_HDMI, pOutput->timing.ho);
		iicbuf_write_ex(HWO_HDMI, pOutput->timing.hw);
		iicbuf_write_ex(VOO_HDMI, pOutput->timing.vo);
		iicbuf_write_ex(VWO_HDMI, pOutput->timing.vw);
		// polarity:
		hpo_o = pVgaFmt->hs_pol & 0x1;
		vpo_o = pVgaFmt->vs_pol & 0x1;
		depo_o = pVgaFmt->de_pol & 0x1;
		iicbuf_write_ex(HPO_O, hpo_o);
		iicbuf_write_ex(VPO_O, vpo_o);
		iicbuf_write_ex(DEPO_O, depo_o);
		// vfmt:
		vfmt = (pVgaFmt->bypass) ? 9 : 8;
		iicbuf_write_ex(VFMT, vfmt);
		// channel swap:
		iicbuf_write_ex(DACSP, pVgaFmt->channel_swap);
		// sync swap:
		iicbuf_write_ex(SYNCS, pVgaFmt->sync_swap);
		// csync type:
		iicbuf_write_ex(CSSEL, pVgaFmt->csync_type);

		// RevD added: 2010.12.14
		iicbuf_write_ex(HD_VGA_SEL, 0);
		iicbuf_write_ex(HDTVEN, 0);
		// bypass:
		iicbuf_write_ex(VGACLK_BP, pVgaFmt->bypass ? 1 : 0);
		if (!(pOutput->channel & CHANNEL_VGA)) {	// channel selection, +wyc 2011-8-15
			iicbuf_write_ex(VGACH_SW, 1);
			iicbuf_write_ex(SEL_R, 0);
		} // default is channel 0
	}

	// HDMI:
	if (pOutput->channel & CHANNEL_HDMI) {
		// Offset and Sync width:
		iicbuf_write_ex(HOO_HDMI, pOutput->timing.ho);
		iicbuf_write_ex(HWO_HDMI, pOutput->timing.hw);
		iicbuf_write_ex(VOO_HDMI, pOutput->timing.vo);
		iicbuf_write_ex(VWO_HDMI, pOutput->timing.vw);
		// Is DVI mode?
		hd_dvib = pHdmiFmt->is_dvi_mode ? 0 : 1;
		iicbuf_write_ex(HD_DVIB, hd_dvib);
		// interlace or progressive:
		switch(pHdmiFmt->format_index) {
		case  5:
		case  6:
		case  7:
		case 10:
		case 11:
		case 20:
		case 21:
		case 22:
		case 25:
		case 26:
			intlc = 1;
			break;
		default:
			intlc = 0;
			break;
		}
		iicbuf_write_ex(INTLC, intlc);
		// PR: pixel repeat:
		switch(pHdmiFmt->format_index) {
		case 6:
		case 7:
		case 8:
		case 9:
		case 21:
		case 22:
		case 23:
		case 24:
			pr = 2;
			break;
		case 10:
		case 11:
		case 12:
		case 13:
		case 25:
		case 26:
		case 27:
		case 28:
			pr = 4;
			break;
		case 14:
		case 15:
		case 29:
		case 30:
			pr = 2;
			break;
		case 35:
		case 36:
		case 37:
		case 38:
			pr = 2;
			break;
		default:
			pr = 0;
			break;
		}
		iicbuf_write_ex(PR, pr);
		// COPY fixed to 1:
		copy = 1;
		iicbuf_write_ex(COPY, copy);
		// data polarity invert?
		hd_lv_pol = pHdmiFmt->data_pol_invert;
		iicbuf_write_ex(HD_LV_POL, hd_lv_pol);
		// data & channel swap:
		hd_lv_seq = pHdmiFmt->channel_swap;
		iicbuf_write_ex(HD_LV_SEQ, hd_lv_seq);
		// HDMI_LVDS_SEL fixed to 1:
		iicbuf_write_ex(HDMI_LVDS_SEL, 1);
		// Panel enable for lvds:
		iicbuf_write_ex(PWM_EN, 0);	// 2010.06.18
		// h/v sync polarity:
		hsp = pHdmiFmt->hs_pol;
		vsp = pHdmiFmt->vs_pol;
		iicbuf_write_ex(_HSP, hsp);
		iicbuf_write_ex(_VSP, vsp);
		// M1 M0:
		// adjust first:
		switch(pHdmiFmt->format_index) {
		case 1:
			if (pHdmiFmt->aspect_ratio == AS_RATIO_16_9) {  // only support 4:3
					return ERR_ASPECT_RATIO_NOMATCH;
			}
			break;
		case 4:
		case 5:
		case 16:
		case 19:
		case 20:
		case 31:
		case 32:
		case 33:
		case 34:
			if (pHdmiFmt->aspect_ratio == AS_RATIO_4_3) { 	// only support 16:9
					return ERR_ASPECT_RATIO_NOMATCH;
			}
			break;
		default:
			break;
		}
		m1m0 = pHdmiFmt->aspect_ratio;
		iicbuf_write_ex(M1M0, m1m0);
		// C1C0: fixed to ITU709
		c1c0 = 2;
		iicbuf_write_ex(C1C0, c1c0);
		// VIC:
		vic = pHdmiFmt->format_index;
		iicbuf_write_ex(VIC, vic);

		// RevD added: 2010.12.14
		iicbuf_write_ex(HM_LV_SEL, 1);
		// bypass:
		iicbuf_write_ex(HDMI_BP, pHdmiFmt->bypass ? 1 : 0);
		iicbuf_write_ex(HDMICLK_BP, pHdmiFmt->bypass ? 1 : 0);
	}

	// Set TE to 5 for default:
	iicbuf_write_ex(TE, 5);

	// test mode:
//	if (pOutput->test_mode) {
//		iicbuf_write_ex(TEST, 1);
//		iicbuf_write_ex(TSYNC, 1);
//		iicbuf_write_ex(TSTP, 4);	// only support colorbar...
//	}
      iicbuf_write_ex(MAX_VAL, HPD_DELAY_MAX);
	return ERR_NO_ERROR;
}

static int32 set_prefer_info(CH703x_PREFER_INFO* pPrefer)
{
	uint8 dbp0;

	//memory clock:
	iicbuf_write_ex(MCLKFREQ, pPrefer->mclk_khz);
	//scaler related:
	iicbuf_write_ex(UCLKOD_SEL, pPrefer->uclkod_sel);
	iicbuf_write_ex(DAT16_32B, pPrefer->dat16_32b);
	iicbuf_write_ex(TRUE24, pPrefer->true24);
	iicbuf_write_ex(TRUE_COM, pPrefer->true_com);
	//filter:
	dbp0 = pPrefer->dither_filter_enable ? 0 : 1;
	iicbuf_write_ex(DBP0, dbp0);

	return ERR_NO_ERROR;
}

static int32 convert_pll1n1_div(ch_bool pll1n1_to_div, uint8* pll1n1_addr, uint8* div_addr)
{
	uint8 val_t;

	if (pll1n1_to_div) { 	// register value -> div value
		if ((*pll1n1_addr) <= 5) {
			(*div_addr)= 1 << (*pll1n1_addr);
		} else if((*pll1n1_addr) == 6 || (*pll1n1_addr) == 7) {
			(*div_addr) = 64;
		} else {
			return ERR_PLL1N1_WRONG;
		}

		return ERR_NO_ERROR;
	} else {  // div value -> register value
		for(val_t = 1; val_t <= 6; ++val_t) {
			if (*div_addr == (1 << val_t)) {
				*pll1n1_addr = val_t;
				return ERR_NO_ERROR;
			}
		}

		return ERR_PLL1N1_WRONG;
	}
}

static int32 convert_pll1n2_div(ch_bool pll1n2_to_div, uint8* pll1n2_addr, uint8* div_addr)
{
	uint8 val_t;

	if (pll1n2_to_div) { 	// register value -> div value
		if ((*pll1n2_addr) <= 3) {
			*div_addr = 1 << (*pll1n2_addr);
		} else if ((4 == (*pll1n2_addr)) || (6 == (*pll1n2_addr))) {
			(*div_addr) = 16;
		} else if((5 == (*pll1n2_addr)) || (7 == (*pll1n2_addr)))  {
			(*div_addr) = 32;
		} else {
			return ERR_PLL1N2_WRONG;
		}

		return ERR_NO_ERROR;
	} else { 	// div value -> register value
		for(val_t = 0; val_t <= 5; ++val_t) {
			if (*div_addr == (1 << val_t)) {
				*pll1n2_addr = val_t;
				return ERR_NO_ERROR;
			}
		}

		return ERR_PLL1N2_WRONG;
	}
}

static int32 convert_pll1n3_div(ch_bool pll1n3_to_div, uint8* pll1n3_addr, uint8* div_addr)
{
	uint8 val_t;

	if (pll1n3_to_div) { 	// register value -> div value
		if ((*pll1n3_addr) <= 3) {
			*div_addr = 1 << (*pll1n3_addr);
		} else if((4 == (*pll1n3_addr)) || (6 == (*pll1n3_addr))) {
			(*div_addr) = 16;
		} else if((5 == (*pll1n3_addr)) || (7 == (*pll1n3_addr))) {
			(*div_addr) = 32;
		} else {
			return ERR_PLL1N3_WRONG;
		}

		return ERR_NO_ERROR;
	} else { 	// div value -> register value
		for(val_t = 0; val_t <= 5; ++val_t) {
			if (*div_addr == (1 << val_t)) {
				*pll1n3_addr = val_t;
				return ERR_NO_ERROR;
			}
		}

		return ERR_PLL1N3_WRONG;
	}
}

static int32 convert_pll2n5_div(ch_bool pll2n5_to_div, uint8* pll2n5_addr, uint8* div_addr)
{
	if (pll2n5_to_div) { 	// register value -> div value
		switch (*pll2n5_addr) {
		case 0:
			*div_addr = 1;
			break;
		case 4:
			*div_addr = 2;
			break;
		case 1:
			*div_addr = 3;
			break;
		case 8:
			*div_addr = 4;
			break;
		case 2:
			*div_addr = 5;
			break;
		case 5:
			*div_addr = 6;
			break;
		case 3:
			*div_addr = 7;
			break;
		case 12:
			*div_addr = 8;
			break;
		default :
			return ERR_PLL2N5_WRONG;
		}
	} else { 	// div value -> register value
		switch(*div_addr) {
		case 1:
			*pll2n5_addr = 0;
			break;
		case 2:
			*pll2n5_addr = 4;
			break;
		case 3:
			*pll2n5_addr = 1;
			break;
		case 4:
			*pll2n5_addr = 8;
			break;
		case 5:
			*pll2n5_addr = 2;
			break;
		case 6:
			*pll2n5_addr = 5;
			break;
		case 7:
			*pll2n5_addr = 3;
			break;
		case 8:
			*pll2n5_addr = 12;
			break;
		default :
			return ERR_PLL2N5_WRONG;
		}
	}

	return ERR_NO_ERROR;
}

static int32 convert_pll2n9_div(ch_bool pll2n9_to_div, uint8* pll2n9_addr, uint8* div_addr)
{
	if (pll2n9_to_div) {
		if (*pll2n9_addr <= 3) {
			(*div_addr)= (*pll2n9_addr) + 5;
			return ERR_NO_ERROR;
		}
	} else {
		if (*div_addr >= 5 && *div_addr <= 8) {
			*pll2n9_addr = (*div_addr) - 5;
			return ERR_NO_ERROR;
		}
	}

	return ERR_PLL2N9_WRONG;
}

static int32 convert_pll2n10_div(ch_bool pll2n10_to_div, uint8* pll2n10_addr, uint8* div_addr)
{
	if (pll2n10_to_div) {
		if (*pll2n10_addr <= 3) {
			(*div_addr)= (*pll2n10_addr) + 8;
			return ERR_NO_ERROR;
		}
	} else {
		if (*div_addr >= 8 && *div_addr <= 11) {
			*pll2n10_addr = (*div_addr) - 8;
			return ERR_NO_ERROR;
		}
	}

	return ERR_PLL2N10_WRONG;
}

static int32 convert_pll2n11_div(ch_bool pll2n11_to_div, uint8* pll2n11_addr, uint8* div_addr)
{
	if (pll2n11_to_div) {
		switch (*pll2n11_addr) {
		case 0:
			*div_addr = 1;
			break;
		case 4:
			*div_addr = 2;
			break;
		case 1:
			*div_addr = 3;
			break;
		case 8:
			*div_addr = 4;
			break;
		case 2:
			*div_addr = 5;
			break;
		case 5:
			*div_addr = 6;
			break;
		case 3:
			*div_addr = 7;
			break;
		case 12:
			*div_addr = 8;
			break;
		default:
			return ERR_PLL2N11_WRONG;
		}
	} else {
		switch(*div_addr) {
		case 1:
			*pll2n11_addr = 0;
			break;
		case 2:
			*pll2n11_addr = 4;
			break;
		case 3:
			*pll2n11_addr = 1;
			break;
		case 4:
			*pll2n11_addr = 8;
			break;
		case 5:
			*pll2n11_addr = 2;
			break;
		case 6:
			*pll2n11_addr = 5;
			break;
		case 7:
			*pll2n11_addr = 3;
			break;
		case 8:
			*pll2n11_addr = 12;
			break;
		default:
			return ERR_PLL2N11_WRONG;
		}
	}

	return ERR_NO_ERROR;
}

static int32 convert_pll3n6_div(ch_bool pll3n6_to_div, uint8* pll3n6_addr, uint8* div_addr)
{
	uint8 val_t;

	if (pll3n6_to_div) { 	// register value -> div value
		if (*pll3n6_addr <= 3) {
			*div_addr = 1 << (*pll3n6_addr);
			return ERR_NO_ERROR;
		}
	} else { 	// div value -> register value
		for(val_t = 0; val_t <= 3; ++val_t) {
			if (*div_addr == (1 << val_t)) {
				*pll3n6_addr = val_t;
				return ERR_NO_ERROR;
			}
		}
	}

	return ERR_PLL3N6_WRONG;
}

static int32 convert_dmxtal_div(ch_bool dmxtal_to_div, uint8* dmxtal_addr, uint8* div_addr)
{
	if (dmxtal_to_div) { 	// register value -> div value
		switch(*dmxtal_addr) {
		case 0x00:
			*div_addr = 1;
			break;
		case 0x04:
			*div_addr = 2;
			break;
		case 0x08:
			*div_addr = 3;
			break;
		case 0x0C:
			*div_addr = 4;
			break;
		case 0x10:
			*div_addr = 5;
			break;
		case 0x11:
			*div_addr = 6;
			break;
		case 0x12:
			*div_addr = 7;
			break;
		case 0x13:
			*div_addr = 8;
			break;
		case 0x01:
			*div_addr = 9;
			break;
		case 0x14:
			*div_addr = 10;
			break;
		case 0x02:
			*div_addr = 11;
			break;
		case 0x15:
			*div_addr = 12;
			break;
		case 0x03:
			*div_addr = 13;
			break;
		case 0x16:
			*div_addr = 14;
			break;
		case 0x18:
			*div_addr = 15;
			break;
		case 0x17:
			*div_addr = 16;
			break;
		default:
			return ERR_DMXTAL_WRONG;
		}
	} else { 	// div value -> register value
		switch(*div_addr)
		{
		case 1:
			*dmxtal_addr = 0x00;
			break;
		case 2:
			*dmxtal_addr = 0x04;
			break;
		case 3:
			*dmxtal_addr = 0x08;
			break;
		case 4:
			*dmxtal_addr = 0x0C;
			break;
		case 5:
			*dmxtal_addr = 0x10;
			break;
		case  6:
			*dmxtal_addr = 0x11;
			break;
		case 7:
			*dmxtal_addr = 0x12;
			break;
		case 8:
			*dmxtal_addr = 0x13;
			break;
		case 9:
			*dmxtal_addr = 0x01;
			break;
		case 10:
			*dmxtal_addr = 0x14;
			break;
		case 11:
			*dmxtal_addr = 0x02;
			break;
		case 12:
			*dmxtal_addr = 0x15;
			break;
		case 13:
			*dmxtal_addr = 0x03;
			break;
		case 14:
			*dmxtal_addr = 0x16;
			break;
		case 15:
			*dmxtal_addr = 0x18;
			break;
		case 16:
			*dmxtal_addr = 0x17;
			break;
		default:
			return ERR_DMXTAL_WRONG;
		}
	}

	return ERR_NO_ERROR;
}

static int32 cal_and_set_clk_pll(CH703x_INPUT_INFO* pInput,
	CH703x_OUTPUT_INFO* pOutput, CH703x_PREFER_INFO* pPrefer)
{
	uint8 pll1n1_reg, pll1n1_div;
	uint8 pll1n2_reg, pll1n2_div;
	uint8 pll1n3_reg, pll1n3_div;
	uint8 pll3n6_reg, pll3n6_div;
	uint8 pll2n5_reg, pll2n5_div;
	uint8 dmxtal_reg, dmxtal_div;
	uint8 pll2n9_reg, pll2n9_div;
	uint8 pll2n10_reg, pll2n10_div;
	uint8 pll2n11_reg, pll2n11_div;

	uint8 gcksel_reg;
	uint8 xch_reg;
	uint8 dpckn4_reg;

	uint32 a2_reg;
	uint32 a1_reg;
	uint32 a3_reg;
	uint32 uclk2d_reg;
	uint8 uclksec_reg;
	uint8 dri_pll_n1_1_reg;
	uint8 dri_pll_n1_0_reg;
	uint8 dri_pll_n3_1_reg;
	uint8 dri_pll_n3_0_reg;
	uint8 dri_vcomode_reg;
	uint8 dri_pll_divsel_reg;
	uint8 dri_pll_vcosel_reg;
	uint8 dri_modsel_reg;

	uint32 clk_src_khz;
	uint32 lvds_clk_khz;
	uint32 hdmi_clk_khz;
//	uint64 a1_h, a1_l; -wyc
	uint32 a1_h, a1_l;

	uint32 val_t;
//	uint32 val_t1, val_t2; -wyc
	int32  ret;

	// 1. decide clock source:
	if (pInput->crystal_used) {
		gcksel_reg = 1;
	} else {
		gcksel_reg = 0;
	}
	iicbuf_write_ex(GCKSEL, gcksel_reg);

	// 2. DPCLK select:
//	if (pOutput->test_mode == 1) {
//		if(pInput->crystal_used) {  // if crystal used, use crystal to generate test pattern...
//			pPrefer->dpclk_sel = 0; // from crystal...
//		}
//	}
	iicbuf_write_ex(DPCKSEL, pPrefer->dpclk_sel);

	// 3. CRYS_EN fix to 0:
	iicbuf_write_ex(CRYS_EN, 0);

	// 4. decide clock source:
	if (gcksel_reg) {
		clk_src_khz = pInput->crystal_khz;
	} else {
		clk_src_khz = pInput->pclk_khz;
	}

	// 5. calculate pll1n1:
	for(pll1n1_div = 1; pll1n1_div <= 64; pll1n1_div <<= 1) {
		val_t = clk_src_khz / pll1n1_div;
		if(val_t >= 2300 && val_t <= 4600)
			break;
	}
	if (pll1n1_div > 64) {
		return ERR_PLL1N1_WRONG;
	}
	if (ERR_NO_ERROR != (ret = convert_pll1n1_div(0, &pll1n1_reg, &pll1n1_div))) {
		return ret;
	}
	iicbuf_write_ex(PLL1N1, pll1n1_reg);

	// 6. pll1n2:
	pll1n2_div = pll1n1_div;
	if (ERR_NO_ERROR != (ret = convert_pll1n2_div(0, &pll1n2_reg, &pll1n2_div))) {
		return ret;
	}
	iicbuf_write_ex(PLL1N2, pll1n2_reg);

	// 7. pll1n3:
	pll1n3_div = 32 / pll1n2_div;
	if (ERR_NO_ERROR != (ret = convert_pll1n3_div(0, &pll1n3_reg, &pll1n3_div))) {
		return ret;
	}
	iicbuf_write_ex(PLL1N3, pll1n3_reg);

	// 8. pll3n6:
	for(pll3n6_div = 1; pll3n6_div <= 8; pll3n6_div <<= 1) {
		val_t = pPrefer->mclk_khz * pll3n6_div / 64;
		if (val_t >= 2300 && val_t <= 2600)
			break;
	}
	if (pll3n6_div > 8)
		return ERR_PLL3N6_WRONG;
	if (ERR_NO_ERROR != (ret = convert_pll3n6_div(0, &pll3n6_reg, &pll3n6_div))) {
		return ret;
	}
	iicbuf_write_ex(PLL3N6, pll3n6_reg);

	// 9. A2:
	val_t  = (pll1n1_div * pll3n6_div * pPrefer->mclk_khz);
	a2_reg = os_shift_udiv_roundup(val_t, clk_src_khz, 12);		// wyc
//	a2_reg = ((clk_src_khz << 12) + (val_t -1)) / val_t;		// wyc
//	val_t1 = (clk_src_khz << 12);
//	val_t2 = (pll1n1_div * pll3n6_div * pPrefer->mclk_khz);
//	a2_reg = (val_t1 % val_t2) ? (val_t1 / val_t2 + 1) : (val_t1 / val_t2);
	iicbuf_write_ex(A2, a2_reg);

	// 10. XCH:
	if (pInput->if_type == IF_CPU) {
		xch_reg = 0;
	} else {
		xch_reg = (pInput->pixel_fmt.multiplexed != MULTI_1X) ? 1 : 0;
	}
	iicbuf_write_ex(XCH, xch_reg);

	// 11. DPCKN4:
	if (pInput->pixel_fmt.multiplexed == MULTI_3X) {
		dpckn4_reg = 1;
	} else {
		dpckn4_reg = 0;
	}
	iicbuf_write_ex(DPCKN4, dpckn4_reg);

	// 12. uclk2d:
	if (AUDIO_SPDIF != pInput->audio_type) {  // i2s
		uclk2d_reg = pOutput->uclk_khz / 10;
	} else	{  // spdif
		uclk2d_reg = (clk_src_khz * 2 * pll1n2_div * pll1n3_div) / (pll1n1_div * 10);
	}
	iicbuf_write_ex(PCLK_NUM, uclk2d_reg);

	// 13. uclksec:
	if ((pOutput->channel & CHANNEL_HDMI) &&
			(pOutput->uclk_khz == 54000 ||
			 pOutput->uclk_khz == 72000 ||
			 pOutput->uclk_khz == 74250 ||
			 pOutput->uclk_khz == 108000 ||
			 pOutput->uclk_khz == 148500)) {
		uclksec_reg = 1;
	} else {
		uclksec_reg = 0;
	}
	iicbuf_write_ex(UCLKSEC, uclksec_reg);
	// According to uclksec to calculate:
	if (uclksec_reg == 0) {
// change pll2n5 calculation: pll2n5 = pll2n5 * pll2n11. wyc 2011-11-30	<<<
		pll2n9_div  = pll2n10_div = 8;
		for(pll2n11_div = 1; pll2n11_div <= 8; ++pll2n11_div) {
			// find pll2n5
			for(pll2n5_div = 1; pll2n5_div <= 8; ++pll2n5_div) {
//				val_t = pOutput->uclk_khz * pll2n5_div * pll2n11_div / 64; wyc 2011-11-30
//				if (val_t >= 2300 && val_t <= 4600)
				val_t = pOutput->uclk_khz * pll2n5_div * pll2n11_div;
				if (val_t >= 147200 && val_t <= 294463)
					goto pll2n5_found;
			}
		}
		return ERR_PLL2N5_WRONG;	// not found pll2n5
pll2n5_found:
		dmxtal_div = 1; // compatible
//		val_t  = pOutput->uclk_khz * pll1n1_div * pll2n5_div * pll2n11_div;	// wyc
		val_t  *= pll1n1_div;	// wyc 2011-11-30
		switch(pPrefer->uclkod_sel) {
		case 0:
//			os_shift_udiv(divisor, dividend, shift)
			a1_reg = os_shift_udiv(clk_src_khz, val_t, 20);
//			a1_reg = (uint32)((((uint64)pOutput->uclk_khz) * pll1n1_div * pll2n5_div * pll2n11_div * (1 << 20)) / clk_src_khz);
			iicbuf_write_ex(A1, a1_reg);
			break;
		case 1:
			a3_reg = os_shift_udiv(val_t, clk_src_khz, 12);
//			a3_reg = clk_src_khz * (1 << 12) / (pOutput->uclk_khz * pll1n1_div * pll2n5_div * pll2n11_div);
			iicbuf_write_ex(A3, a3_reg);
			break;
		case 2:
			a1_h  = os_shift_udiv(val_t, clk_src_khz, 12);
//			a1_l  = ((clk_src_khz * (1 << 12))/val_t - 0.5) * (1 << 19);	algorighm, wyc
			a1_l  = os_shift_udiv(val_t, clk_src_khz, 31) - (1 << 18);
//			a1_h = (clk_src_khz * (1 << 12)) / (pOutput->uclk_khz * pll1n1_div * pll2n5_div * pll2n11_div);
//			a1_l = (((uint64)clk_src_khz) * (1 << 12) * 1000000) / (pOutput->uclk_khz * pll1n1_div * pll2n5_div * pll2n11_div) - 500000;
//			a1_l = a1_l * (1 << 19) / 1000000;
			a1_reg = (uint32)(((a1_h & 0xFFF) << 20) | (a1_l & 0xFFFFF));
			iicbuf_write_ex(A1, a1_reg);
		default:
			return ERR_UCLKOD_SEL_NO_SUPPORT;
		}	// switch(pPrefer->uclkod_sel)
// change pll2n5 calculation: pll2n5 = pll2n5 * pll2n11. wyc 2011-11-30	<<<
	} else {  // uclksec == 1
		// set uclkod_sel to 3 - 2010.06.18
		iicbuf_write_ex(UCLKOD_SEL, 3);

		switch(pOutput->uclk_khz) {
		case 54000:
			dmxtal_div  = 6;
			pll2n10_div = 8;
			pll2n9_div  = 6;
			pll2n5_div  = 4;
			pll2n11_div = 1;
			break;
		case 72000:
			dmxtal_div  = 6;
			pll2n10_div = 8;
			pll2n9_div  = 6;
			pll2n5_div  = 3;
			pll2n11_div = 1;
			break;
		case 74250:
			dmxtal_div  = 5;
			pll2n10_div = 11;
			pll2n9_div  = 5;
			pll2n5_div  = 4;
			pll2n11_div = 1;
			break;
		case 108000:
			dmxtal_div  = 6;
			pll2n10_div = 8;
			pll2n9_div  = 6;
			pll2n5_div  = 2;
			pll2n11_div = 1;
			break;
		case 148500:
			dmxtal_div  = 5;
			pll2n10_div = 11;
			pll2n9_div  = 5;
			pll2n5_div  = 2;
			pll2n11_div = 1;
			break;
		default:
			return ERR_UCLKSEC_JUDGE;
		}	// switch(pOutput->uclk_khz)
	}
	if (ERR_NO_ERROR != (ret = convert_pll2n5_div(0, &pll2n5_reg, &pll2n5_div)))
		return ret;
	iicbuf_write_ex(PLL2N5, pll2n5_reg);

	if (ERR_NO_ERROR != (ret = convert_pll2n9_div(0, &pll2n9_reg, &pll2n9_div)))
		return ret;
	iicbuf_write_ex(PLL2_SELH, pll2n9_reg);	// PLL2N9 is PLL2_SELH

	if (ERR_NO_ERROR != (ret = convert_pll2n10_div(0, &pll2n10_reg, &pll2n10_div)))
		return ret;
	iicbuf_write_ex(PLL2_SELL, pll2n10_reg);	// PLL2N10 is PLL2_SELL

	if (ERR_NO_ERROR != (ret = convert_pll2n11_div(0, &pll2n11_reg, &pll2n11_div)))
		return ret;
	iicbuf_write_ex(PLL2N11, pll2n11_reg);

	if (ERR_NO_ERROR != (ret = convert_dmxtal_div(0, &dmxtal_reg, &dmxtal_div)))
		return ret;
	iicbuf_write_ex(DIVXTAL, dmxtal_reg);

	// 14. analog hdmi driver:
	if (pOutput->channel & CHANNEL_HDMI) {
		//decide hdmi clock freq:
		if (pOutput->hdmi_fmt.bypass) {
			if (pPrefer->dpclk_sel == 2) {
				hdmi_clk_khz = pInput->pclk_khz;
			} else {
				hdmi_clk_khz = pInput->crystal_khz;
			}
		} else {
			hdmi_clk_khz = pOutput->uclk_khz;
		}
		// set pll according to hdmi clock:
		if (hdmi_clk_khz < 25000 || hdmi_clk_khz > 165000)
			return ERR_HDMI_CLOCK_NO_SUPPORT;
		if (hdmi_clk_khz <= 40000) {
			dri_pll_n1_1_reg = 0;
			dri_pll_n1_0_reg = 0;
			dri_pll_n3_1_reg = 0;
			dri_pll_n3_0_reg = 0;
		} else if(hdmi_clk_khz < 80000) {
			dri_pll_n1_1_reg = 0;
			dri_pll_n1_0_reg = 1;
			dri_pll_n3_1_reg = 0;
			dri_pll_n3_0_reg = 1;
		} else {
			dri_pll_n1_1_reg = 1;
			dri_pll_n1_0_reg = 0;
			dri_pll_n3_1_reg = 1;
			dri_pll_n3_0_reg = 0;
		}
		iicbuf_write_ex(DRI_PLL_N1_1, dri_pll_n1_1_reg);
		iicbuf_write_ex(DRI_PLL_N1_0, dri_pll_n1_0_reg);
		iicbuf_write_ex(DRI_PLL_N3_1, dri_pll_n3_1_reg);
		iicbuf_write_ex(DRI_PLL_N3_0, dri_pll_n3_0_reg);

		// misc:
		dri_vcomode_reg = 1;
		dri_pll_divsel_reg = 1;
		dri_pll_vcosel_reg = 0;
		dri_modsel_reg = 1;
		iicbuf_write_ex(DRI_VCOMODE, dri_vcomode_reg);
		iicbuf_write_ex(DRI_PLL_DIVSEL, dri_pll_divsel_reg);
		iicbuf_write_ex(DRI_PLL_VCOSEL, dri_pll_vcosel_reg);
		iicbuf_write_ex(DRI_MODSEL, dri_modsel_reg);
	}

	// 15. analog LVDS driver:
	if (pOutput->channel & CHANNEL_LVDS) {
		//decide lvds clock:
		if (pOutput->lvds_fmt.bypass) {
			if (pPrefer->dpclk_sel == 2) {
				lvds_clk_khz = pInput->pclk_khz;
			} else {
				lvds_clk_khz = pInput->crystal_khz;
			}
		} else {
			lvds_clk_khz = pOutput->uclk_khz;
		}
		//set pll according to lvds clock:
		if (lvds_clk_khz < 25000 && lvds_clk_khz > 165000) {
			return ERR_LVDS_CLOCK_NO_SUPPORT;
		}
		if (lvds_clk_khz < 50000) {
			dri_pll_n1_1_reg = 0;
			dri_pll_n1_0_reg = 0;
			dri_pll_n3_1_reg = 0;
			dri_pll_n3_0_reg = 0;
		} else if (lvds_clk_khz < 100000) {
			dri_pll_n1_1_reg = 0;
			dri_pll_n1_0_reg = 1;
			dri_pll_n3_1_reg = 0;
			dri_pll_n3_0_reg = 1;
		} else {
			dri_pll_n1_1_reg = 1;
			dri_pll_n1_0_reg = 0;
			dri_pll_n3_1_reg = 1;
			dri_pll_n3_0_reg = 0;
		}
		iicbuf_write_ex(DRI_PLL_N1_1, dri_pll_n1_1_reg);
		iicbuf_write_ex(DRI_PLL_N1_0, dri_pll_n1_0_reg);
		iicbuf_write_ex(DRI_PLL_N3_1, dri_pll_n3_1_reg);
		iicbuf_write_ex(DRI_PLL_N3_0, dri_pll_n3_0_reg);

		// misc:
		dri_vcomode_reg = 0;
		dri_pll_divsel_reg = 0;
		dri_pll_vcosel_reg = 1;
		dri_modsel_reg = 0;
		iicbuf_write_ex(DRI_VCOMODE, dri_vcomode_reg);
		iicbuf_write_ex(DRI_PLL_DIVSEL, dri_pll_divsel_reg);
		iicbuf_write_ex(DRI_PLL_VCOSEL, dri_pll_vcosel_reg);
		iicbuf_write_ex(DRI_MODSEL, dri_modsel_reg);
	}

	return ERR_NO_ERROR;
}

static int32 cal_and_set_scaler(CH703x_INPUT_INFO* pInput,
	CH703x_OUTPUT_INFO* pOutput, CH703x_PREFER_INFO* pPrefer)
{
	uint8 wrlen_reg;
	uint32 frame_rate_in, frame_rate_out, field_rate_out;
	uint32 hai_down, hai_sdram;
	uint8 fltbp2_reg, fltbp1_reg;
	uint8 dnsmpen_reg;
	uint32 hadwn_reg;
	uint8 blk_h_reg;
	uint32 fba_inc_reg;
	uint8 sfm_reg;
	uint8 maskfm_en_reg;	// wyc 2011-11-30
	uint8 thren_reg;
	uint32 thrrl_reg;
//	uint8 lnsel_reg; -WYC
	uint8 wrfast_reg;
	uint8 chg_hl_reg;
	uint8 vsmst_reg;
	uint8 gap_en_reg;
	uint8 zrcts_reg;
//	uint8 pac8_1_enable_reg;

	uint32 val_t;
	uint32 hao_t;
//	uint32 vao_t;	-WYC
	uint32 vai_t;

	uint32 intlace_status;	// input
	uint32 intlc_status;	// output

	// Pre 1. ZRCTS fix to 0:
	zrcts_reg = 0;
	iicbuf_write_ex(ZRCTS, zrcts_reg);

	// Pre 2. PAC[8_1] fix to 0xE3:
	//pac8_1_enable_reg = 0xE3;
	iicbuf_write_ex(PAC8_1_ENABLE, 0x63);

	// if all bypass mode, nothing to do:
	if (all_bypass_mode) {
		return ERR_NO_ERROR;
	}

	// if no all bypass mode........

	intlace_status = iicbuf_read_ex(INTLACE);
	intlc_status = iicbuf_read_ex(INTLC);

	// 1. WRLEN fixed to 0:
	wrlen_reg = 0;
	iicbuf_write_ex(WRLEN, wrlen_reg);

	// 2. frame rate:
	frame_rate_in  = ((pInput->pclk_khz * 10000) / pInput->timing.ht) * 10 / pInput->timing.vt;
	frame_rate_out = ((pOutput->uclk_khz * 10000) / pOutput->timing.ht) * 10 / pOutput->timing.vt;
	field_rate_out = intlc_status ? (frame_rate_out * 2) : (frame_rate_out);

	// 3. dnsmpen & hadwn:
	hai_down = pInput->timing.ha;
	if (pOutput->ds_percent_h) {
		hao_t = pOutput->timing.ha * (100 - pOutput->ds_percent_h) / 100;
	} else {
		hao_t = pOutput->timing.ha;
	}
	hao_t = hao_t + (hao_t & 1);	//	hao_t = hao_t + (hao_t % 2);
	if (hai_down > hao_t && pOutput->rotate != ROTATE_90 && pOutput->rotate != ROTATE_270) {
		dnsmpen_reg = 1;
		hadwn_reg = hao_t;
		hai_down = hadwn_reg;
		fltbp2_reg = 1;
		fltbp1_reg = 1;
	} else {
		dnsmpen_reg = 0;
		hadwn_reg = 0;
		fltbp2_reg = 1;
		fltbp1_reg = 1;
	}
	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
		val_t = 720;
	} else {
		if (pPrefer->dat16_32b) {
			val_t = 720;
		} else {
			if (intlace_status) { 	// added - 2010.09.09
				val_t = 720;
			} else {
				val_t = pPrefer->true24 ? 720 : 1440;
			}
		}
	}
	if (hai_down > val_t &&
	   (pOutput->rotate != ROTATE_NO || pOutput->h_flip || pOutput->v_flip || intlace_status)) {
		dnsmpen_reg = 1;
		hadwn_reg = val_t;
		hai_down = hadwn_reg;
		fltbp2_reg = 1;
		fltbp1_reg = 1;
	}
	iicbuf_write_ex(DNSMPEN, dnsmpen_reg);
	iicbuf_write_ex(HADWSPP, hadwn_reg);
	// filter adjust:
	if (pInput->timing.ha * 100 / hao_t > pPrefer->hscale_ratio_gate) {
		fltbp1_reg = 0;
	}
	iicbuf_write_ex(FLTBP2, fltbp2_reg);
	iicbuf_write_ex(FLTBP1, fltbp1_reg);

	// 4. blk_h:
	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
		val_t = 100;
	} else {
		if (pPrefer->dat16_32b) {
			val_t = 100;
		} else if(pPrefer->true24) {
			val_t = 100;
		} else {
			val_t = pPrefer->true_com ? 75 : 50;
		}
	}
	if (val_t == 75) {
		hai_sdram = (hai_down / 4) * 3 + (hai_down & 3); // (hai_down % 4);
	} else {
		hai_sdram = hai_down * val_t / 100;
	}
	if(intlace_status) {  // added: if interlaced input, blk_h fixed to 23 - 2010.09.09
		blk_h_reg = 23;
	} else if(pOutput->rotate != ROTATE_NO || pOutput->h_flip || pOutput->v_flip) {
		blk_h_reg = 45;
	} else if(hai_sdram <= 720 && pInput->timing.va <= 720) {
		blk_h_reg = 45;
	} else {
		blk_h_reg = hai_sdram / 16;
//		blk_h_reg = (hai_sdram % 16) ? (blk_h_reg + 1) : blk_h_reg;
		blk_h_reg = (hai_sdram & 15) ? (blk_h_reg + 1) : blk_h_reg;
	}
	iicbuf_write_ex(BLK_H, blk_h_reg);

	// 5. FBA_INC, SFM, THREN, THRRL
	if ( (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) &&
	     pPrefer->true24 == 0 &&
	     pPrefer->true_com == 0 &&
	     pPrefer->dat16_32b == 0) {
		vai_t = pInput->timing.va / 2;
	} else {
		vai_t = pInput->timing.va;
	}
	val_t = (vai_t + 15) >> 4;
	fba_inc_reg = blk_h_reg * val_t;
// change on 2011-11-30, wyc >>>
	if (fba_inc_reg > FBA_INC_GATE) {
		sfm_reg = 1;
		thren_reg = 1;
//		val_t = (frame_rate_in * 100) / field_rate_out;
//		if (val_t > 100) {
//			thrrl_reg = pInput->timing.vt * (10000 - 10000 / val_t) / 10000 + THRRL_ADJUST_DEF;
//		} else {
//			thrrl_reg = pInput->timing.vt * (100 - val_t) / 100 + THRRL_ADJUST_DEF;
//		} 
	} else {
		sfm_reg = 0;
		thren_reg = 0;
		thrrl_reg = 0;
	}

	// 7. wrfast:
	if (frame_rate_in >= field_rate_out) {
		wrfast_reg = 1;
		thren_reg  = 0;
		maskfm_en_reg = 1;
//		if (((720 == pInput->timing.va) && (1080 == pOutput->timing.va)) ||
//			((frame_rate_in - field_rate_out) < 2))
			fba_inc_reg = 0;
	} else {
		wrfast_reg = 0;
		thren_reg  = 1;
		maskfm_en_reg = 0;
//      thrrl_reg = (( (4096.0 / fba_inc_reg )-1 ) * pInput->timing.va )-1;
		thrrl_reg = (pInput->timing.va * 4096) / fba_inc_reg - pInput->timing.va - 1;
		iicbuf_write_ex(THRRL, thrrl_reg);
	}
	iicbuf_write_ex(WRFAST, wrfast_reg);
	iicbuf_write_ex(FBA_INC, fba_inc_reg);
	iicbuf_write_ex(SFM, sfm_reg);
	iicbuf_write_ex(THREN, thren_reg);
	iicbuf_write_ex(MASKFM_EN, maskfm_en_reg);
// change on 2011-11-30, wyc <<<

	// 6. LNSEL:
// original code WYC_TEST
//	if (pOutput->ds_percent_v) {
//		vao_t = pOutput->timing.va * (100 - pOutput->ds_percent_v) / 100;
//	} else {
//		vao_t = pOutput->timing.va;
//	}
//	vao_t = vao_t + (vao_t & 1);	// (vao_t % 2);
//	val_t = intlc_status ? (vao_t / 2) : vao_t;
//	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270) {
//		if (hai_down <= val_t)
//			lnsel_reg = 2; // remove flicker - 2010.05.25
//		else
//			lnsel_reg = 1;
//	} else if(pOutput->rotate == ROTATE_180 || pOutput->h_flip || pOutput->v_flip) {
//		if (pInput->timing.va <= val_t)
//			lnsel_reg = 2;
//		else
//			lnsel_reg = 1;
//	} else {
//		if (pInput->timing.va <= val_t)
//			lnsel_reg = 3;
//		else
//			lnsel_reg = 1;
//	}
//	if (scale_line_adjust) {
//		lnsel_reg++;
//	}
//	iicbuf_write_ex(LNSEL, lnsel_reg);

	iicbuf_write_ex(LNSEL, lnsel_saved);	// WYC_TEST

	//8. CHG_HL:
	chg_hl_reg = ((pPrefer->dat16_32b == 0) && (!pPrefer->true24) && (!pPrefer->true_com) &&
				   (((pOutput->rotate == ROTATE_NO)  && ( pOutput->h_flip)) ||
					((pOutput->rotate == ROTATE_90)  && (!pOutput->h_flip)) ||
					((pOutput->rotate == ROTATE_270) && ( pOutput->h_flip)) ||
					((pOutput->rotate == ROTATE_180) && (!pOutput->h_flip)))) ? 1 : 0;
	iicbuf_write_ex(CHG_HL, chg_hl_reg);

	//9. vsmst fix to 2:
	vsmst_reg = 2;
	iicbuf_write_ex(VSMST, vsmst_reg);

	//10. add GAP_EN adjust - 2010.06.23
	if (wrfast_reg == 1 && (pOutput->rotate != ROTATE_NO || pOutput->v_flip)) {
		gap_en_reg = 1;
	} else {
		gap_en_reg = 0;
	}
	iicbuf_write_ex(GAP_EN, gap_en_reg);

	return ERR_NO_ERROR;
}

#define	SDRAM_CLK_FREQ			166000	// 166MHz
#define HRES_THD_TRUE24			800//1024
#define VRES_THD_TRUE24			600//768
#define HRES_THD_TRUE_COM		1280
#define VRES_THD_TRUE_COM		800

static void ToPreferInfo(CH703x_INPUT_INFO* pInput,
	CH703x_OUTPUT_INFO* pOutput, CH703x_PREFER_INFO* pPrefer)
{
	pPrefer->mclk_khz   = SDRAM_CLK_FREQ;
	pPrefer->uclkod_sel = 0;
//	pPrefer->uclkod_sel = 1;		// work 1080p, fail 1440x900
//	pPrefer->uclkod_sel = 2;		// work 1080p, fail 1440x900
	pPrefer->dpclk_sel  = 2; 		// From external
	pPrefer->dat16_32b  = 0;		// dat16_32b = 0, 0--32 bits
//  true24 case:
//  input:  800x480   65fps 33250KHz clock
//  output: 1024x768  60fps rotate 90, 180, 270 OK
//  output: 1280x800  60fps rotate 90, 180, 270 OK
//  output: 1440x900  60fps rotate 90, 180, 270 OK after adjust LNSEL
	if (pInput->timing.ha > HRES_THD_TRUE24 || pInput->timing.va > VRES_THD_TRUE24)
		pPrefer->true24 = 0;
	else
		pPrefer->true24 = 1;
	if (pOutput->rotate == ROTATE_90 || pOutput->rotate == ROTATE_270 ||
		pInput->timing.ha > HRES_THD_TRUE_COM || pInput->timing.va > VRES_THD_TRUE_COM ||
		pPrefer->true24)
		pPrefer->true_com	= 0;
	else
		pPrefer->true_com	= 1;
	pPrefer->dither_filter_enable = 1; 		// 1;
	pPrefer->hscale_ratio_gate	  = 130;	// 1.3
}

//----------------- local functions of interface ------------------------

extern int edid_valid(uint8 *edid);

//static int edid_valid(uint8 *edid)
//{
//	return ((edid[0]==0x00) && (edid[1]==0xff) &&
//			(edid[2]==0xff) && (edid[3]==0xff) &&
//			(edid[4]==0xff) && (edid[5]==0xff) &&
//			(edid[6]==0xff) && (edid[7]==0x00));
//}

#if 1//SHOW_EDID_DATA // MTK-Modify
// Dump out an EDID block in a simple format
static void show_edid_block(uint8* data, int block)
{
	int  i, j;

	i = 0;
	while (i < 128) {
		printk(" 0x%04X:", i + block * 128); // MTK-Modify
		for(j = 16; j != 0; j--) {
			printk(" %02x", data[i++]); // MTK-Modify
    	}
    	printk("\n"); // MTK-Modify
	}
   	printk("\n"); // MTK-Modify
}

#else

#define show_edid_block(data, block)	do { } while(0)

#endif

//==================== interface ==============================

CH703x_DEV_TYPE  ch_check_chip(CH703x_DEV_TYPE i)
{
	int		m;
	// detect which device present
	if (i >= DEV_UNKNOW) {
		for(m = 0; m < CH703x_I2CADDR_NUM; m++) {
			i = hw_detectChip(ch703x_i2caddr[m], 0, DEV_UNKNOW-1);
			if (i < DEV_UNKNOW) break;
		}
		return i;
	}

	// detect whether the specified device present
	return hw_detectChip(ch703x_devinfo[i].i2cAddr, i, i);
}

int32   ch_getMcuVersion(CH703x_DEV_TYPE chip, FW703X_CFG *cfg)
{
	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	if (NULL == cfg)
		return ERR_PARAMETER;

	return fw_getMcuVersion(ch703x_devinfo[chip].i2cAddr, cfg);
}

int32   ch_get_edid(CH703x_DEV_TYPE chip, uint8 *edid, uint16 len, uint8 whichEDID)
{
	uint8 	addr, blk;
	int32	ret;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	if (NULL == edid)
		return ERR_PARAMETER;
	if (len < 128)
		return ERR_PARAMETER;
	addr = ch703x_devinfo[chip].i2cAddr;

	// bit7 = 1 for VGA DDC, 0 for HDMI DDC
//	if( whichEDID & GET_VGA_EDID ) {
// 		make sure turn on PROM power
//	} else {
// 		make sure turn on DDC power and HPD power
//	}

	whichEDID &= GET_EDID_MASK;		// only keep bit 7
	printk("read EDID 0x%02X...\n", whichEDID); // MTK-Modify
	ret = fw_getEdidBlock(addr, &edid[0], whichEDID); // first EDID block
	if (ERR_NO_ERROR != ret)
		return ret;
	show_edid_block(&edid[0], 0);

	if ( !edid_valid(edid) ) {
		printk("invalid EDID\n"); // MTK-Modify
		return ERR_INVALID_EDID;
	}

	// extension EDID
	blk = (uint8)(len >> 7);	// len/128 = maximum EDID blocks
	if (edid[126] >= blk) {
		printk("Too many %d EDID extensions, only read %d blocks\n",
			edid[126], blk-1); // MTK-Modify
		edid[126] = blk-1;		// ignore the later blocks
	}
	for(blk = 1; blk <= edid[126]; blk++) {
		ret = fw_getEdidBlock(addr, &edid[blk*128], (blk << 4) | whichEDID);
		if (ERR_NO_ERROR != ret)
			break;
		show_edid_block(&edid[blk*128], blk);
	}

	return ret;
}

int32   ch_set_mode(CH703x_DEV_TYPE chip, CH703x_INPUT_INFO *pInput,
			CH703x_OUTPUT_INFO *pOutput)
{
	CH703x_PREFER_INFO	preferInfo;
	int32				ret;
	uint8				addr;

	// check parameter
	if (NULL == pInput)
		return ERR_INPUT_INFO;
	if (NULL == pOutput)
		return ERR_OUTPUT_INFO;
	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;

	// generate prefer info
	ToPreferInfo(pInput, pOutput, &preferInfo);
       //printk("[hdmi]%s,#%d\n", __func__, __LINE__);
	// check whether support the specified output channel, +wyc
	if (ERR_NO_ERROR != (ret = hw_checkOutpuChannel(chip, pOutput->channel)))
		return ret;
      //printk("[hdmi]%s,#%d\n", __func__, __LINE__);
	while(1) {
		ret = cal_prepare(pInput, pOutput, &preferInfo);
		if (ERR_NO_ERROR == ret) {
			break;	// success
		// If failed by true color enable, disable it...
		} else if (ERR_NO_SUPPORT_TRUE24 == ret) {
//			MSG_OUT("ERR_NO_SUPPORT_TRUE24, disable it!\n");
			preferInfo.true24   = 0;
			preferInfo.true_com = 1; // use true_com replaced of true24
		} else if (ERR_NO_SUPPORT_TRUECOM == ret) {
//			MSG_OUT("ERR_NO_SUPPORT_TRUECOM, disable it!\n");
			preferInfo.true24   = 0;
			preferInfo.true_com = 0;
		} else {
//			ERR_OUT("cal_prepare fail: %d\n", ret);
			return ret;
		}
	}

	if (ERR_NO_ERROR != (ret = set_input_info(pInput)))
		return ret;

	if (ERR_NO_ERROR != (ret = set_output_info(pOutput)))
		return ret;

	if (ERR_NO_ERROR != (ret = set_prefer_info(&preferInfo)))
		return ret;

	if (ERR_NO_ERROR != (ret = cal_and_set_clk_pll(pInput, pOutput, &preferInfo)))
		return ret;

	if (ERR_NO_ERROR != (ret = cal_and_set_scaler(pInput, pOutput, &preferInfo)))
		return ret;


	// hardware operation
	addr = ch703x_devinfo[chip].i2cAddr;
    hw_ResetIB(addr);
	hw_SetupReg(addr);
    hw_ReleaseMCU(addr);
	//printk("[hdmi]%s,#%d\n", __func__, __LINE__);
	ret = fw_setChanPower(addr, pOutput->channel);
      //printk("[hdmi]%s,#%d\n", __func__, __LINE__);
	return ret;
}

int32   ch_load_firmware(CH703x_DEV_TYPE chip, uint8 *fw, uint16 size)
{
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	if (NULL == fw)
		return ERR_PARAMETER;
	if ((size > 8192) || (size == 0))
		return ERR_TOOLONG_FIRMWARE;

	addr = ch703x_devinfo[chip].i2cAddr;
	return hw_loadFirmware(ch703x_devinfo[chip].i2cAddr, fw, size);
}

int32    ch_set_test_mode(CH703x_DEV_TYPE chip)
{
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;

	addr = ch703x_devinfo[chip].i2cAddr;
	fw_acquireI2C(addr);

	// 0_0x7F, bit 5 TSYNC=1, bit 4 TEST=1, bit 3..0 TSTP=4
	hw_setBits(addr, 0, 0x7F, 0xC0, 0x34);

	fw_releaseI2C(addr);
	return ERR_NO_ERROR;
}

int32   ch_set_dither(CH703x_DEV_TYPE chip, uint8 dither)
{
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;

	addr = ch703x_devinfo[chip].i2cAddr;
	fw_acquireI2C(addr);

	// 0_0x19, bit 6 DBP0, ~dither
	hw_setBits(addr, 0, 0x19, ~(1 << 6), ((~dither) & 1) << 6);

	fw_releaseI2C(addr);
	return ERR_NO_ERROR;
}

int32   ch_set_channel_power(CH703x_DEV_TYPE chip, uint8 chan)
{
	uint8		addr, chanBits;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	addr     = ch703x_devinfo[chip].i2cAddr;
	chanBits = ch703x_devinfo[chip].chanBits;

	chan &= chanBits;
	if (chanBits & CHANNEL_LVDS) {
		printk("Power %s lvds\n", (chan & CHANNEL_LVDS) ? "ON " : "OFF"); // MTK-Modify
	}
	if (chanBits & CHANNEL_HDMI) {
		printk("Power %s hdmi\n", (chan & CHANNEL_HDMI) ? "ON " : "OFF"); // MTK-Modify
	}
	if (chanBits & CHANNEL_VGA) {
		printk("Power %s vga\n",  (chan & CHANNEL_VGA)  ? "ON " : "OFF"); // MTK-Modify
	}
	if (chanBits & CHANNEL_HDTV) {
		printk("Power %s hdtv\n", (chan & CHANNEL_HDTV) ? "ON " : "OFF"); // MTK-Modify
	}
	if (chanBits & CHANNEL_VGA1) {
		printk("Power %s vga1\n", (chan & CHANNEL_VGA1) ? "ON " : "OFF"); // MTK-Modify
	}

	return fw_setChanPower(addr, chan);
}

int32   ch_set_chip_power(CH703x_DEV_TYPE chip, uint8 chan)
{
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	addr = ch703x_devinfo[chip].i2cAddr;

	if (chan == 0x00) {	// if power down, means currently power on ?
		fw_acquireI2C(addr);
	}

	printk("Full power %s chip\n", chan ? "ON " : "OFF"); // MTK-Modify

	// 0_0x04, bit 5
	hw_setBits(addr, 0, 0x04, ~(0x20), chan ? 0x00 : 0x20);

	if (chan) {
		fw_releaseI2C(addr);	// if currently power on?
		msleep(200);		// wait power stable // MTK-Modify
	}
	return ERR_NO_ERROR;
}

int32   ch_set_plug_out_power(CH703x_DEV_TYPE chip, uint8 enable)
{
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	addr = ch703x_devinfo[chip].i2cAddr;

	// 0_0x09, bit 1
	hw_setBits(addr, 0, 0x09, ~(0x01), enable? 0x00: 0x01);

	return ERR_NO_ERROR;
}

int32   ch_start_firmware(CH703x_DEV_TYPE chip)
{
	int32			ret;
	FW703X_CFG 		cfg;
	uint8			addr;

	if (chip >= DEV_UNKNOW)
		return ERR_DEVICE_NO_EXIST;
	addr = ch703x_devinfo[chip].i2cAddr;

      hw_set_HPD_MAX(addr);
      
	// check whether firmware is running
	ret = fw_getMcuVersion(addr, &cfg);
	if (ERR_NO_ERROR != ret)
		return ret;
	printk("MCU V%d.%d DID %X RID %d Cap %X\n",
		cfg.ver_major, cfg.ver_minor, cfg.did, cfg.rid, cfg.capbility); // MTK-Modify
	cfg.did &= CH703x_DIDMASK;
	if ((ch703x_devinfo[chip].devID == cfg.did) && (0x01 & cfg.capbility)) {
		printk("Firmware is running\n"); // MTK-Modify
		return ERR_NO_ERROR;
	}

	printk("Firmware is not running\n"); // MTK-Modify
	return ERR_FIRMWARE_NOT_RUNNING;
}

uint8   ch_check_connection(CH703x_DEV_TYPE chip, int delay)
{
	uint8	addr;

	if (chip >= DEV_UNKNOW)
		return 0;

	addr = ch703x_devinfo[chip].i2cAddr;

	return fw_getConnection(addr, delay);
}

uint8	ch_enable_MCU_AutoFunc(CH703x_DEV_TYPE chip, uint8 autoBits)
{
	uint8	addr;
	if (chip >= DEV_UNKNOW)
		return 0;
	addr = ch703x_devinfo[chip].i2cAddr;

	if (autoBits)	// wait for a while if enable
		msleep(ENABLE_AUTOFUNC_DELAY); //MTK-Modify

	return hw_enableMcuAutoFunc(addr, autoBits);
}

// Enter:
//		pwm_duty:	same as pwm_duty in LVDS_FMT, 0~255
// Return:  old setting
uint8	ch_set_lvds_brightness(CH703x_DEV_TYPE chip, uint8 pwm_duty)
{
	uint8	addr;
	if (chip >= DEV_UNKNOW)
		return 0;
	addr = ch703x_devinfo[chip].i2cAddr;

	return hw_setPWMDuty(addr, pwm_duty);
}

// -------- lvds_bypass_mode, add on 2011-11-11 ----------
#if 0
static uint8 hw_getBits_ByRegID(uint8 addr, MULTI_REG_ID rID)
{
	uint8 bits, old;

	bits = 	(1 << (g_MultiRegTable[rID].low.endBit + 1)) -
			(1 <<  g_MultiRegTable[rID].low.startBit);

	i2c_write_page(addr, g_MultiRegTable[rID].page);
	old = i2c_read_byte(addr, g_MultiRegTable[rID].low.reg);

	return (uint8)((old & bits) >> g_MultiRegTable[rID].low.startBit);
}

static uint8 hw_setBits_ByRegID(uint8 addr, MULTI_REG_ID rID, uint8 val)
{
	uint8 bits, old;

	bits = 	(1 << (g_MultiRegTable[rID].low.endBit + 1)) -
			(1 <<  g_MultiRegTable[rID].low.startBit);
	val = (val <<  g_MultiRegTable[rID].low.startBit) & bits;
	
	old = hw_setBits(addr, g_MultiRegTable[rID].page,
		g_MultiRegTable[rID].low.reg, ~bits, val);

	return (uint8)((old & bits) >> g_MultiRegTable[rID].low.startBit);
}

// Enter:
//		current:  bit 7,    0 - enable, 1 - disable
//				  bi7 2..0, 0 ~ 7 set current to
//			0 - 2.45mA,  1 - 2.8mA,  2 - 3.15mA,  3 - 3.5mA(default)
//			4 - 3.85mA,  5 - 4.2mA,  6 - 4.55mA,  7 - 4.9mA
// Note:
//		only set this when the output is lvds bypass mode
//		some settings may cause panel fail to power on 
// Return:  old setting

uint8	ch_set_lvds_current(CH703x_DEV_TYPE chip, uint8 current)
{
	uint8	addr, old;
	if (chip >= DEV_UNKNOW)
		return 0;
	addr = ch703x_devinfo[chip].i2cAddr;

	old = hw_setBits_ByRegID(addr, DRI_DBEN, current >> 7) << 7;

	old |= (current & 0x80) ? hw_getBits_ByRegID(addr, DRI_DRAMP) :
			hw_setBits_ByRegID(addr, DRI_DRAMP, current);

	return old;
}

// Enter:
//		bOn:	0 - Off; 1 - On
// Return:  old setting
uint8	ch_set_lvds_common_voltage(CH703x_DEV_TYPE chip, uint8 bOn)
{
	uint8	addr;
	if (chip >= DEV_UNKNOW)
		return 0;
	addr = ch703x_devinfo[chip].i2cAddr;

	return hw_setBits_ByRegID(addr, DRI_CMFB_EN, bOn);
}
#endif

//MTK-ADD_START
void ch_dump_reg(void)
{
	uint8 page, index;

	for(page = 0; page < REG_PAGE_NUM; ++page) {
	      if(page != 2)
	      {
		  hw_WritePage(CH7035_I2CADDR, page);
		  for(index = 0x07; index < REG_NUM_PER_PAGE; ++index) {
			printk("page=0x%x,0x%02x=0x%02x\n",page,index,hw_ReadByte(CH7035_I2CADDR, index));
		  }	
		}
	}
}

void ch_read_reg(unsigned char u8Reg)
{
    printk("ch7035_read,0x%02x=0x%02x\n",u8Reg,hw_ReadByte(CH7035_I2CADDR, u8Reg));
}

void ch_write_reg(unsigned char u8Reg, unsigned char u8Data)
{
    hw_WriteByte(CH7035_I2CADDR, u8Reg, u8Data);
}

void hw_set_HPD_MAX(uint8 addr)
{
    i2c_write_page(addr, 1);
    i2c_write_byte(addr, 0x1E, (uint8)(HPD_DELAY_MAX>>3));
}

void hw_set_inv_clk(uint8 inv)
{
    uint8 tmp;
    i2c_write_page(CH7035_I2CADDR, 1);
    tmp = i2c_read_byte(CH7035_I2CADDR, 0x07);

    tmp = (inv > 0) ? (tmp | (0x01 << 2)): (tmp & ~(0x01 << 2));

    i2c_write_byte(CH7035_I2CADDR, 0x07, tmp);
}

void hw_set_suspend(void)
{
    uint8 tmp;
    i2c_write_page(1, 0);
    tmp = i2c_read_byte(1, 0x7);
    i2c_write_byte(1, 0x7, tmp|(0x1<<5));

    i2c_write_page(1, 4);
    tmp = i2c_read_byte(1, 0x52);
    i2c_write_byte(1, 0x52, tmp&~(0x1<<2));    
}

void hw_set_resume(void)
{
    uint8 tmp;
    i2c_write_page(1, 0);
    tmp = i2c_read_byte(1, 0x7);
    i2c_write_byte(1, 0x7, tmp&~(0x1<<5));

    i2c_write_page(1, 4);
    tmp = i2c_read_byte(1, 0x52);
    i2c_write_byte(1, 0x52, tmp|(0x1<<2));    
}

//MTK-ADD_END
