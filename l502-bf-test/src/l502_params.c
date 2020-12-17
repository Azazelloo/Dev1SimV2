/**  @addtogroup config_params
    @{
    @file l502_params.c
    Файл содержит функции по обработке команд от ПК на установку параметров
    конфигурации сбора данных и других параметров.
    Только в данном файле должны быть изменения полей структуры общих
    настроек - g_set.
    Так же здесь  */


#include <stdint.h>
#include <stdlib.h>

#include "l502_global.h"
#include "l502_cmd.h"
#include "l502_fpga.h"
#include "l502_params.h"
#include "l502_stream.h"



/* проверка правильного режима синхронизации */
#define CHECK_SYNC_MODE(cmd)  (((cmd) != L502_SYNC_INTERNAL) \
                            && ((cmd) !=L502_SYNC_EXTERNAL_MASTER) \
                             && ((cmd) != L502_SYNC_DI_SYN1_RISE) \
                             && ((cmd) != L502_SYNC_DI_SYN2_RISE) \
                            && ((cmd) != L502_SYNC_DI_SYN1_FALL) \
                            && ((cmd) != L502_SYNC_DI_SYN2_FALL) ? L502_BF_ERR_INVALID_CMD_PARAMS : 0)


t_settings g_set = {
    .lch_cnt = 1,
    .adc_freq_div = 1,
    .adc_frame_delay = 0,
    .din_freq_div = 2,
    .ref_freq = L502_REF_FREQ_2000KHZ,
    .out_freq_div = X502_OUT_FREQ_DIV_DEFAULT,
    .sync_mode    = L502_SYNC_INTERNAL,
    .sync_start_mode     = L502_SYNC_INTERNAL
    };

t_module_info g_module_info;




/** @brief Установка количества логических каналов

    Проверка и запись в поле g_set.lch_cnt значение кол-ва каналов в логической
    таблице АЦП.

    @param[in] lch_cnt   Количество логических каналов (от 1 до #L502_LTABLE_MAX_CH_CNT)
    @return              Код ошибки */
int32_t params_set_lch_cnt(uint32_t lch_cnt) {
    if (lch_cnt > L502_LTABLE_MAX_CH_CNT)
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    g_set.lch_cnt = lch_cnt;
    return 0;
}

/** @brief Установить параметры логического канала

    Функция проверяет входные параметры и записывает их в соответствующее поле
    таблицы g_set.lch[]

    @param[in] index   Номер логического канала [0, L502_LTABLE_MAX_CH_CNT-1]
    @param[in] ch      Номер физического канала (от 0 до 15 или 31)
    @param[in] mode    Режим измерения для данного лог. канала
    @param[in] range   Диапазон измерения для данного лог. канала
    @param[in] avg     Коэф. усреднения по данному лог. каналу
    @param[in] flags   Дополнительные флаги
    @return            Код ошибки */
int32_t params_set_lch(uint32_t index, uint32_t ch, t_l502_lch_mode mode, 
                     t_l502_adc_range range, uint32_t avg, uint32_t flags) {
    if (index >= L502_LTABLE_MAX_CH_CNT)
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    if ((mode!=L502_LCH_MODE_COMM) && (mode != L502_LCH_MODE_DIFF) &&
        (mode!=L502_LCH_MODE_ZERO))
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    if ((ch >= 32) || ((mode != L502_LCH_MODE_COMM) && (ch>=16)))
        return L502_BF_ERR_INVALID_CMD_PARAMS;


    g_set.lch[index].phy_ch = ch;
    g_set.lch[index].mode = mode;
    g_set.lch[index].range = range;
    g_set.lch[index].avg = avg;
    g_set.lch[index].flags = flags;
    return 0;
}

/** Установка делителя частоты АЦП
    @param[in] div   Значение делителя
    @return          Код ошибки */
int32_t params_set_adc_freq_div(uint32_t div) {
    if ((div==0) || (div > L502_ADC_FREQ_DIV_MAX))
         return L502_BF_ERR_INVALID_CMD_PARAMS;
    g_set.adc_freq_div = div;
    return 0;
}

/** Установка значения опорной частоты
    @param[in] freq_code   Значение частоты. Для внутренней может быть только
                           #L502_REF_FREQ_2000KHZ или #L502_REF_FREQ_1500KHZ
    @return                Код ошибки */
