/* Copyright 2013-2015 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <skiboot.h>
#include <io.h>
#include <timebase.h>
#include <pci.h>
#include <pci-virt.h>
#include <interrupts.h>
#include <npu2.h>
#include <npu2-regs.h>
#include <xscom.h>

/* PHY Registers. The documentation for the PHY training is written in
 * terms of bits within an actual register so we use that
 * representation here. */
struct npu2_phy_reg {
	uint64_t offset;
	uint64_t start;
	uint64_t len;
};

struct npu2_phy_reg NPU2_PHY_RX_DATA_DAC_SPARE_MODE	= {0x000, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_DAC_CNTL6		= {0x00c, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_DAC_CNTL5		= {0x028, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_DAC_CNTL9		= {0x030, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_DAC_CNTL5_EO		= {0x00a, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_DAC_CNTL4		= {0x026, 63, 64};
struct npu2_phy_reg NPU2_PHY_RX_RUN_LANE		= {0x0c8, 48, 1};
struct npu2_phy_reg NPU2_PHY_RX_IORESET			= {0x096, 63, 1};
struct npu2_phy_reg NPU2_PHY_TX_IORESET			= {0x113, 48, 1};
struct npu2_phy_reg NPU2_PHY_RX_PR_RESET		= {0x096, 62, 1};
struct npu2_phy_reg NPU2_PHY_RX_LANE_ANA_PDWN		= {0x002, 54, 1};
struct npu2_phy_reg NPU2_PHY_RX_LANE_DIG_PDWN		= {0x088, 48, 1};
struct npu2_phy_reg NPU2_PHY_RX_PR_IQ_RES_SEL		= {0x004, 59, 3};
struct npu2_phy_reg NPU2_PHY_RX_PR_PHASE_STEP		= {0x08a, 60, 4};
struct npu2_phy_reg NPU2_PHY_TX_LANE_PDWN		= {0x101, 48, 1};
struct npu2_phy_reg NPU2_PHY_RX_RUN_DCCAL		= {0x0c8, 49, 1};
struct npu2_phy_reg NPU2_PHY_RX_DCCAL_DONE		= {0x0ca, 49, 1};
struct npu2_phy_reg NPU2_PHY_RX_LANE_BUSY		= {0x0ca, 50, 1};
struct npu2_phy_reg NPU2_PHY_RX_B_BANK_CONTROLS		= {0x002, 58, 6};
struct npu2_phy_reg NPU2_PHY_TX_UNLOAD_CLK_DISABLE	= {0x103, 56, 1};
struct npu2_phy_reg NPU2_PHY_TX_FIFO_INIT		= {0x105, 53, 1};
struct npu2_phy_reg NPU2_PHY_TX_RXCAL			= {0x103, 57, 1};
struct npu2_phy_reg NPU2_PHY_RX_INIT_DONE		= {0x0ca, 48, 1};

/* These registers are per-PHY, not per lane */
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_SWO_EN		= {0x3c9, 48, 1};
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_REQ		= {0x3c1, 49, 1};
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_DONE		= {0x3c1, 50, 1};
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_ERROR		= {0x3c1, 51, 1};
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_N			= {0x3c3, 48, 9};
struct npu2_phy_reg NPU2_PHY_TX_ZCAL_P			= {0x3c5, 48, 9};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_PRE_EN		= {0x34d, 51, 5};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_PRE_SELECT		= {0x34d, 56, 5};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_PRE_EN		= {0x34f, 51, 5};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_PRE_SELECT		= {0x34f, 56, 5};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_POST_EN		= {0x361, 49, 7};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_POST_SELECT	= {0x361, 56, 7};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_POST_EN		= {0x363, 49, 7};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_POST_SELECT	= {0x363, 56, 7};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_MARGINPU_EN	= {0x351, 48, 8};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_MARGINPU_EN	= {0x353, 48, 8};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_MARGINPD_EN	= {0x351, 56, 8};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_MARGINPD_EN	= {0x353, 56, 8};
struct npu2_phy_reg NPU2_PHY_TX_MARGINPU_SELECT		= {0x355, 48, 8};
struct npu2_phy_reg NPU2_PHY_TX_MARGINPD_SELECT		= {0x355, 56, 8};
struct npu2_phy_reg NPU2_PHY_TX_PSEG_MAIN_EN		= {0x357, 51, 7};
struct npu2_phy_reg NPU2_PHY_TX_NSEG_MAIN_EN		= {0x359, 51, 7};
struct npu2_phy_reg NPU2_PHY_RX_HIST_MIN_EYE_WIDTH	= {0x24e, 54, 8};
struct npu2_phy_reg NPU2_PHY_RX_HIST_MIN_EYE_WIDTH_LANE	= {0x24e, 49, 5};
struct npu2_phy_reg NPU2_PHY_RX_HIST_MIN_EYE_WIDTH_VALID= {0x24e, 48, 1};

#define NPU2_PHY_REG(scom_base, reg, lane)					\
	SETFIELD(PPC_BITMASK(27, 31), ((reg)->offset << 42) | scom_base, lane)

#define NPU2_MAX_PHY_LANES			24

/* This is a bit of a gross hack but it does the job */
#define FOR_EACH_LANE(ndev, lane) \
	for (lane = 0; lane < NPU2_MAX_PHY_LANES; lane++)	\
		if (!(ndev->lane_mask & (1 << lane)))		\
			continue;				\
		else

typedef uint32_t (*step)(struct npu2_dev *);

struct procedure {
	const char *name;
	step steps[];
};

#define DEFINE_PROCEDURE(NAME, STEPS...)		\
	static struct procedure procedure_##NAME =	\
	{.name = #NAME, .steps = {NAME, ##STEPS}}

#define PROCEDURE_INPROGRESS	(1 << 31)
#define PROCEDURE_COMPLETE	(1 << 30)
#define PROCEDURE_NEXT		(1 << 29)
#define PROCEDURE_FAILED	2
#define PROCEDURE_ABORTED 	3
#define PROCEDURE_UNSUPPORTED	4

/* Mask defining which status bits we want to expose */
#define PROCEDURE_STATUS_MASK	0xc000000f

static void phy_write_lane(struct npu2_dev *ndev, struct npu2_phy_reg *reg, int lane, uint64_t val)
{
	uint64_t old_val, reg_addr;
	int rc;
	uint64_t mask = PPC_BITMASK(reg->start, reg->start + reg->len - 1);

	/* Check to make sure we're not trying to specify a lane to a
	 * non-per-lane register */
	if (lane >= 0)
		assert(reg->offset < 0x200);
	else
		assert(reg->offset >= 0x200);

	reg_addr = NPU2_PHY_REG(ndev->pl_xscom_base, reg, lane);
	rc = xscom_read(ndev->npu->chip_id, reg_addr, &old_val);
	if (rc)
		NPU2DEVERR(ndev, "error %d reading scom 0x%llx\n", rc, reg_addr);
	val = SETFIELD(mask, old_val, val);
	rc = xscom_write(ndev->npu->chip_id, reg_addr, val);
	if (rc)
		NPU2DEVERR(ndev, "error %d writing scom 0x%llx\n", rc, reg_addr);
}

static uint64_t phy_read_lane(struct npu2_dev *ndev, struct npu2_phy_reg *reg, int lane)
{
	uint64_t val, reg_addr;
	int rc;
	uint64_t mask = PPC_BITMASK(reg->start, reg->start + reg->len - 1);

	/* Check to make sure we're not trying to specify a lane to a
	 * non-per-lane register */
	if (lane >= 0)
		assert(reg->offset < 0x200);
	else
		assert(reg->offset >= 0x200);

	reg_addr = NPU2_PHY_REG(ndev->pl_xscom_base, reg, lane);
	rc = xscom_read(ndev->npu->chip_id, reg_addr, &val);
	if (rc)
		NPU2DEVERR(ndev, "error %d reading scom 0x%llx\n", rc, reg_addr);

	return GETFIELD(mask, val);
}

#define phy_write(ndev, reg, val) phy_write_lane(ndev, reg, -1, val)
#define phy_read(ndev, reg) phy_read_lane(ndev, reg, -1)

static uint32_t stop(struct npu2_dev *npu_dev __unused)
{
	return PROCEDURE_COMPLETE | PROCEDURE_ABORTED;
}
DEFINE_PROCEDURE(stop);

static uint32_t nop(struct npu2_dev *npu_dev __unused)
{
	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(nop);

/* Procedure 1.2.1 (RESET_NPU_DL) from opt_programmerguide.odt. Also
 * incorporates AT reset. */
static uint32_t reset_ntl(struct npu2_dev *ndev __unused)
{
	return PROCEDURE_NEXT;
}

static uint32_t reset_ndl(struct npu2_dev *ndev)
{
	uint64_t val;

	if ((ndev->pl_xscom_base & 0xFFFFFFFF) == 0x9010C3F)
		val = SETFIELD(PPC_BITMASK(0,1), 0ull, ndev->index % 3);
	else {
		switch (ndev->index % 3) {
		case 0:
			val = SETFIELD(PPC_BITMASK(0,1), 0ull, 2);
			break;
		case 1:
			val = SETFIELD(PPC_BITMASK(0,1), 0ull, 1);
			break;
		case 2:
			val = SETFIELD(PPC_BITMASK(0,1), 0ull, 0);
			break;
		}
	}

	npu2_write_mask(ndev->npu, NPU2_NTL_PRI_CFG(ndev), val, -1ULL);

	val = PPC_BIT(0) | PPC_BIT(1);

	npu2_write_4b(ndev->npu, NPU2_NTL_DL_CONTROL(ndev), val);
	npu2_write_4b(ndev->npu, NPU2_NTL_DL_CONTROL(ndev), 0);
	npu2_write_4b(ndev->npu, NPU2_NTL_DL_CONFIG(ndev), PPC_BIT(0));

	/* NTL Reset */
	val = PPC_BIT(8) | PPC_BIT(9);
	npu2_write(ndev->npu, NPU2_NTL_MISC_CFG1(ndev), val);
	val = PPC_BIT(8);
	npu2_write(ndev->npu, NPU2_NTL_MISC_CFG1(ndev), val);
	val = 0;
	npu2_write(ndev->npu, NPU2_NTL_MISC_CFG1(ndev), val);

	return PROCEDURE_NEXT;
}

static uint32_t reset_ntl_release(struct npu2_dev *ndev)
{
	/* Credit Setup */
	npu2_write(ndev->npu, NPU2_NTL_CRED_HDR_CREDIT_TX(ndev), 0x0200000000000000);
	npu2_write(ndev->npu, NPU2_NTL_PRB_HDR_CREDIT_TX(ndev), 0x0200000000000000);
	npu2_write(ndev->npu, NPU2_NTL_ATR_HDR_CREDIT_TX(ndev), 0x0200000000000000);
	npu2_write(ndev->npu, NPU2_NTL_RSP_HDR_CREDIT_TX(ndev), 0x0200000000000000);
	npu2_write(ndev->npu, NPU2_NTL_CRED_DATA_CREDIT_TX(ndev), 0x1000000000000000);
	npu2_write(ndev->npu, NPU2_NTL_RSP_DATA_CREDIT_TX(ndev), 0x1000000000000000);
	npu2_write(ndev->npu, NPU2_NTL_CRED_HDR_CREDIT_RX(ndev), 0x0000BE0000000000);
	npu2_write(ndev->npu, NPU2_NTL_DBD_HDR_CREDIT_RX(ndev), 0x0000640000000000);
	npu2_write(ndev->npu, NPU2_NTL_ATSD_HDR_CREDIT_RX(ndev), 0x0000200000000000);
	npu2_write(ndev->npu, NPU2_NTL_RSP_HDR_CREDIT_RX(ndev), 0x0000BE0000000000);
	npu2_write(ndev->npu, NPU2_NTL_CRED_DATA_CREDIT_RX(ndev), 0x0001000000000000);
	npu2_write(ndev->npu, NPU2_NTL_RSP_DATA_CREDIT_RX(ndev), 0x0001000000000000);

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(reset_ntl, reset_ndl, reset_ntl_release);

/* Procedure 1.2.2 - Reset I/O PHY Lanes */
static uint32_t phy_reset(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_RX_RUN_LANE, lane, 0);

	return PROCEDURE_NEXT;
}

static uint32_t phy_reset_wait(struct npu2_dev *ndev)
{
	int lane;

	/* Wait for all lanes to become inactive */
	FOR_EACH_LANE(ndev, lane)
		if (phy_read_lane(ndev, &NPU2_PHY_RX_LANE_BUSY, lane))
			return PROCEDURE_INPROGRESS;

	FOR_EACH_LANE(ndev, lane) {
		/* Set lane in reset */
		phy_write_lane(ndev, &NPU2_PHY_RX_IORESET, lane, 1);
		phy_write_lane(ndev, &NPU2_PHY_TX_IORESET, lane, 1);

		/* Release lane from reset */
		phy_write_lane(ndev, &NPU2_PHY_RX_IORESET, lane, 0);
		phy_write_lane(ndev, &NPU2_PHY_TX_IORESET, lane, 0);

		/* Reset the phase rotator */
		phy_write_lane(ndev, &NPU2_PHY_RX_PR_RESET, lane, 1);
		phy_write_lane(ndev, &NPU2_PHY_RX_PR_RESET, lane, 0);
	}

	return PROCEDURE_NEXT;
}

/* Procedure 1.2.3 - Initialise I/O PHY Registers */
static uint32_t phy_reset_complete(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane) {
		phy_write_lane(ndev, &NPU2_PHY_RX_LANE_ANA_PDWN, lane, 0);
		phy_write_lane(ndev, &NPU2_PHY_RX_LANE_DIG_PDWN, lane, 0);
		phy_write_lane(ndev, &NPU2_PHY_RX_PR_IQ_RES_SEL, lane, 0x7);
		phy_write_lane(ndev, &NPU2_PHY_RX_PR_PHASE_STEP, lane, 0x8);
		phy_write_lane(ndev, &NPU2_PHY_TX_LANE_PDWN, lane, 0);
	}

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(phy_reset, phy_reset_wait, phy_reset_complete);

static uint32_t phy_tx_zcal(struct npu2_dev *ndev)
{
	if (ndev->npu->tx_zcal_complete[ndev->index % 3])
		return PROCEDURE_COMPLETE;
	ndev->npu->tx_zcal_complete[ndev->index % 3] = 1;

	/* Turn off SW enable and enable zcal state machine */
	phy_write(ndev, &NPU2_PHY_TX_ZCAL_SWO_EN, 0);

	/* Start impedance calibration state machine */
	phy_write(ndev, &NPU2_PHY_TX_ZCAL_REQ, 1);

	return PROCEDURE_NEXT;
}

static uint32_t phy_tx_zcal_wait(struct npu2_dev *ndev)
{
	int done, error;

	done = phy_read(ndev, &NPU2_PHY_TX_ZCAL_DONE);
	error = phy_read(ndev, &NPU2_PHY_TX_ZCAL_ERROR);

	/* We have never seen this in the field and it is not
	 * expected. Therefore it's best to error out which will
	 * complain loudly in the logs than to continue with nominal
	 * values which wont work well. */
	if (error)
		return PROCEDURE_COMPLETE | PROCEDURE_FAILED;

	if (!done)
		return PROCEDURE_INPROGRESS;

	return PROCEDURE_NEXT;
}

/* FIXME: What are the right values? I just chose the middle ones... */
#define MARGIN_RATIO		(32)
#define FFE_PRE_COEFF		(7)
#define FFE_POST_COEFF		(12)

#define PRE_WIDTH		(5)
#define POST_WIDTH		(7)
#define MAIN_WIDTH		(7)
#define ZCAL_MIN		(16 * 2)
#define ZCAL_MAX		(33 * 2)
#define PRECURSOR_X2_MAX	(4 * 2 + 1)
#define POSTCURSOR_X2_MAX	(6 * 2 + 1)
#define MARGIN_X2_MAX		(8 * 2)
#define MAIN_X2_MAX		((6 * 2) + 1)
#define TOTAL_X2_MAX		(PRECURSOR_X2_MAX + POSTCURSOR_X2_MAX + 2*MARGIN_X2_MAX + MAIN_X2_MAX)

static uint32_t therm(uint32_t dec)
{
	return ((0x1 << dec) - 1);
}

static uint32_t therm_with_half(uint32_t dec, uint8_t width)
{
	/* If the LSB of the 2r equivalent is on, then we need to set the 2r bit (MSB) */
	uint32_t half_on = ( dec & 0x1 ) << ( width - 1 );

	/* Shift the 2r equivalent to a 1r value and convert to a thermometer code. */
	uint32_t x1_equiv = ((1 << (dec >> 1 )) - 1);

	/* Combine 1r equivalent thermometer code + the 2r MSB value. */
	return half_on | x1_equiv;
}

static uint32_t phy_tx_zcal_calculate(struct npu2_dev *ndev)
{
	int p_value, n_value;
	uint32_t zcal_n;
	uint32_t zcal_p;
	uint32_t p_main_enable = MAIN_X2_MAX;
	uint32_t p_margin_pu_enable = MARGIN_X2_MAX;
	uint32_t p_margin_pd_enable = MARGIN_X2_MAX;
	uint32_t p_precursor_select;
	uint32_t p_postcursor_select;
	uint32_t margin_pu_select;
	uint32_t n_main_enable = MAIN_X2_MAX;
	uint32_t n_margin_pu_enable = MARGIN_X2_MAX;
	uint32_t n_margin_pd_enable = MARGIN_X2_MAX;
	uint32_t n_precursor_select;
	uint32_t n_postcursor_select;
	uint32_t margin_pd_select;
	uint32_t margin_select;

	if (ndev->index < 3) {
		if (ndev->npu->tx_zcal_complete[0])
			return PROCEDURE_COMPLETE;
	} else {
		if (ndev->npu->tx_zcal_complete[1])
			return PROCEDURE_COMPLETE;
	}

	/* Convert the value from 8R to 2R by / 4 */
	zcal_n = phy_read(ndev, &NPU2_PHY_TX_ZCAL_N) / 4;
	zcal_p = phy_read(ndev, &NPU2_PHY_TX_ZCAL_P) / 4;

	/* Again, if the hardware detects an unexpected condition it's
	 * better just to fail loudly. */
	if ((zcal_n < ZCAL_MIN) || (zcal_n > ZCAL_MAX) ||
	    (zcal_p < ZCAL_MIN) || (zcal_p > ZCAL_MAX))
		return PROCEDURE_COMPLETE | PROCEDURE_FAILED;

	p_value = zcal_p - TOTAL_X2_MAX;
	p_precursor_select = (p_value * FFE_PRE_COEFF)/128;
	p_postcursor_select = (p_value * FFE_POST_COEFF)/128;
	margin_pu_select = (p_value * MARGIN_RATIO)/256;

	if (p_value % 2) {
		p_main_enable--;
		p_value++;
	}

	while (p_value < 0) {
		if (p_main_enable > 1) {
			p_main_enable -= 2;
		} else if ((p_margin_pu_enable + p_margin_pd_enable) > 0) {
			if (p_margin_pu_enable == p_margin_pd_enable)
				p_margin_pd_enable -= 2;
			else
				p_margin_pu_enable -= 2;
		}
		p_value += 2;
	}

	n_value = zcal_n - TOTAL_X2_MAX;
	n_precursor_select = (n_value * FFE_PRE_COEFF)/128;
	n_postcursor_select = (n_value * FFE_POST_COEFF)/128;
	margin_pd_select = (p_value * MARGIN_RATIO)/256;

	if (n_value % 2) {
		n_main_enable--;
		n_value++;
	}

	while (n_value < 0) {
		if (n_main_enable > 1) {
			n_main_enable -= 2;
		} else if ((n_margin_pu_enable + n_margin_pd_enable) > 0) {
			if (n_margin_pu_enable == n_margin_pd_enable)
				n_margin_pd_enable -= 2;
			else
				n_margin_pu_enable -= 2;
		}
		n_value += 2;
	}

	margin_select = therm((margin_pu_select + 1)/2) &
		therm((margin_pd_select + 1)/2) &
		therm((p_margin_pu_enable + 1)/2) &
		therm((p_margin_pd_enable + 1)/2) &
		therm((n_margin_pu_enable + 1)/2) &
		therm((n_margin_pd_enable + 1)/2);

	phy_write(ndev, &NPU2_PHY_TX_PSEG_PRE_EN, therm_with_half(PRECURSOR_X2_MAX, PRE_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_PRE_SELECT, therm_with_half(p_precursor_select, PRE_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_POST_EN, therm_with_half(POSTCURSOR_X2_MAX, POST_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_POST_SELECT, therm_with_half(p_postcursor_select, POST_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_MARGINPU_EN, therm((p_margin_pu_enable + 1)/2));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_MARGINPD_EN, therm((p_margin_pd_enable + 1)/2));
	phy_write(ndev, &NPU2_PHY_TX_PSEG_MAIN_EN, therm_with_half(p_main_enable, MAIN_WIDTH));

	phy_write(ndev, &NPU2_PHY_TX_NSEG_PRE_EN, therm_with_half(PRECURSOR_X2_MAX, PRE_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_PRE_SELECT, therm_with_half(n_precursor_select, PRE_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_POST_EN, therm_with_half(POSTCURSOR_X2_MAX, POST_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_POST_SELECT, therm_with_half(n_postcursor_select, POST_WIDTH));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_MARGINPU_EN, therm((n_margin_pu_enable + 1)/2));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_MARGINPD_EN, therm((n_margin_pd_enable + 1)/2));
	phy_write(ndev, &NPU2_PHY_TX_NSEG_MAIN_EN, therm_with_half(n_main_enable, MAIN_WIDTH));

	phy_write(ndev, &NPU2_PHY_TX_MARGINPU_SELECT, therm(margin_select + 1)/2);
	phy_write(ndev, &NPU2_PHY_TX_MARGINPD_SELECT, therm(margin_select + 1)/2);

	if (ndev->index < 3)
		ndev->npu->tx_zcal_complete[0] = true;
	else
		ndev->npu->tx_zcal_complete[1] = true;

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(phy_tx_zcal, phy_tx_zcal_wait, phy_tx_zcal_calculate);

/* Procedure 1.2.8 - Enable Downstream Link Training */
static uint32_t phy_enable_tx_rxcal(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_TX_RXCAL, lane, 1);

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(phy_enable_tx_rxcal);

static uint32_t phy_disable_tx_rxcal(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_TX_RXCAL, lane, 0);

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(phy_disable_tx_rxcal);

/* Procedure 1.2.4 - I/O PHY DC Calibration */
static uint32_t phy_rx_dccal(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_RX_RUN_DCCAL, lane, 1);

	return PROCEDURE_NEXT;
}

static uint32_t phy_rx_dccal_complete(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		if (!phy_read_lane(ndev, &NPU2_PHY_RX_DCCAL_DONE, lane))
			return PROCEDURE_INPROGRESS;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_RX_RUN_DCCAL, lane, 0);

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_RX_B_BANK_CONTROLS, lane, 0);

	return PROCEDURE_NEXT;
}

/* Procedure 1.2.5 - IO PHY Tx FIFO Init */
static uint32_t phy_tx_fifo_init(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane) {
		phy_write_lane(ndev, &NPU2_PHY_TX_UNLOAD_CLK_DISABLE, lane, 0);
		phy_write_lane(ndev, &NPU2_PHY_TX_FIFO_INIT, lane, 1);
		phy_write_lane(ndev, &NPU2_PHY_TX_UNLOAD_CLK_DISABLE, lane, 1);
	}

	return PROCEDURE_COMPLETE;
}

/* We group TX FIFO init in here mainly because that's what was done
 * on NVLink1 */
DEFINE_PROCEDURE(phy_rx_dccal, phy_rx_dccal_complete, phy_tx_fifo_init);

/* Procedure 1.2.7 - I/O PHY Upstream Link Training */
static uint32_t phy_rx_training(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		phy_write_lane(ndev, &NPU2_PHY_RX_RUN_LANE, lane, 1);

	return PROCEDURE_NEXT;
}

static uint32_t phy_rx_training_wait(struct npu2_dev *ndev)
{
	int lane;

	FOR_EACH_LANE(ndev, lane)
		if (!phy_read_lane(ndev, &NPU2_PHY_RX_INIT_DONE, lane))
			return PROCEDURE_INPROGRESS;

	return PROCEDURE_COMPLETE;
}
DEFINE_PROCEDURE(phy_rx_training, phy_rx_training_wait);

static struct procedure *npu_procedures[] = {
	&procedure_stop,
	&procedure_nop,
	NULL,
	NULL,
	&procedure_phy_reset,
	&procedure_phy_tx_zcal,
	&procedure_phy_rx_dccal,
	&procedure_phy_enable_tx_rxcal,
	&procedure_phy_disable_tx_rxcal,
	&procedure_phy_rx_training,
	&procedure_reset_ntl,

	/* Place holders for pre-terminate and terminate procedures */
	&procedure_nop,
	&procedure_nop};

/* Run a procedure step(s) and return status */
static uint32_t get_procedure_status(struct npu2_dev *dev)
{
	uint32_t result;
	uint16_t procedure = dev->procedure_number;
	uint16_t step = dev->procedure_step;
	const char *name = npu_procedures[procedure]->name;

	do {
		result = npu_procedures[procedure]->steps[step](dev);

		if (result & PROCEDURE_NEXT) {
			step++;
			NPU2DEVINF(dev, "Running procedure %s step %d\n", name, step);
		}
	} while (result & PROCEDURE_NEXT);

	dev->procedure_step = step;

	if (result & PROCEDURE_COMPLETE)
		NPU2DEVINF(dev, "Procedure %s complete\n", name);
	else if (mftb() > dev->procedure_tb + msecs_to_tb(1000)) {
		NPU2DEVINF(dev, "Procedure %s timed out\n", name);
		result = PROCEDURE_COMPLETE | PROCEDURE_FAILED;
	}

	/* Mask off internal state bits */
	dev->procedure_status = result & PROCEDURE_STATUS_MASK;

	return dev->procedure_status;
}

static int64_t npu_dev_procedure_read(struct npu2_dev *dev, uint32_t offset,
				      uint32_t size, uint32_t *data)
{
	int64_t rc = OPAL_SUCCESS;

	if (size != 4) {
		/* Short config reads are not supported */
		prlog(PR_ERR, "NPU%d: Short read of procedure register\n", dev->npu->phb.opal_id);
		return OPAL_PARAMETER;
	}

	*data = 0;

	switch (offset) {
	case 0:
		/* Only run the procedure if not already complete */
		if (dev->procedure_status & PROCEDURE_COMPLETE)
			*data = dev->procedure_status;
		else
			*data = get_procedure_status(dev);

		break;

	case 4:
		*data = dev->procedure_number;
		break;

	default:
		prlog(PR_ERR, "NPU%d: Invalid vendor specific offset 0x%08x\n",
		      dev->npu->phb.opal_id, offset);
		rc = OPAL_PARAMETER;
	}

	return rc;
}

static int64_t npu_dev_procedure_write(struct npu2_dev *dev, uint32_t offset,
				       uint32_t size, uint32_t data)
{
	const char *name;
	int64_t rc = OPAL_SUCCESS;

	if (size != 4) {
		/* Short config writes are not supported */
		prlog(PR_ERR, "NPU%d: Short read of procedure register\n",
		      dev->npu->phb.opal_id);
		return OPAL_PARAMETER;
	}

	switch (offset) {
	case 0:
		/* We ignore writes to the status register */
		NPU2DEVINF(dev, "Ignoring writes to status register\n");
		break;

	case 4:
		if (data >= ARRAY_SIZE(npu_procedures) ||
		    !npu_procedures[data]) {
			NPU2DEVINF(dev, "Unsupported procedure number %d\n", data);
			dev->procedure_status = PROCEDURE_COMPLETE
				| PROCEDURE_UNSUPPORTED;
			break;
		}

		name = npu_procedures[data]->name;
		if (dev->procedure_number == data
		    && !(dev->procedure_status & PROCEDURE_COMPLETE))
			NPU2DEVINF(dev, "Restarting procuedure %s\n", name);
		else
			NPU2DEVINF(dev, "Starting procedure %s\n", name);

		dev->procedure_status = PROCEDURE_INPROGRESS;
		dev->procedure_number = data;
		dev->procedure_step = 0;
		dev->procedure_data = 0;
		dev->procedure_tb = mftb();
		break;

	default:
		NPU2DEVINF(dev, "Invalid vendor specific offset 0x%08x\n", offset);
		rc = OPAL_PARAMETER;
	}

	return rc;
}

int64_t npu2_dev_procedure(void *dev, struct pci_cfg_reg_filter *pcrf,
			   uint32_t offset, uint32_t len, uint32_t *data,
			   bool write)
{
	struct pci_virt_device *pvd = dev;
	struct npu2_dev *ndev = pvd->data;

	if (write)
		return npu_dev_procedure_write(ndev, offset - pcrf->start,
					       len, *data);

	return npu_dev_procedure_read(ndev, offset - pcrf->start, len, data);
}
