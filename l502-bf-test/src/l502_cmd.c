/***************************************************************************//**
    @addtogroup cmd_process
    @{
    @file l502_cmd.c
    Файл содержит логику обработки команд от ПК, переданных через
    HostDMA в BlackFin.
    Для каждого кода команды в таблице f_cmd_tbl задана функция для
    обработки команды. Если в таблице код команды не найден,
    то возвращается ошибка. Для пользовательских команд
    всегда вызывается usr_cmd_process().
*******************************************************************************/
    
#include "l502_global.h"
#include "l502_cmd.h"
#include "l502_stream.h"
#include "l502_params.h"
#include "l502_user_process.h"
#include "l502_async.h"
#include "l502_sport_tx.h"

#include <string.h>
#include <cdefBF523.h>
#include <sys/exception.h>
#include <ccblkfn.h>
#include "l502_fpga.h"



extern void l502_cmd_test(t_l502_bf_cmd *cmd);
static void f_cmd_set_param(t_l502_bf_cmd *cmd);
static void f_cmd_get_param(t_l502_bf_cmd *cmd);
static void f_cmd_config(t_l502_bf_cmd *cmd);
static void f_cmd_streams_start(t_l502_bf_cmd *cmd);
static void f_cmd_stream_stop(t_l502_bf_cmd *cmd);
static void f_cmd_preload(t_l502_bf_cmd *cmd);
static void f_cmd_stream_enable(t_l502_bf_cmd *cmd);
static void f_cmd_stream_disable(t_l502_bf_cmd *cmd);
static void f_cmd_async_out(t_l502_bf_cmd *cmd);
static void f_cmd_fpga_reg_wr(t_l502_bf_cmd *cmd);
static void f_cmd_fpga_reg_rd(t_l502_bf_cmd *cmd);
static void f_cmd_get_out_status(t_l502_bf_cmd *cmd);


/* Тип, описывающий функцию обработки конкретной команды */
typedef void (*t_cmd_func)(t_l502_bf_cmd* cmd);

static const uint32_t f_regaddr_k[L502_ADC_RANGE_CNT] = {L502_REGS_IOARITH_K10,
                                                     L502_REGS_IOARITH_K5,
                                                     L502_REGS_IOARITH_K2,
                                                     L502_REGS_IOARITH_K1,
                                                     L502_REGS_IOARITH_K05,
                                                     L502_REGS_IOARITH_K02};

static const uint32_t f_regaddr_offs[L502_ADC_RANGE_CNT] = {L502_REGS_IOARITH_B10,
                                                     L502_REGS_IOARITH_B5,
                                                     L502_REGS_IOARITH_B2,
                                                     L502_REGS_IOARITH_B1,
                                                     L502_REGS_IOARITH_B05,
                                                     L502_REGS_IOARITH_B02};



static volatile uint8_t f_cmd_req=0;
/* таблица с соответствием кодов команд и функций для их выполнения */
static const struct {
    uint32_t cmd_code;
    t_cmd_func start;
} f_cmd_tbl[] = {
    {L502_BF_CMD_CODE_TEST,             l502_cmd_test},
    {L502_BF_CMD_CODE_SET_PARAM,        f_cmd_set_param},
    {L502_BF_CMD_CODE_GET_PARAM,        f_cmd_get_param},
    {L502_BF_CMD_CODE_CONFIGURE,        f_cmd_config},
    {L502_BF_CMD_CODE_STREAM_START,     f_cmd_streams_start},
    {L502_BF_CMD_CODE_STREAM_STOP ,     f_cmd_stream_stop},
    {L502_BF_CMD_CODE_PRELOAD,          f_cmd_preload},
    {L502_BF_CMD_CODE_STREAM_EN,        f_cmd_stream_enable},
    {L502_BF_CMD_CODE_STREAM_DIS,       f_cmd_stream_disable},
    {L502_BF_CMD_CODE_ASYNC_OUT,        f_cmd_async_out},
    {L502_BF_CMD_CODE_FPGA_REG_WR,      f_cmd_fpga_reg_wr},
    {L502_BF_CMD_CODE_FPGA_REG_RD,      f_cmd_fpga_reg_rd},
    {L502_BF_CMD_CODE_GET_OUT_STATUS,   f_cmd_get_out_status},
};





