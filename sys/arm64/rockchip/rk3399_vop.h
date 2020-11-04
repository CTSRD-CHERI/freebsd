#ifndef _ARM64_ROCKCHIP_RK3399_VOP_
#define	_ARM64_ROCKCHIP_RK3399_VOP_

#define	RK3399_REG_CFG_DONE			0x0000
#define	RK3399_VERSION_INFO			0x0004
#define	RK3399_SYS_CTRL				0x0008
#define	 SYS_CTRL_STANDBY_EN			(1 << 22)
#define	 SYS_CTRL_MIPI_OUT_EN			(1 << 15)
#define	 SYS_CTRL_EDP_OUT_EN			(1 << 14)
#define	 SYS_CTRL_HDMI_OUT_EN			(1 << 13)
#define	 SYS_CTRL_RGB_OUT_EN			(1 << 12)
#define	 SYS_CTRL_ALL_OUT_EN			(SYS_CTRL_MIPI_OUT_EN |\
						 SYS_CTRL_EDP_OUT_EN |\
						 SYS_CTRL_HDMI_OUT_EN |\
						 SYS_CTRL_RGB_OUT_EN)
#define	RK3399_SYS_CTRL1			0x000c
#define	RK3399_DSP_CTRL0			0x0010
#define	 DSP_CTRL0_OUT_MODE_S			0
#define	 DSP_CTRL0_OUT_MODE_M			(0xf << DSP_CTRL0_OUT_MODE_S)
#define	RK3399_DSP_CTRL1			0x0014
#define	 DSP_CTRL1_MIPI_POL_S			28
#define	 DSP_CTRL1_MIPI_POL_M			(0xf << DSP_CTRL1_MIPI_POL_S)
#define	RK3399_DSP_BG				0x0018
#define	RK3399_MCU_CTRL				0x001c
#define	RK3399_WB_CTRL0				0x0020
#define	RK3399_WB_CTRL1				0x0024
#define	RK3399_WB_YRGB_MST			0x0028
#define	RK3399_WB_CBR_MST			0x002c
#define	RK3399_WIN0_CTRL0			0x0030
#define	 WIN0_CTRL0_LB_MODE_S	5
#define	 WIN0_CTRL0_LB_MODE_M	(0x7 << WIN0_CTRL0_LB_MODE_S)
#define	 WIN0_CTRL0_DATA_FMT_S	1
#define	 WIN0_CTRL0_DATA_FMT_M	(0x7 << WIN0_CTRL0_DATA_FMT_S)
#define	 WIN0_CTRL0_EN		(1 << 0)

