#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"
#include "flash.h"
#include "sha256.h"
#include "sysctl.h"

#include "board.h"
#include "face_lib.h"

volatile face_save_info_t g_face_save_info;
face_info_t g_face_info;
volatile static uint8_t uid_table[FACE_DATA_MAX_COUNT][UID_LEN];

static int get_face_id(void)
{
    int ret = -1;
    int i;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if(((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1) == 0)
        {
            ret = i;
            break;
        }
    }

    if(i >= FACE_DATA_MAX_COUNT)
    {
        printf("get_face_id:> Err too many data\n");
        return -1;
    }
    return ret;
}

static void flash_init_uid_table(uint8_t init_flag)
{
    int i;
    uint32_t uid_addr;
    if(!init_flag)
    {
        for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
        {
            uid_addr = FACE_DATA_ADDERSS + i * sizeof(face_info_t) + 4;
            w25qxx_read_data(uid_addr, uid_table[i], UID_LEN);
        }
    } else
    {
        memset(uid_table, 0, UID_LEN * FACE_DATA_MAX_COUNT);
    }
    return;
}

//高四位置0xF
uint32_t flash_get_id_by_uid(uint8_t *uid)
{
    uint32_t i, j;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if(memcmp(uid_table[i], uid, UID_LEN) == 0)
            {
                printf("find uid:");
                for(uint8_t j = 0; j < UID_LEN; j++)
                    printf("%02x ", uid_table[i][j]);
                return i | 0xF0000000;
            }
        }
    }
    return 0;
}

int flash_delete_face_info(uint32_t id)
{
    if(g_face_save_info.number == 0)
    {
        printf("del pic, no pic\n");
        return -1;
    }
    g_face_save_info.face_info_index[id / 32] &= ~(1 << (id % 32));
    g_face_save_info.number--;
    w25qxx_write_data(DATA_ADDRESS, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
    memset(uid_table[id], 0, UID_LEN);
    return 0;
}

int flash_delete_face_all(void)
{
    g_face_save_info.number = 0;
    memset((void *)g_face_save_info.face_info_index, 0, sizeof(g_face_save_info.face_info_index));
    w25qxx_write_data(DATA_ADDRESS, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
    memset(uid_table, 0, UID_LEN * FACE_DATA_MAX_COUNT);
    return 0;
}

int flash_get_face_info(face_info_t *face_info, uint32_t id)
{
    uint32_t image_address = FACE_DATA_ADDERSS + id * (sizeof(face_info_t));
    w25qxx_read_data(image_address, (uint8_t *)face_info, sizeof(face_info_t));
    if(face_info->index != id)
    {
        printf("flash dirty! oft=%x, info.index %d != id %d\r\n", image_address, face_info->index, id);
        return -1;
    }
    return 0;
}

int flash_save_face_info(uint8_t *image, float *features, uint8_t *uid, uint32_t valid, char *name, char *note, uint8_t *ret_uid)
{
    int face_id = get_face_id();
    if(face_id >= FACE_DATA_MAX_COUNT || face_id < 0)
    {
        printf("get_face_id err\n");
        return -1;
    }
    printf("Save face_id is %d\n", face_id);
    memcpy(g_face_info.info, features, sizeof(g_face_info.info));
    g_face_info.index = face_id; //record to verify flash content

    if(uid == NULL)
    {
        uint64_t tmp = read_cycle();
        memcpy(uid_table[face_id], (uint8_t *)&tmp, 8);
        memset(uid_table[face_id] + 8, 0, 8);
        if(ret_uid)
            memcpy(ret_uid, uid_table[face_id], UID_LEN);
    } else
    {
        memcpy(uid_table[face_id], uid, UID_LEN);
    }
    memcpy(g_face_info.uid, uid_table[face_id], UID_LEN);

    g_face_info.valid = valid; //pass permit

    if(name == NULL)
        strncpy(g_face_info.name, "default", NAME_LEN);
    else
        strncpy(g_face_info.name, name, NAME_LEN - 1);

    if(note == NULL)
        strncpy(g_face_info.note, "null", NOTE_LEN - 1);
    else
        strncpy(g_face_info.note, note, NOTE_LEN - 1);

    w25qxx_write_data(FACE_DATA_ADDERSS + face_id * (sizeof(face_info_t)), (uint8_t *)&g_face_info, sizeof(face_info_t));
    g_face_save_info.number++;
    g_face_save_info.face_info_index[face_id / 32] |= (1 << (face_id % 32));
    w25qxx_write_data(DATA_ADDRESS, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));

    return 0;
}

int flash_get_saved_uid_feature(uint32_t index, uint8_t uid[16 + 1], float *feature)
{
    uint32_t i;
    uint32_t cnt = 0;
    face_info_t v_face_info;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if(cnt == index)
            {
                flash_get_face_info(&v_face_info, i);
                if(feature)
                    memcpy(feature, v_face_info.info, sizeof(v_face_info.info));
                if(uid)
                    memcpy(uid, v_face_info.uid, sizeof(v_face_info.uid));
                break;
            } else
            {
                cnt++;
            }
        }
    }

    if(cnt == index)
    {
        return 0;
    } else
    {
        return -1;
    }
}

