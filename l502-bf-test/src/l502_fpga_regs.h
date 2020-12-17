#ifndef L5XX_REGS_H
#define L5XX_REGS_H

#define L583_BF_ADDR_ENDPROG           (0xFFFFFFFCUL)


#define L502_MAX_PAGES_CNT  252

#define L502_BF_SDRAM_SIZE (32UL*1024*1024)

#define L502_BF_MEMADDR_CMD      0xFF800800


#define L502_BF_CMD_READ          0x0001
#define L502_BF_CMD_WRITE         0x0002
#define L502_BF_CMD_HIRQ          0x0004
#define L502_BF_CMD_HDMA_RST      0x0008


/********************* Адреса регистров блока EEPROM *************************/
#define L502_REGS_EEPROM_BLOCK                    0x0100

#define L502_REGS_EEPROM_SET_RD_ADDR             (L502_REGS_EEPROM_BLOCK + 0)
#define L502_REGS_EEPROM_RD_DWORD                (L502_REGS_EEPROM_BLOCK + 1)
#define L502_REGS_EEPROM_RD_STATUS               (L502_REGS_EEPROM_BLOCK + 2)
#define L502_REGS_EEPROM_WR_STATUS_EN            (L502_REGS_EEPROM_BLOCK + 3)
#define L502_REGS_EEPROM_WR_EN                   (L502_REGS_EEPROM_BLOCK + 4)
#define L502_REGS_EEPROM_WR_DIS                  (L502_REGS_EEPROM_BLOCK + 5)
#define L502_REGS_EEPROM_WR_STATUS               (L502_REGS_EEPROM_BLOCK + 6)
#define L502_REGS_EEPROM_ERASE_4K                (L502_REGS_EEPROM_BLOCK + 7)
#define L502_REGS_EEPROM_ERASE_64K               (L502_REGS_EEPROM_BLOCK + 8)
#define L502_REGS_EEPROM_WR_BYTE                 (L502_REGS_EEPROM_BLOCK + 9)
#define L502_REGS_EEPROM_HARD_WR_STATUS_EN       (L502_REGS_EEPROM_BLOCK + 0xF)
#define L502_REGS_HARD_ID                        (L502_REGS_EEPROM_BLOCK + 0xA)
#define L502_REGS_JEDEC_RD_ID                    (L502_REGS_EEPROM_BLOCK + 0xB)

/********************* Адреса регистров с отладочной информацией **************/
#define L502_REGS_DBG_BLOCK                      0x0140
#define L502_REGS_DBG_EVENTS                     (L502_REGS_DBG_BLOCK + 0)
#define L502_REGS_DBG_LAST_ABORT_ADDR            (L502_REGS_DBG_BLOCK + 8)
#define L502_REGS_DBG_LAST_NACK_ADDR             (L502_REGS_DBG_BLOCK + 9)
#define L502_REGS_DBG_LINK_REPLAY_CNT            (L502_REGS_DBG_BLOCK + 10)

/********************* Адреса регистров блока IOHARD **************************/
#define L502_REGS_IOHARD_BLOCK                    0x0200
//Адрес Control Table
#define L502_REGS_IOHARD_LTABLE                   (L502_REGS_IOHARD_BLOCK+0)
#define L502_REGS_IOHARD_LTABLE_MAX_SIZE          0x100 // Максимальный размер Control Table

