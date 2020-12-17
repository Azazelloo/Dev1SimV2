/** @defgroup config_params Настройки сбора данных модуля */

/***************************************************************************//**
    @addtogroup config_params
    @{
    @file l502_params.h
    Файл содержит определение структур, описывающих текущие
    настройки сбора данных для модуля и объявляет
    внешнюю переменную g_set, содержащую эти параметры
*******************************************************************************/

#ifndef L502_PARAMS_H_
#define L502_PARAMS_H_


#include "l502_defs.h"

/** Калибровочные коэффициенты ЦАП */
typedef struct {
    float offs; /**< Смещение нуля */
    float k; /**< Коэффициент шкалы */
} t_dac_cbr_coef;

/** Информация о используемом модуле */
typedef struct {
    uint32_t devflags; /**< Флаги с информацией о наличии опций и сост. модуля */
    uint16_t fpga_ver; /**< Версия FPGA */
    uint8_t  plda_ver; /**< Версия PLDA */
    t_dac_cbr_coef dac_cbr[L502_DAC_CH_CNT]; /**< Коэффициенты ЦАП */
} t_module_info;


/** Параметры логического канала */
typedef struct {
    uint8_t phy_ch; /**< Номер физического канала */
    uint8_t mode; /**< Режим сбора */
    uint8_t range; /**< Диапазон */
    uint8_t avg; /**< Коэф. усреднения */
    uint32_t flags; /**< Доп. флаги */
} t_lch;


/** Настройки сбора данных */
typedef struct {
    /** Таблица логических каналов */
    t_lch lch[L502_LTABLE_MAX_CH_CNT];
    uint16_t lch_cnt; /**< Количество каналов в лог. таблице */
    uint32_t adc_freq_div; /**< Делитель частоты АЦП */
    uint32_t din_freq_div; /**< Делитель частоты для цифрового входа */
    uint32_t adc_frame_delay; /**< Межкадровая задержка */
    uint32_t ref_freq; /**< Значение опорной частоты (2 или 1.5 МГц) или внешняя */
    uint16_t out_freq_div; /**< Делитель частоты вывода */
    uint8_t sync_mode; /**< Режим синхронизации для опорной частоты */
    uint8_t sync_start_mode; /**< Режим запуска сигнала синхронизации */
} t_settings;

/** Структура, содержащая текущие настройки сбора данных */
extern t_settings g_set;
/** Структура, содержащая информацию о модуле */
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
