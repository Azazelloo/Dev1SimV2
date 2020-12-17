/** @defgroup streams Управление потоками сбора данных */

/***************************************************************************//**
    @addtogroup streams
    @{
    @file l502_stream.h

    Файл содержит описания функций для работы с интерфейсом HostDMA
 ******************************************************************************/

#ifndef L502_STREAM_H_
#define L502_STREAM_H_

#include <stdint.h>

/** Состояние потока на ввод */
typedef enum {
    IN_STREAM_STOP = 0,     /**< Поток на ввод остановлен */
    IN_STREAM_RUN = 2,      /**< Поток на ввод в рабочем режиме */
    IN_STREAM_OV_ALERT = 4, /**< Произошло переполнение - нужно сообщить о ошибке */
    IN_STREAM_ERR= 3        /**< Поток остановлен из-за ошибки/переполнения */
} t_in_stream_state;

/** Состояние потока на вывод */
typedef enum {
    OUT_STREAM_STOP = 0,    /**< Поток на вывод остановлен */
    OUT_STREAM_PRELOAD = 1, /**< Идет предзагрузка данных - данные принимаются
                                от ПК, но еще не запущен синхронных сбор/выдача */
    OUT_STREAM_RUN = 2,     /** Поток запущен на выдачу */
    OUT_STREAM_ERR = 3,     /** Поток остановлен по ошибке (сейчас не используется) */
    OUT_STREAM_CYCLE = 4    /** Режим циклического буфера (сейчас не реализован) */
} t_out_stream_state;


/** Состояние потока на ввод */
extern t_in_stream_state  g_stream_in_state;
/** Состояние потока на вывод */
extern t_out_stream_state g_stream_out_state;
/** Режим работы прошивки из #t_l502_bf_mode */
extern volatile int g_mode;
/** Флаги, обозначающие какие потоки разрешены */
extern int g_streams;


int32_t streams_start(void);
int32_t streams_stop(void);

int32_t stream_enable(uint32_t streams);
int32_t stream_disable(uint32_t streams);

int32_t stream_out_preload(void);


void stream_in_buf_free(uint32_t size);
void stream_out_buf_free(uint32_t size);


uint32_t sport_in_buffer_size(void);
int32_t sport_in_set_step_size(uint32_t size);



#endif

/** @} */