int32_t params_set_ref_freq(uint32_t freq_code) {
    g_set.ref_freq = freq_code;
    return 0;
}
/** Установка значения межкадровой задержки
    @param[in] delay    Значение межкадровой задержки (от 0 до L502_ADC_INTERFRAME_DELAY_MAX)
    @return             Код ошибки */
int32_t params_set_adc_interframe_delay(uint32_t delay) {
    if (delay > L502_ADC_INTERFRAME_DELAY_MAX)
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    g_set.adc_frame_delay = delay;
    return 0;
}

/** Установка источника опроной частоты синхронизации
    @param[in] sync_mode    Значение из #t_l502_sync_mode
    @return             Код ошибки */
int32_t params_set_sync_mode(t_l502_sync_mode sync_mode) {
    int32_t err = CHECK_SYNC_MODE(sync_mode);
    if (!err)
        g_set.sync_mode = sync_mode;
    return err;
}

/** Установка источника синхронизации старта сбора данных
    @param[in] sync_mode    Значение из #t_l502_sync_mode
    @return             Код ошибки */
int32_t params_set_sync_start_mode(t_l502_sync_mode sync_mode) {
    int32_t err = CHECK_SYNC_MODE(sync_mode);
    if (!err)
        g_set.sync_start_mode = sync_mode;
    return err;
}


/** Установка делителя частоты синхронного ввода цифровых линий
    @param[in] div   Значение делителя
    @return          Код ошибки */
int32_t params_set_din_freq_div(uint32_t div) {
    if ((div==0) || (div > L502_DIN_FREQ_DIV_MAX))
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    g_set.din_freq_div = div;
    return 0;
}

/** Установка делителя частоты вывода на ЦАП
    @param[in] div   Значение делителя (1 или 2)
    @return          Код ошибки */
int32_t params_set_dac_freq_div(uint32_t div) {
    if ((div < X502_OUT_FREQ_DIV_MIN) || (div > X502_OUT_FREQ_DIV_MAX))
        return L502_BF_ERR_INVALID_CMD_PARAMS;
    g_set.out_freq_div = div;
    return 0;
}




/** @brief Запись параметров сбора в регистры ПЛИС

    Функция выполняет запись всех параметров из структуры #g_set в регистры
    ПЛИС. Функция может вызываться только когда сбор данных остановлен.

    @return Код ошибки */
int32_t configure(void) {
    uint16_t ch;

    int32_t err = g_mode != L502_BF_MODE_IDLE ? L502_BF_ERR_STREAM_RUNNING : 0;
    if (!err) {
        /* записываем логическую таблицу */
        for (ch = 0; ch < g_set.lch_cnt; ch++) {
            uint32_t wrd = ((g_set.lch[ch].phy_ch & 0xF) << 3) | (g_set.lch[ch].range & 0x7);

            if (g_set.lch[ch].mode == L502_LCH_MODE_ZERO) {
                wrd |= (3 << 7);
            } else if (g_set.lch[ch].mode == L502_LCH_MODE_COMM) {
                wrd |= (g_set.lch[ch].phy_ch & 0x10 ? 2 : 1) << 7;
            }

            if (g_set.lch[ch].avg)
                wrd |= ((g_set.lch[ch].avg-1) & 0x7F) << 9;

            fpga_reg_write(L502_REGS_IOHARD_LTABLE + g_set.lch_cnt - 1 - ch, wrd);
        }

        fpga_reg_write(L502_REGS_IOHARD_LCH_CNT, g_set.lch_cnt - 1);

        fpga_reg_write(L502_REGS_IOHARD_ADC_FREQ_DIV,  g_set.adc_freq_div - 1);
        fpga_reg_write(L502_REGS_IOARITH_ADC_FREQ_DIV, g_set.adc_freq_div - 1);

        fpga_reg_write(L502_REGS_IOHARD_ADC_FRAME_DELAY, g_set.adc_frame_delay);

        fpga_reg_write(L502_REGS_IOHARD_IO_MODE, (g_set.sync_mode & 0x7)
                                  | ((g_set.sync_start_mode&0x7)<<3)
                                  | ((g_set.ref_freq==L502_REF_FREQ_2000KHZ ? 0 : 2) << 7)
                                  | (((g_set.out_freq_div-1)&0x3FF)<<9));

         fpga_reg_write(L502_REGS_IOHARD_DIGIN_FREQ_DIV, g_set.din_freq_div - 1);
    }
    return err;
}

/** @} */