#define L502_REGS_IOHARD_LCH_CNT                  (L502_REGS_IOHARD_BLOCK+0x100)
#define L502_REGS_IOHARD_ADC_FREQ_DIV             (L502_REGS_IOHARD_BLOCK+0x102)
#define L502_REGS_IOHARD_ADC_FRAME_DELAY          (L502_REGS_IOHARD_BLOCK+0x104)
#define L502_REGS_IOHARD_DIGIN_FREQ_DIV           (L502_REGS_IOHARD_BLOCK+0x106)
#define L502_REGS_IOHARD_IO_MODE                  (L502_REGS_IOHARD_BLOCK+0x108)
#define L502_REGS_IOHARD_GO_SYNC_IO               (L502_REGS_IOHARD_BLOCK+0x10A)
#define L502_REGS_IOHARD_PRELOAD_ADC              (L502_REGS_IOHARD_BLOCK+0x10C)
#define L502_REGS_IOHARD_ASYNC_OUT                (L502_REGS_IOHARD_BLOCK+0x112)
#define L502_REGS_IOHARD_LED                      (L502_REGS_IOHARD_BLOCK+0x114)
#define L502_REGS_IOHARD_DIGIN_PULLUP             (L502_REGS_IOHARD_BLOCK+0x116)
#define L502_REGS_IOHARD_OUTSWAP_BFCTL            (L502_REGS_IOHARD_BLOCK+0x118)
#define L502_REGS_IOHARD_OUTSWAP_ERROR            (L502_REGS_IOHARD_BLOCK+0x120)



/********************* Адреса регистров блока IOARITH **************************/
#define L502_REGS_IOARITH_BLOCK                    0x0400
#define L502_REGS_IOARITH_B10                      L502_REGS_IOARITH_BLOCK
#define L502_REGS_IOARITH_B5                       (L502_REGS_IOARITH_BLOCK+0x01)
#define L502_REGS_IOARITH_B2                       (L502_REGS_IOARITH_BLOCK+0x02)
#define L502_REGS_IOARITH_B1                       (L502_REGS_IOARITH_BLOCK+0x03)
#define L502_REGS_IOARITH_B05                      (L502_REGS_IOARITH_BLOCK+0x04)
#define L502_REGS_IOARITH_B02                      (L502_REGS_IOARITH_BLOCK+0x05)
#define L502_REGS_IOARITH_K10                      (L502_REGS_IOARITH_BLOCK+0x08)
#define L502_REGS_IOARITH_K5                       (L502_REGS_IOARITH_BLOCK+0x09)
#define L502_REGS_IOARITH_K2                       (L502_REGS_IOARITH_BLOCK+0x0A)
#define L502_REGS_IOARITH_K1                       (L502_REGS_IOARITH_BLOCK+0x0B)
#define L502_REGS_IOARITH_K05                      (L502_REGS_IOARITH_BLOCK+0x0C)
#define L502_REGS_IOARITH_K02                      (L502_REGS_IOARITH_BLOCK+0x0D)
#define L502_REGS_IOARITH_ADC_FREQ_DIV             (L502_REGS_IOARITH_BLOCK+0x12)
#define L502_REGS_IOARITH_IN_STREAM_ENABLE         (L502_REGS_IOARITH_BLOCK+0x19)
#define L502_REGS_IOARITH_DIN_ASYNC                (L502_REGS_IOARITH_BLOCK+0x1A)


/********************* Адреса регистров блока управления BlackFin'ом **********/
#define L502_REGS_BF_CTL_BLOCK               0
#define L502_REGS_BF_CTL                     (L502_REGS_BF_CTL_BLOCK+0)
#define L502_REGS_BF_CMD                     (L502_REGS_BF_CTL_BLOCK+1)
#define L502_REGS_BF_STATUS                  (L502_REGS_BF_CTL_BLOCK+2)
#define L502_REGS_BF_IRQ                     (L502_REGS_BF_CTL_BLOCK+3)
#define L502_REGS_BF_IRQ_EN                  (L502_REGS_BF_CTL_BLOCK+4)
#define L502_REGS_BF_REQ_ADDR                (L502_REGS_BF_CTL_BLOCK+5)
#define L502_REGS_BF_REQ_SIZE                (L502_REGS_BF_CTL_BLOCK+6)
#define L502_REGS_BF_REQ_DATA                (L502_REGS_BF_CTL_BLOCK+128)