void l502_cmd_done(int32_t result, uint32_t* data, uint32_t size) {
    g_state.cmd.result = result;
    g_state.cmd.data_size = size;
    if (size && (data!=g_state.cmd.data))
        memmove((void*)g_state.cmd.data, data, size*sizeof(data[0]));

    g_state.cmd.status = L502_BF_CMD_STATUS_DONE;
}



void l502_cmd_start(t_l502_bf_cmd* cmd) {
    uint32_t i, fnd;

    if (cmd->code & L502_BF_CMD_CODE_USER) {
        usr_cmd_process(cmd);
    } else {
        for (i=0, fnd=0; !fnd && (i < sizeof(f_cmd_tbl)/sizeof(f_cmd_tbl[0])); i++) {
            if (cmd->code == f_cmd_tbl[i].cmd_code) {
                fnd = 1;
                f_cmd_tbl[i].start(cmd);
            }
        }

        if (!fnd)
            l502_cmd_done(L502_BF_ERR_UNSUP_CMD, NULL, 0);
    }
}




void l502_cmd_check_req(void) {
    if (f_cmd_req == 1) {
        f_cmd_req=0;
        l502_cmd_start((void*)&g_state.cmd);
    }
}

void l502_cmd_set_req(void) {
    f_cmd_req = 1;
    g_state.cmd.status = L502_BF_CMD_STATUS_PROGRESS;
}

static void f_cmd_streams_start(t_l502_bf_cmd *cmd) {
    l502_cmd_done(streams_start(), NULL, 0);
}

static void f_cmd_stream_stop(t_l502_bf_cmd *cmd) {
    l502_cmd_done(streams_stop(), NULL, 0);
}

static void f_cmd_config(t_l502_bf_cmd *cmd) {
    l502_cmd_done(configure(), NULL, 0);
}

static void f_cmd_preload(t_l502_bf_cmd *cmd) {
    l502_cmd_done(stream_out_preload(), NULL, 0);
}

static void f_cmd_stream_enable(t_l502_bf_cmd *cmd) {
    l502_cmd_done(stream_enable(cmd->param), NULL, 0);
}

static void f_cmd_stream_disable(t_l502_bf_cmd *cmd) {
    l502_cmd_done(stream_disable(cmd->param), NULL, 0);
}

static void f_cmd_async_out(t_l502_bf_cmd *cmd) {
    int32_t err = 0;
    if (cmd->data_size < 1) {
        err = L502_BF_ERR_INSUF_CMD_DATA;
    } else {
        switch (cmd->param) {
            case L502_BF_CMD_ASYNC_TYPE_DOUT:
                async_dout(cmd->data[0], cmd->data_size >= 2 ? cmd->data[1] : 0);
                break;
            case L502_BF_CMD_ASYNC_TYPE_DAC1:
                async_dac_out(L502_DAC_CH1, cmd->data[0]);
                break;
            case L502_BF_CMD_ASYNC_TYPE_DAC2:
                async_dac_out(L502_DAC_CH2, cmd->data[0]);
                break;
            default:
                err = L502_BF_ERR_INVALID_CMD_PARAMS;
                break;
        }
    }
    l502_cmd_done(err, NULL, 0);
}

static void f_cmd_fpga_reg_wr(t_l502_bf_cmd *cmd) {
    int32_t err = 0;
    if (cmd->data_size < 1) {
        err = L502_BF_ERR_INSUF_CMD_DATA;
    } else if ((cmd->param & 0xFFFF0000) != 0) {
        err = L502_BF_ERR_INVALID_CMD_PARAMS;
    } else {
        fpga_reg_write(cmd->param, cmd->data[0]);
    }
    l502_cmd_done(err, NULL, 0);
}