uint32_t flash_get_wdt_reboot_count(void)
{
    return 0;
    // uint32_t v_wdt_reboot_count;
    // w25qxx_read_data(DATA_REBOOT_COUNT, (uint8_t *)&v_wdt_reboot_count, 4);
    // return v_wdt_reboot_count;
}

int flash_get_saved_feature_number(void)
{
    return g_face_save_info.number;
}

static void flash_check_wdt_reboot_count(void)
{
    uint32_t v_wdt_reboot_count = flash_get_wdt_reboot_count();
    sysctl_reset_enum_status_t v_reset_status = sysctl_get_reset_status();
    if(v_reset_status == SYSCTL_RESET_STATUS_WDT0 || v_reset_status == SYSCTL_RESET_STATUS_WDT1)
    {
        printf("wdt reboot!\n");
        v_wdt_reboot_count++;
        // w25qxx_write_data(DATA_REBOOT_COUNT, (uint8_t *)&v_wdt_reboot_count, 4);
    } else if(v_wdt_reboot_count != 0)
    {
        v_wdt_reboot_count = 0;
        // w25qxx_write_data(DATA_REBOOT_COUNT, (uint8_t *)&v_wdt_reboot_count, 4);
    }
}

void flash_init(void)
{
    /* spi flash init */
    w25qxx_init(3, 0, 60000000);
    
    uint8_t manuf_id, device_id;
    w25qxx_read_id(&manuf_id, &device_id);
    printf("manuf_id:0x%02x, device_id:0x%02x\n", manuf_id, device_id);
    if((manuf_id != 0xEF && manuf_id != 0xC8) || (device_id != 0x17 && device_id != 0x16))
    {
        printf("manuf_id:0x%02x, device_id:0x%02x\n", manuf_id, device_id);
        return;
    }
    
    w25qxx_read_data(DATA_ADDRESS, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
    
    if(g_face_save_info.header == FACE_HEADER)
    {
        printf("The header ok\r\n");
        uint32_t v_num = g_face_save_info.number;
        printf("there is %d img\r\n", v_num);
        v_num = 0;

        for(uint32_t i = 0; i < FACE_DATA_MAX_COUNT; i++)
        {
            if(g_face_save_info.face_info_index[i / 32] >> (i % 32) & 0x1)
            {
                v_num++;
            }
        }

        if(v_num != g_face_save_info.number)
        {
            printf("err:> index is %d, but saved is %d\n", v_num, g_face_save_info.number);
            g_face_save_info.number = v_num;
        }

        if(v_num >= FACE_DATA_MAX_COUNT)
        {
            printf("ERR, too many pic\n");
        }
        
        flash_init_uid_table(0);
         //4.7ms
    } else
    {
        printf("No header\n");
        g_face_save_info.header = FACE_HEADER;
        g_face_save_info.number = 0;
        memset((void *)g_face_save_info.face_info_index, 0, sizeof(g_face_save_info.face_info_index));
        w25qxx_write_data(DATA_ADDRESS, (uint8_t *)&g_face_save_info, sizeof(face_save_info_t));
        
        flash_init_uid_table(1);
         //4.7ms
    }
    flash_check_wdt_reboot_count();
    
    return;
}

int flash_get_saved_feature(float *feature, uint32_t index)
{
    uint32_t i;
    uint32_t cnt = 0;
    face_info_t v_face_info;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if(cnt == index)
            {
                flash_get_face_info(&v_face_info, i);
                memcpy(feature, v_face_info.info, sizeof(v_face_info.info));
                break;
            } else
            {
                cnt++;
            }
        }
    }
    return 0;
}

