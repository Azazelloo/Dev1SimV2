/** @defgroup config_params ��������� ����� ������ ������ */

/***************************************************************************//**
    @addtogroup config_params
    @{
    @file l502_params.h
    ���� �������� ����������� ��������, ����������� �������
    ��������� ����� ������ ��� ������ � ���������
    ������� ���������� g_set, ���������� ��� ���������
*******************************************************************************/

#ifndef L502_PARAMS_H_
#define L502_PARAMS_H_


#include "l502_defs.h"

/** ������������� ������������ ��� */
typedef struct {
    float offs; /**< �������� ���� */
    float k; /**< ����������� ����� */
} t_dac_cbr_coef;

/** ���������� � ������������ ������ */
typedef struct {
    uint32_t devflags; /**< ����� � ����������� � ������� ����� � ����. ������ */
    uint16_t fpga_ver; /**< ������ FPGA */
    uint8_t  plda_ver; /**< ������ PLDA */
    t_dac_cbr_coef dac_cbr[L502_DAC_CH_CNT]; /**< ������������ ��� */
} t_module_info;


/** ��������� ����������� ������ */
typedef struct {
    uint8_t phy_ch; /**< ����� ����������� ������ */
    uint8_t mode; /**< ����� ����� */
    uint8_t range; /**< �������� */
    uint8_t avg; /**< ����. ���������� */
    uint32_t flags; /**< ���. ����� */
} t_lch;


/** ��������� ����� ������ */
typedef struct {
    /** ������� ���������� ������� */
    t_lch lch[L502_LTABLE_MAX_CH_CNT];
    uint16_t lch_cnt; /**< ���������� ������� � ���. ������� */
    uint32_t adc_freq_div; /**< �������� ������� ��� */
    uint32_t din_freq_div; /**< �������� ������� ��� ��������� ����� */
    uint32_t adc_frame_delay; /**< ����������� �������� */
    uint32_t ref_freq; /**< �������� ������� ������� (2 ��� 1.5 ���) ��� ������� */
    uint16_t out_freq_div; /**< �������� ������� ������ */
    uint8_t sync_mode; /**< ����� ������������� ��� ������� ������� */
    uint8_t sync_start_mode; /**< ����� ������� ������� ������������� */
} t_settings;

/** ���������, ���������� ������� ��������� ����� ������ */
extern t_settings g_set;
/** ���������, ���������� ���������� � ������ */
extern t_module_info g_module_info;

int32_t configure(void);

int32_t params_set_lch_cnt(uint32_t lch_cnt);
int32_t params_set_lch(uint32_t index, uint32_t ch, t_l502_lch_mode mode, 
                     t_l502_adc_range range, uint32_t avg, uint32_t flags);
int32_t params_set_adc_freq_div(uint32_t div);
int32_t params_set_adc_interframe_delay(uint32_t delay);
int32_t params_set_ref_freq(uint32_t freq_code);
int32_t params_set_sync_mode(t_l502_sync_mode sync_mode);
int32_t params_set_sync_start_mode(t_l502_sync_mode sync_mode);
int32_t params_set_din_freq_div(uint32_t div);
int32_t params_set_dac_freq_div(uint32_t div);
int32_t params_set_dac_freq_div(uint32_t div);


#endif

/** @} */