#define	RK3399_WIN0_CTRL1			0x0034
#define	RK3399_WIN0_COLOR_KEY			0x0038
#define	RK3399_WIN0_VIR				0x003c
#define	 WIN0_VIR_WIDTH_RGB888(x) (((((x * 3) >> 2)+((x) % 3)) & 0x3fff) << 0)
#define	RK3399_WIN0_YRGB_MST			0x0040
#define	RK3399_WIN0_CBR_MST			0x0044
#define	RK3399_WIN0_ACT_INFO			0x0048
#define	RK3399_WIN0_DSP_INFO			0x004c
#define	RK3399_WIN0_DSP_ST			0x0050
#define	RK3399_WIN0_SCL_FACTOR_YRGB		0x0054
#define	RK3399_WIN0_SCL_FACTOR_CBR		0x0058
#define	RK3399_WIN0_SCL_OFFSET			0x005c
#define	RK3399_WIN0_SRC_ALPHA_CTRL		0x0060
#define	RK3399_WIN0_DST_ALPHA_CTRL		0x0064
#define	RK3399_WIN0_FADING_CTRL			0x0068
#define	RK3399_WIN0_CTRL2			0x006c
#define	RK3399_WIN1_CTRL0			0x0070
#define	RK3399_WIN1_CTRL1			0x0074
#define	RK3399_WIN1_COLOR_KEY			0x0078
#define	RK3399_WIN1_VIR				0x007c
#define	RK3399_WIN1_YRGB_MST			0x0080
#define	RK3399_WIN1_CBR_MST			0x0084
#define	RK3399_WIN1_ACT_INFO			0x0088
#define	RK3399_WIN1_DSP_INFO			0x008c
#define	RK3399_WIN1_DSP_ST			0x0090
#define	RK3399_WIN1_SCL_FACTOR_YRGB		0x0094
#define	RK3399_WIN1_SCL_FACTOR_CBR		0x0098
#define	RK3399_WIN1_SCL_OFFSET			0x009c
#define	RK3399_WIN1_SRC_ALPHA_CTRL		0x00a0
#define	RK3399_WIN1_DST_ALPHA_CTRL		0x00a4
#define	RK3399_WIN1_FADING_CTRL			0x00a8
#define	RK3399_WIN1_CTRL2			0x00ac
#define	RK3399_WIN2_CTRL0			0x00b0
#define	RK3399_WIN2_CTRL1			0x00b4
#define	RK3399_WIN2_VIR0_1			0x00b8
#define	RK3399_WIN2_VIR2_3			0x00bc
#define	RK3399_WIN2_MST0			0x00c0
#define	RK3399_WIN2_DSP_INFO0			0x00c4
#define	RK3399_WIN2_DSP_ST0			0x00c8
#define	RK3399_WIN2_COLOR_KEY			0x00cc
#define	RK3399_WIN2_MST1			0x00d0
#define	RK3399_WIN2_DSP_INFO1			0x00d4
#define	RK3399_WIN2_DSP_ST1			0x00d8
#define	RK3399_WIN2_SRC_ALPHA_CTRL		0x00dc
#define	RK3399_WIN2_MST2			0x00e0
#define	RK3399_WIN2_DSP_INFO2			0x00e4
#define	RK3399_WIN2_DSP_ST2			0x00e8
#define	RK3399_WIN2_DST_ALPHA_CTRL		0x00ec
#define	RK3399_WIN2_MST3			0x00f0
#define	RK3399_WIN2_DSP_INFO3			0x00f4
#define	RK3399_WIN2_DSP_ST3			0x00f8
#define	RK3399_WIN2_FADING_CTRL			0x00fc
#define	RK3399_WIN3_CTRL0			0x0100
#define	RK3399_WIN3_CTRL1			0x0104
#define	RK3399_WIN3_VIR0_1			0x0108
#define	RK3399_WIN3_VIR2_3			0x010c
#define	RK3399_WIN3_MST0			0x0110
#define	RK3399_WIN3_DSP_INFO0			0x0114
#define	RK3399_WIN3_DSP_ST0			0x0118
#define	RK3399_WIN3_COLOR_KEY			0x011c
#define	RK3399_WIN3_MST1			0x0120
#define	RK3399_WIN3_DSP_INFO1			0x0124
#define	RK3399_WIN3_DSP_ST1			0x0128
#define	RK3399_WIN3_SRC_ALPHA_CTRL		0x012c
#define	RK3399_WIN3_MST2			0x0130
#define	RK3399_WIN3_DSP_INFO2			0x0134
#define	RK3399_WIN3_DSP_ST2			0x0138
#define	RK3399_WIN3_DST_ALPHA_CTRL		0x013c
#define	RK3399_WIN3_MST3			0x0140
#define	RK3399_WIN3_DSP_INFO3			0x0144
#define	RK3399_WIN3_DSP_ST3			0x0148
#define	RK3399_WIN3_FADING_CTRL			0x014c
#define	RK3399_HWC_CTRL0			0x0150
#define	RK3399_HWC_CTRL1			0x0154
#define	RK3399_HWC_MST				0x0158
#define	RK3399_HWC_DSP_ST			0x015c
#define	RK3399_HWC_SRC_ALPHA_CTRL		0x0160
#define	RK3399_HWC_DST_ALPHA_CTRL		0x0164
#define	RK3399_HWC_FADING_CTRL			0x0168
#define	RK3399_HWC_RESERVED1			0x016c
#define	RK3399_POST_DSP_HACT_INFO		0x0170
#define	RK3399_POST_DSP_VACT_INFO		0x0174
#define	RK3399_POST_SCL_FACTOR_YRGB		0x0178
#define	RK3399_POST_RESERVED			0x017c
#define	RK3399_POST_SCL_CTRL			0x0180
#define	RK3399_POST_DSP_VACT_INFO_F1		0x0184
#define	RK3399_DSP_HTOTAL_HS_END		0x0188
#define	RK3399_DSP_HACT_ST_END			0x018c
#define	RK3399_DSP_VTOTAL_VS_END		0x0190
#define	RK3399_DSP_VACT_ST_END			0x0194
#define	RK3399_DSP_VS_ST_END_F1			0x0198
#define	RK3399_DSP_VACT_ST_END_F1		0x019c
#define	RK3399_PWM_CTRL				0x01a0
#define	RK3399_PWM_PERIOD_HPR			0x01a4
#define	RK3399_PWM_DUTY_LPR			0x01a8
#define	RK3399_PWM_CNT				0x01ac
#define	RK3399_BCSH_COLOR_BAR			0x01b0
#define	RK3399_BCSH_BCS				0x01b4
#define	RK3399_BCSH_H				0x01b8
#define	RK3399_BCSH_CTRL			0x01bc
#define	RK3399_CABC_CTRL0			0x01c0
#define	RK3399_CABC_CTRL1			0x01c4
#define	RK3399_CABC_CTRL2			0x01c8
#define	RK3399_CABC_CTRL3			0x01cc
#define	RK3399_CABC_GAUSS_LINE0_0		0x01d0
#define	RK3399_CABC_GAUSS_LINE0_1		0x01d4
#define	RK3399_CABC_GAUSS_LINE1_0		0x01d8
#define	RK3399_CABC_GAUSS_LINE1_1		0x01dc
#define	RK3399_CABC_GAUSS_LINE2_0		0x01e0
#define	RK3399_CABC_GAUSS_LINE2_1		0x01e4
#define	RK3399_FRC_LOWER01_0			0x01e8
#define	RK3399_FRC_LOWER01_1			0x01ec
#define	RK3399_FRC_LOWER10_0			0x01f0
#define	RK3399_FRC_LOWER10_1			0x01f4
#define	RK3399_FRC_LOWER11_0			0x01f8
#define	RK3399_FRC_LOWER11_1			0x01fc
#define	RK3399_AFBCD0_CTRL			0x0200
#define	RK3399_AFBCD0_HDR_PTR			0x0204
#define	RK3399_AFBCD0_PIC_SIZE			0x0208
#define	RK3399_AFBCD0_STATUS			0x020c
#define	RK3399_AFBCD1_CTRL			0x0220
#define	RK3399_AFBCD1_HDR_PTR			0x0224
#define	RK3399_AFBCD1_PIC_SIZE			0x0228
#define	RK3399_AFBCD1_STATUS			0x022c
#define	RK3399_AFBCD2_CTRL			0x0240
#define	RK3399_AFBCD2_HDR_PTR			0x0244
#define	RK3399_AFBCD2_PIC_SIZE			0x0248
#define	RK3399_AFBCD2_STATUS			0x024c
#define	RK3399_AFBCD3_CTRL			0x0260
#define	RK3399_AFBCD3_HDR_PTR			0x0264
#define	RK3399_AFBCD3_PIC_SIZE			0x0268
#define	RK3399_AFBCD3_STATUS			0x026c
#define	RK3399_INTR_EN0				0x0280
#define	RK3399_INTR_CLEAR0			0x0284
#define	RK3399_INTR_STATUS0			0x0288
#define	RK3399_INTR_RAW_STATUS0			0x028c
#define	RK3399_INTR_EN1				0x0290
#define	RK3399_INTR_CLEAR1			0x0294
#define	RK3399_INTR_STATUS1			0x0298
#define	RK3399_INTR_RAW_STATUS1			0x029c
#define	RK3399_LINE_FLAG			0x02a0
#define	RK3399_VOP_STATUS			0x02a4
#define	RK3399_BLANKING_VALUE			0x02a8
#define	RK3399_MCU_BYPASS_PORT			0x02ac
#define	RK3399_WIN0_DSP_BG			0x02b0
#define	RK3399_WIN1_DSP_BG			0x02b4
#define	RK3399_WIN2_DSP_BG			0x02b8
#define	RK3399_WIN3_DSP_BG			0x02bc
#define	RK3399_YUV2YUV_WIN			0x02c0
#define	RK3399_YUV2YUV_POST			0x02c4
#define	RK3399_AUTO_GATING_EN			0x02cc
#define	RK3399_WIN0_CSC_COE			0x03a0
#define	RK3399_WIN1_CSC_COE			0x03c0
#define	RK3399_WIN2_CSC_COE			0x03e0
#define	RK3399_WIN3_CSC_COE			0x0400
#define	RK3399_HWC_CSC_COE			0x0420
#define	RK3399_BCSH_R2Y_CSC_COE			0x0440
#define	RK3399_BCSH_Y2R_CSC_COE			0x0460
#define	RK3399_POST_YUV2YUV_Y2R_COE		0x0480
#define	RK3399_POST_YUV2YUV_3X3_COE		0x04a0
#define	RK3399_POST_YUV2YUV_R2Y_COE		0x04c0
#define	RK3399_WIN0_YUV2YUV_Y2R			0x04e0
#define	RK3399_WIN0_YUV2YUV_3X3			0x0500
#define	RK3399_WIN0_YUV2YUV_R2Y			0x0520
#define	RK3399_WIN1_YUV2YUV_Y2R			0x0540
#define	RK3399_WIN1_YUV2YUV_3X3			0x0560
#define	RK3399_WIN1_YUV2YUV_R2Y			0x0580
#define	RK3399_WIN2_YUV2YUV_Y2R			0x05a0
#define	RK3399_WIN2_YUV2YUV_3X3			0x05c0
#define	RK3399_WIN2_YUV2YUV_R2Y			0x05e0
#define	RK3399_WIN3_YUV2YUV_Y2R			0x0600
#define	RK3399_WIN3_YUV2YUV_3X3			0x0620
#define	RK3399_WIN3_YUV2YUV_R2Y			0x0640
#define	RK3399_WIN2_LUT_ADDR			0x1000
#define	RK3399_WIN3_LUT_ADDR			0x1400
#define	RK3399_HWC_LUT_ADDR			0x1800
#define	RK3399_CABC_GAMMA_LUT_ADDR		0x1c00
#define	RK3399_GAMMA_LUT_ADDR			0x2000