static void f_cmd_fpga_reg_rd(t_l502_bf_cmd *cmd) {
    int32_t err = 0;
    uint32_t val;
    if ((cmd->param & 0xFFFF0000) != 0) {
        err = L502_BF_ERR_INVALID_CMD_PARAMS;
    } else {
        val = fpga_reg_read(cmd->param);
    }
    l502_cmd_done(err, &val, 1);
}

static void f_cmd_get_out_status(t_l502_bf_cmd *cmd) {
    uint32_t val = sport_tx_out_status();
    l502_cmd_done(0, &val, 1);
}


/*  Установка различных параметров.
    Код параметра определяется по cmd->param, значение берется из cmd->data
    в соответствии с параметром */
static void f_cmd_set_param(t_l502_bf_cmd *cmd) {
    int32_t err = g_mode != L502_BF_MODE_IDLE ? L502_BF_ERR_STREAM_RUNNING :
                  cmd->data_size < 1 ? L502_BF_ERR_INSUF_CMD_DATA : 0;

    if (!err) {
        switch (cmd->param) {
            case L502_BF_PARAM_MODULE_INFO:
                if (cmd->data_size > 0) {
                    g_module_info.devflags = cmd->data[0];
                }
                if (cmd->data_size > 1) {
                    g_module_info.fpga_ver = cmd->data[1] & 0xFFFF;
                    g_module_info.plda_ver = (cmd->data[1]>>16) & 0xFF;
                }
                break;
            case L502_BF_PARAM_LCH_CNT:
                err = params_set_lch_cnt(cmd->data[0]);
                break;
            case L502_BF_PARAM_LCH:
                /* параметры: 0 - индекс, 1 - физ канал, 2 - режим, 3 - диапазон, 4 - усреденение */
                if (cmd->data_size < 5) {
                    err = L502_BF_ERR_INSUF_CMD_DATA;
                } else {
                    err = params_set_lch(cmd->data[0], cmd->data[1], (t_l502_lch_mode)cmd->data[2],
                                        (t_l502_adc_range)cmd->data[3], cmd->data[4],
                                        cmd->data_size>5 ? cmd->data[5] : 0);
                }
                break;
            case L502_BF_PARAM_ADC_FREQ_DIV:
                err = params_set_adc_freq_div(cmd->data[0]);
                break;
            case L502_BF_PARAM_REF_FREQ_SRC:
                err = params_set_ref_freq(cmd->data[0]);
                break;
            case L502_BF_PARAM_ADC_FRAME_DELAY:
                err = params_set_adc_interframe_delay(cmd->data[0]);
                break;
            case L502_BF_PARAM_SYNC_MODE:
                err = params_set_sync_mode((t_l502_sync_mode)cmd->data[0]);
                break;
            case L502_BF_PARAM_SYNC_START_MODE:
                err = params_set_sync_start_mode((t_l502_sync_mode)cmd->data[0]);
                break;
            case L502_BF_PARAM_DIN_FREQ_DIV:
                err = params_set_din_freq_div(cmd->data[0]);
                break;
            case L502_BF_PARAM_DAC_FREQ_DIV:
                err = params_set_dac_freq_div(cmd->data[0]);
                break;
            case L502_BF_PARAM_IN_STEP_SIZE:
                err = sport_in_set_step_size(cmd->data[0]);
                break;
            case L502_BF_PARAM_ADC_COEF:
                if (cmd->data_size < 3) {
                    err = L502_BF_ERR_INSUF_CMD_DATA;
                } else {
                    uint32_t range = cmd->data[0];
                    if (range >= L502_ADC_RANGE_CNT) {
                        err = L502_BF_ERR_INVALID_CMD_PARAMS;
                    } else {
                        fpga_reg_write(f_regaddr_k[range], cmd->data[1]);
                        fpga_reg_write(f_regaddr_offs[range], cmd->data[2]);
                    }
                }
                break;
            case L502_BF_PARAM_DAC_COEF:
                if (cmd->data_size < 3) {
                    err = L502_BF_ERR_INSUF_CMD_DATA;
                } else {
                    uint32_t ch = cmd->data[0];
                    if (ch >= L502_DAC_CH_CNT) {
                        err = L502_BF_ERR_INVALID_CMD_PARAMS;
                    } else {
                        float* pk = (float*)&cmd->data[1];
                        float* po = (float*)&cmd->data[2];
                        g_module_info.dac_cbr[ch].k = *pk;
                        g_module_info.dac_cbr[ch].offs = *po;
                    }
                }
                break;
            default:
                err = L502_BF_ERR_INVALID_CMD_PARAMS;
                break;
        }
    }
    l502_cmd_done(err, NULL, 0);
}