//1 success
//2 no feature store
//0 error
int flash_get_saved_feature_sha256(uint8_t sha256[32 + 1])
{
    uint8_t cal_sha256[32 + 1];
    static uint8_t flag = 0;
    uint8_t feature[FEATURE_DIMENSION * 4 + 1], tmp[FEATURE_DIMENSION * 4 + 1];
    uint32_t i = 0, cnt = 0;
    face_info_t v_face_info;

    flag = 0;
    cnt = 0;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if(flash_get_face_info(&v_face_info, i) == 0)
            {
                memcpy(tmp, v_face_info.info, (FEATURE_DIMENSION * 4));
                if(flag == 0)
                {
                    memcpy(feature, tmp, (FEATURE_DIMENSION * 4));
                    flag = 1;
                } else
                {
                    for(uint16_t j = 0; j < (FEATURE_DIMENSION * 4); j++)
                    {
                        feature[j] ^= tmp[j];
                    }
                }
            } else
            {
                printf("flash_get_face_info failed\r\n");
                return 0;
            }
            cnt++;
        }
    }

    if(cnt == 0)
    {
        printf("flash_get_saved_feature_sha256 cnt == 0\r\n");
        memset(sha256, 0xff, 32);
        return 1;
    }

    sha256_hard_calculate(feature, (FEATURE_DIMENSION * 4), cal_sha256);
    memcpy(sha256, cal_sha256, 32);
#if 0
    printf("feature: \r\n");
    for (uint16_t i = 0; i < FEATURE_DIMENSION * 4; i++)
    {
        printf("%02x ", *(feature + i));
        if (i % 4 == 4)
            printf("\r\n");
    }
    printf("\r\n************************************\r\n");

    for (uint16_t i = 0; i < 32; i++)
    {
        printf("%02X", sha256[i]);
    }
#endif

    return 1;
}

int flash_get_saved_faceinfo(face_info_t *info, uint32_t index)
{
    uint32_t i;
    uint32_t cnt = 0;
    face_info_t v_face_info;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            if(cnt == index)
            {
                flash_get_face_info(info, i);
                break;
            } else
            {
                cnt++;
            }
        }
    }
    return (i < FACE_DATA_MAX_COUNT) ? 0 : -1;
}

#if 0
float calCosinDistance(float *faceFeature0P, float *faceFeature1P, int featureLen)
{
    // definitiion
    float coorFeature = 0;
    float sumFeature0 = 0;
    float sumFeature1 = 0;

    // calculate the sum square
    for (int fIdx = 0; fIdx < featureLen; fIdx++)
    {
        float featureVal0 = *(faceFeature0P + fIdx);
        float featureVal1 = *(faceFeature1P + fIdx);

        sumFeature0 += featureVal0 * featureVal0;
        sumFeature1 += featureVal1 * featureVal1;
        coorFeature += featureVal0 * featureVal1;
    }

    // cosin distance
    return (0.5 + 0.5 * coorFeature / sqrt(sumFeature0 * sumFeature1))*100;
}