enum rockchip_data_format {
	ARGB8888 = 0,
	RGB888 = 1,
	RGB565 = 2,
};

enum {
	LB_YUV_3840X5 = 0x0,
	LB_YUV_2560X8 = 0x1,
	LB_RGB_3840X2 = 0x2,
	LB_RGB_2560X4 = 0x3,
	LB_RGB_1920X5 = 0x4,
	LB_RGB_1280X8 = 0x5
};

enum vop_modes {
	VOP_MODE_EDP = 0,
	VOP_MODE_MIPI,
	VOP_MODE_HDMI,
	VOP_MODE_MIPI1,
	VOP_MODE_DP,
	VOP_MODE_NONE,
};

enum vop_pol {
	HSYNC_POSITIVE = 0,
	VSYNC_POSITIVE,
	DEN_NEGATIVE,
	DCLK_INVERT,
};

/* generic display timings */
enum display_flags {
	DISPLAY_FLAGS_HSYNC_LOW		= 1 << 0,
	DISPLAY_FLAGS_HSYNC_HIGH	= 1 << 1,
	DISPLAY_FLAGS_VSYNC_LOW		= 1 << 2,
	DISPLAY_FLAGS_VSYNC_HIGH	= 1 << 3,
	DISPLAY_FLAGS_DE_LOW		= 1 << 4,
	DISPLAY_FLAGS_DE_HIGH		= 1 << 5,
	DISPLAY_FLAGS_PIXDATA_POSEDGE	= 1 << 6,
	DISPLAY_FLAGS_PIXDATA_NEGEDGE	= 1 << 7,
	DISPLAY_FLAGS_INTERLACED	= 1 << 8,
	DISPLAY_FLAGS_DOUBLESCAN	= 1 << 9,
	DISPLAY_FLAGS_DOUBLECLK		= 1 << 10,
};

struct timing_entry {
	uint32_t min;
	uint32_t typ;
	uint32_t max;
};

struct display_timing {
	struct timing_entry pixelclock;

	struct timing_entry hactive;		/* hor. active video */
	struct timing_entry hfront_porch;	/* hor. front porch */
	struct timing_entry hback_porch;	/* hor. back porch */
	struct timing_entry hsync_len;		/* hor. sync len */

	struct timing_entry vactive;		/* ver. active video */
	struct timing_entry vfront_porch;	/* ver. front porch */
	struct timing_entry vback_porch;	/* ver. back porch */
	struct timing_entry vsync_len;		/* ver. sync len */

	enum display_flags flags;		/* display flags */
	bool hdmi_monitor;			/* is hdmi monitor? */
};

#endif /* !_ARM64_ROCKCHIP_RK3399_VOP_ */