#define L502_BF_REQ_DATA_SIZE_MAX            128
#define L502_BF_REQ_DATA_SIZE_MIN              8


/********************* Адреса регистров блока DMA *****************************/
#define L502_REGS_DMA_CTL_BLOCK              0x700
#define L502_REGS_DMA_CAP                    (L502_REGS_DMA_CTL_BLOCK)
#define L502_REGS_DMA_EN                     (L502_REGS_DMA_CTL_BLOCK+1)
#define L502_REGS_DMA_DIS                    (L502_REGS_DMA_CTL_BLOCK+2)
#define L502_REGS_DMA_RST                    (L502_REGS_DMA_CTL_BLOCK+3)
#define L502_REGS_DMA_IRQ                    (L502_REGS_DMA_CTL_BLOCK+4)
#define L502_REGS_DMA_IRQ_EN                 (L502_REGS_DMA_CTL_BLOCK+5)
#define L502_REGS_DMA_IRQ_DIS                (L502_REGS_DMA_CTL_BLOCK+6)

#define L502_REGS_DMA_CH_PARAMS_SIZE   (16 + L502_MAX_PAGES_CNT*4)
#define L502_DMA_CHNUM_IN         0
#define L502_DMA_CHNUM_OUT        1

/* номер регистра, с которого начинаются параметры канала DMA */
#define L502_REGS_DMA_CH_PARAMS(ch)         (0x800 + L502_REGS_DMA_CH_PARAMS_SIZE*ch)
#define L502_REGS_DMA_CH_CTL(ch)            (L502_REGS_DMA_CH_PARAMS(ch) + 0)
#define L502_REGS_DMA_CH_CMP_CNTR(ch)       (L502_REGS_DMA_CH_PARAMS(ch) + 1)
#define L502_REGS_DMA_CH_CUR_CNTR(ch)       (L502_REGS_DMA_CH_PARAMS(ch) + 2)
#define L502_REGS_DMA_CH_CUR_POS(ch)        (L502_REGS_DMA_CH_PARAMS(ch) + 3)
#define L502_REGS_DMA_CH_PC_POS(ch)         (L502_REGS_DMA_CH_PARAMS(ch) + 4)


/* адреса для регистров страниц DMA АЦП */
#define L502_REGS_DMA_CH_PAGES(ch)          (L502_REGS_DMA_CH_PARAMS(ch) + 16)
#define L502_REGS_DMA_CH_PAGE_ADDRL(ch,n)   (L502_REGS_DMA_CH_PAGES(ch) + 4*(n))
#define L502_REGS_DMA_CH_PAGE_ADDRH(ch,n)   (L502_REGS_DMA_CH_PAGES(ch) + 4*(n)+1)
#define L502_REGS_DMA_CH_PAGE_LEN(ch,n)     (L502_REGS_DMA_CH_PAGES(ch) + 4*(n)+2)





#define L502_REGBIT_BF_STATUS_HWAIT_Pos      0
#define L502_REGBIT_BF_STATUS_HWAIT_Msk      (1UL << L502_REGBIT_BF_STATUS_HWAIT_Pos)

#define L502_REGBIT_BF_STATUS_BUSY_Pos       1
#define L502_REGBIT_BF_STATUS_BUSY_Msk       (1UL << L502_REGBIT_BF_STATUS_BUSY_Pos)


/* описание отдельных битов регистров */
#define L502_REGBIT_DMA_CTL_PACK_SIZE_Pos    0
#define L502_REGBIT_DMA_CTL_PACK_SIZE_Msk    (0xFFUL << L502_REGBIT_DMA_CTL_PACK_SIZE_Pos)

#define L502_REGBIT_DMA_CTL_PAGE_CNT_Pos     16
#define L502_REGBIT_DMA_CTL_PAGE_CNT_Msk     (0xFFUL << L502_REGBIT_DMA_CTL_PAGE_CNT_Pos)