int calulate_score(float *features, float *score)
{
    int i;
    int v_id = -1;
    face_info_t v_face_info;
    float v_score;
    float v_score_max = 0.0;
    for(i = 0; i < FACE_DATA_MAX_COUNT; i++)
    {
        if((g_face_save_info.face_info_index[i / 32] >> (i % 32)) & 0x1)
        {
            flash_get_face_info(&v_face_info, i);
            v_score = calCosinDistance(features, v_face_info.info, 196);
            if(v_score > v_score_max)
            {
                v_score_max = v_score;
                v_id = i;
            }
        }
    }
    *score = v_score_max;
    return v_id;
}
#endif

// BOARD_CFG_ADDR
// BOARD_CFG_LEN
uint8_t flash_load_cfg(board_cfg_t *cfg)
{
    w25qxx_status_t stat;
    uint8_t sha256[32];

    stat = w25qxx_read_data(BOARD_CFG_ADDR, (uint8_t *)cfg, sizeof(board_cfg_t));

    // printf("sizeof(board_cfg_t):%ld\r\n", sizeof(board_cfg_t));

    if(cfg->header == CFG_HEADER && stat == W25QXX_OK)
    {
        sha256_hard_calculate((uint8_t *)cfg, sizeof(board_cfg_t) - 32, sha256); //34是因为结构体8字节对齐，导致尾部多了2字节
        if(memcmp(cfg->cfg_sha256, sha256, 32) == 0)
        {
            cfg->cfg_right_flag = 1;
            return 1;
        } else
        {
            return 0;
        }
    } else
    {
        memset(cfg, 0, sizeof(board_cfg_t));
        return 0;
    }

    return 0;
}

uint8_t flash_save_cfg(board_cfg_t *cfg)
{
    w25qxx_status_t stat;

    cfg->cfg_right_flag = 0;

    sha256_hard_calculate((uint8_t *)cfg, sizeof(board_cfg_t) - 32, cfg->cfg_sha256);

    for(uint8_t i = 0; i < 32; i++)
    {
        printf("%02X", cfg->cfg_sha256[i]);
    }
    printf("\r\n");

    stat = w25qxx_write_data(BOARD_CFG_ADDR, (uint8_t *)cfg, sizeof(board_cfg_t));

    return (stat == W25QXX_OK) ? 1 : 0;
}

uint8_t flash_cfg_print(board_cfg_t *cfg)
{
    printf("uart_baud:%d\r\n", cfg->uart_baud);
    printf("recong_out_feature:%d\r\n", cfg->recong_out_feature);
    printf("relay_open_in_s:%d\r\n", cfg->relay_open_in_s);
    printf("pkt_fix:%d\r\n", cfg->pkt_sum_header);
    printf("auto_out_feature:%d\r\n", cfg->auto_out_feature);
    printf("out_interval_in_ms:%d\r\n", cfg->out_interval_in_ms);
    printf("lcd_cam_dir:%08X\r\n", cfg->lcd_cam_dir);
    printf("face_gate:%d\r\n", cfg->face_gate);
    printf("port_cfg:%08X\r\n", cfg->port_cfg);
    printf("key_relay_pin_cfg:%08X\r\n", cfg->key_relay_pin_cfg);
}

uint8_t flash_cfg_set_default(board_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(board_cfg_t));
    cfg->header = CFG_HEADER;
    cfg->cfg_right_flag = 0;

    cfg->uart_baud = 115200;
    cfg->recong_out_feature = 0;
    cfg->relay_open_in_s = 1;
    cfg->pkt_sum_header = 0;
    cfg->auto_out_feature = 0;
    cfg->out_interval_in_ms = 500;
    cfg->lcd_cam_dir = 0x08; //TODO: 默认值??
    cfg->face_gate = FACE_RECGONITION_THRESHOLD;
    cfg->port_cfg = (uint32_t)((PROTOCOL_PORT_TX_PIN << 24) | (PROTOCOL_PORT_RX_PIN << 16) |
                               (DEBUE_TX_PIN << 8) | (DEBUE_RX_PIN));
    cfg->key_relay_pin_cfg = (uint32_t)((CONFIG_KEY_DIR << 24) | (KEY_PIN << 16) |
                                (RELAY_HIGH_PIN << 8) | (RELAY_LOWX_PIN));

}