static void f_cmd_get_param(t_l502_bf_cmd *cmd) {
    int32_t err = 0;
    uint32_t ret_size = 0;


    switch (cmd->param) {
        case L502_BF_PARAM_FIRM_VERSION:
            cmd->data[0] = L502_BF_FIRM_VERSION;
            cmd->data[1] = L502_BF_FIRM_FEATURES;
            ret_size = 2;
            break;
        case L502_BF_PARAM_STREAM_MODE:
            cmd->data[0] = g_mode;
            ret_size = 1;
            break;
        case L502_BF_PARAM_ENABLED_STREAMS:
            cmd->data[0] = g_streams;
            ret_size = 1;
            break;
        case L502_BF_PARAM_IN_BUF_SIZE:
            cmd->data[0] = sport_in_buffer_size();
            ret_size = 1;
            break;
        case L502_BF_PARAM_LCH_CNT:
            cmd->data[0] = g_set.lch_cnt;
            ret_size = 1;
            break;
        case L502_BF_PARAM_LCH:
            if (cmd->data_size < 1) {
                err = L502_BF_ERR_INSUF_CMD_DATA;
            } else {
                uint32_t index = cmd->data[0];
                if (index >= L502_LTABLE_MAX_CH_CNT) {
                    err = L502_BF_ERR_INVALID_CMD_PARAMS;
                } else {
                    cmd->data[1] = g_set.lch[index].phy_ch;
                    cmd->data[2] = g_set.lch[index].mode;
                    cmd->data[3] = g_set.lch[index].range;
                    cmd->data[4] = g_set.lch[index].avg;
                    cmd->data[5] = g_set.lch[index].flags;
                    ret_size = 6;
                }
            }
            break;
        case L502_BF_PARAM_ADC_FREQ_DIV:
            cmd->data[0] = g_set.adc_freq_div;
            ret_size = 1;
            break;
        case L502_BF_PARAM_REF_FREQ_SRC:
            cmd->data[0] = g_set.ref_freq;
            ret_size = 1;
            break;
        case L502_BF_PARAM_ADC_FRAME_DELAY:
            cmd->data[0] = g_set.adc_frame_delay;
            ret_size = 1;
            break;
        case L502_BF_PARAM_SYNC_MODE:
            cmd->data[0] = g_set.sync_mode;
            ret_size = 1;
            break;
        case L502_BF_PARAM_SYNC_START_MODE:
            cmd->data[0] = g_set.sync_start_mode;
            ret_size = 1;
            break;
        case L502_BF_PARAM_DIN_FREQ_DIV:
            cmd->data[0] = g_set.din_freq_div;
            ret_size = 1;
            break;
        case L502_BF_PARAM_DAC_FREQ_DIV:
            cmd->data[0] = g_set.out_freq_div;
            ret_size = 1;
            break;
        default:
            err = L502_BF_ERR_INVALID_CMD_PARAMS;
            break;
    }

    l502_cmd_done(err, cmd->data, ret_size);
}

/** @} */