#define L502_REGBIT_DMA_CTL_AUTOSTOP_Pos     31
#define L502_REGBIT_DMA_CTL_AUTOSTOP_Msk     (0x1UL << L502_REGBIT_DMA_CTL_AUTOSTOP_Pos)

#define L502_REGBIT_DMA_CTL_PC_WAIT_Pos     30
#define L502_REGBIT_DMA_CTL_PC_WAIT_Msk     (0x1UL << L502_REGBIT_DMA_CTL_PC_WAIT_Pos)

#define L502_REGBIT_DMA_CH_ADC_Pos           0
#define L502_REGBIT_DMA_CH_ADC_Msk           (0x1UL << L502_REGBIT_DMA_CH_ADC_Pos)

#define L502_REGBIT_DMA_CH_DAC_Pos           1
#define L502_REGBIT_DMA_CH_DAC_Msk           (0x1UL << L502_REGBIT_DMA_CH_DAC_Pos)



#define L502_REGBIT_BF_CTL_BF_RESET_Pos       1
#define L502_REGBIT_BF_CTL_BF_RESET_Msk       (0x1UL << L502_REGBIT_BF_CTL_BF_RESET_Pos)


#define L502_REGBIT_BF_CTL_HOST_WAIT_Pos      3
#define L502_REGBIT_BF_CTL_HOST_WAIT_Msk      (0x1UL << L502_REGBIT_BF_CTL_HOST_WAIT_Pos)

#define L502_REGBIT_BF_CTL_DSP_MODE_Pos       4
#define L502_REGBIT_BF_CTL_DSP_MODE_Msk       (0x1UL << L502_REGBIT_BF_CTL_DSP_MODE_Pos)

#define L502_REGBIT_BF_CTL_DBG_MODE_Pos       5
#define L502_REGBIT_BF_CTL_DBG_MODE_Msk       (0x1UL << L502_REGBIT_BF_CTL_DBG_MODE_Pos)

#define L502_REGBIT_BF_CTL_CLK_DIV_Pos        8
#define L502_REGBIT_BF_CTL_CLK_DIV_Msk        (0xFUL << L502_REGBIT_BF_CTL_CLK_DIV_Pos)

#define L502_REGBIT_DMA_CURPOS_PAGE_Pos       24
#define L502_REGBIT_DMA_CURPOS_PAGE_Msk       (0xFFUL << L502_REGBIT_DMA_CURPOS_PAGE_Pos)

#define L502_REGBIT_ADC_SLV_CLK_LOCK_Pos      31
#define L502_REGBIT_ADC_SLV_CLK_LOCK_Msk      (0x1UL << L502_REGBIT_ADC_SLV_CLK_LOCK_Pos)



#define L502_REGBIT_IOHARD_OUT_SWAP_Pos     0
#define L502_REGBIT_IOHARD_OUT_SWAP_Msk     (0x1UL << L502_REGBIT_IOHARD_OUT_SWAP_Pos)

#define L502_REGBIT_IOHARD_OUT_TFS_EN_Pos   1
#define L502_REGBIT_IOHARD_OUT_TFS_EN_Msk   (0x1UL << L502_REGBIT_IOHARD_OUT_TFS_EN_Pos)

#define L502_REGBIT_IOHARD_OUT_RING_Pos     2
#define L502_REGBIT_IOHARD_OUT_RING_Msk     (0x1UL << L502_REGBIT_IOHARD_OUT_RING_Pos)

#define L502_REGBIT_IOHARD_OUT_RFS_EN_Pos   3
#define L502_REGBIT_IOHARD_OUT_RFS_EN_Msk   (0x1UL << L502_REGBIT_IOHARD_OUT_RFS_EN_Pos)








#define L502_REGBIT_DMA_IRQ_STEP_Msk(ch)     (1UL << ch)
#define L502_REGBIT_DMA_IRQ_PAGE_Msk(ch)     (1UL << (ch+8))



#endif // L5XX_REGS_H
