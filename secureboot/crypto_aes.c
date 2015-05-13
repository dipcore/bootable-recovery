//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#include "crypto_aes.h"
#include "crypto_common.h"

#if defined(BUILD_WITH_SECURE_AESDMA)
//call AESDMA
#include <drvAESDMA.h>
#include <MsOS.h>
#include <drvSYS.h>
#include <mmap.h>

static MS_U32 stHwAesPA = 0;
static MS_U32 stHwAesVA = 0;
static MS_U32 stHwAesLen = 0;
static int g_initflag = -1;
#define BIT(bits)                   (1<<bits)

typedef struct
{
    MS_U32 pBuf;
    MS_U32 *pKey;
    MS_U32 *pIV;
    MS_U32 u32Len;
    MS_BOOL bIsDecrypt;
    DrvAESDMA_CipherMode eMode;
}AES_ParamStruct;

static int HWAESAddrInit(void)
{
    if (-1 == g_initflag) {
        mmap_info_t* minfo = NULL;
        if (mmap_init() != 0) {
            return -1;
        }
        MDrv_SYS_GlobalInit();
        // the area in mmap locate in miu0
        minfo = mmap_get_info("E_MMAP_ID_HW_AES_BUF");
        if (minfo == NULL) {
            DEBUG("mmap_get_info E_MMAP_ID_HW_AES_BUF fail.");
            return -1;
        }
        if (minfo->size != 0) {
            if (!MsOS_MPool_Mapping(0, minfo->addr, minfo->size, 1)) {
                DEBUG("mapping read buf failed, %lx,%ld ", minfo->addr, minfo->size);
                return -1;
            }
            stHwAesPA = minfo->addr;
            stHwAesLen = minfo->size;
            stHwAesVA = MsOS_MPool_PA2KSEG1(stHwAesPA);
            DEBUG("stHwAesPA : 0x%x,stHwAesLen : 0x%x,stHwAesVA : 0x%x\n",(unsigned int)stHwAesPA,(unsigned int)stHwAesLen,(unsigned int)stHwAesVA);
        }
        g_initflag = 0;
    }
    return 0;
}

static int CommonAES128(AES_ParamStruct *AESPara)
{
    DRVAESDMA_RESULT result = DRVAESDMA_OK;
    MS_U32 u32AESBuf;
    DEBUG("IN\n");
    if((AESPara->pBuf == 0)) {
        DEBUG("The input parameters are not correct\n");
        return DRVAESDMA_INVALID_PARAM;
    }
    if (-1 == HWAESAddrInit()) {
        DEBUG("init HW AES address error\n");
        return -1;
    }
    // check memory size
    if (stHwAesLen < AESPara->u32Len) {
        DEBUG("memory size is too small\n");
        return -1;
    }
    // copy data to HW AES address
    memcpy((void*)stHwAesVA,(void*)AESPara->pBuf,AESPara->u32Len);;
    MDrv_AESDMA_Init(0x00, 0x20000000, 2); // wait the AESDMA.a
    MDrv_AESDMA_Reset();
    MDrv_AESDMA_SelEng(AESPara->eMode, AESPara->bIsDecrypt);
    result=MDrv_AESDMA_SetFileInOut(stHwAesPA, AESPara->u32Len, stHwAesPA, stHwAesPA+AESPara->u32Len-1);
    if (result == DRVAESDMA_MIU_ADDR_ERROR) {
        DEBUG("CommonAES128 execte MDrv_AESDMA_SetFileInOut fail\n");
        return -1;
    }
    if (AESPara->eMode == E_DRVAESDMA_CIPHER_CBC) {
        MDrv_AESDMA_SetIV(AESPara->pIV);
    }
    char key[AES_KEY_LEN] = "\0";
    if (AESPara->pKey!=NULL) {
        memcpy(key,AESPara->pKey,AES_KEY_LEN);
        MDrv_AESDMA_SetKey((MS_U32*)key);
    } else {
        //key is NULL ,we use Efuse key
        DEBUG("Use EFUSE Key\n");
        MDrv_AESDMA_SetSecureKey();
    }
    MDrv_AESDMA_Start(TRUE);
    DEBUG("AESDMA polling finish bits\n");
    while ((MDrv_AESDMA_GetStatus() & BIT(16)) == 0);
    // copy data back
    memcpy((void*)AESPara->pBuf,(void*)stHwAesVA,AESPara->u32Len);
    DEBUG("OK\n");
    return 0;
}

static int Secure_AES_ECB_Decrypt_HW(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    int ret=0;
    DEBUG("IN\n");
    //check u32Len align 16
    if (0 != u32Len%16) {
         DEBUG("u32InLen should align 16\n");
         return -1;
    }
    AES_ParamStruct AESParam;
    AESParam.eMode = E_DRVAESDMA_CIPHER_ECB;
    AESParam.pBuf = u32Addr;
    AESParam.pKey = (MS_U32 *)bKey;
    AESParam.u32Len = u32Len;
    AESParam.pIV= (MS_U32 *)NULL;
    AESParam.bIsDecrypt = TRUE;
    ret=CommonAES128(&AESParam);
    if (ret==0) {
        DEBUG("OK\n");
    } else {
        DEBUG("Secure_AES_Decrypt fail\n");
    }
    return ret;
}
static int Secure_AES_ECB_Encrypt_HW(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    int ret=0;
    DEBUG("IN\n");
    //check u32Len align 16
    if (0 != u32Len%16) {
         DEBUG("u32InLen should align 16\n");
         return -1;
    }
    AES_ParamStruct AESParam;
    AESParam.eMode = E_DRVAESDMA_CIPHER_ECB;
    AESParam.pBuf = u32Addr;
    AESParam.pKey = (MS_U32 *)bKey;
    AESParam.u32Len = u32Len;
    AESParam.pIV= (MS_U32 *)NULL;
    AESParam.bIsDecrypt = FALSE;
    ret=CommonAES128(&AESParam);
    if (ret==0) {
        DEBUG("OK\n");
    } else {
        DEBUG("Secure_AES_Encrypt Error\n");
    }
    return ret;
}

#else
//call SW
#define AES_ENCRYPT     0
#define AES_DECRYPT     1

//SW Efuse Key to encrypt CustomerKeyBank
static MS_U8 hwKey[0x10]={0xE0, 0x10, 0x01, 0xFF, 0x0F, 0xAA, 0x55, 0xFC, \
                       0x92, 0x4D, 0x53, 0x54, 0x41, 0xFF, 0x07, 0x00}; // temporary



/**
 * \brief          AES context structure
 */
typedef struct
{
    int nr;                     /*!<  number of rounds  */
    unsigned long *rk;          /*!<  AES round keys    */
    unsigned long buf[68];      /*!<  unaligned data    */
}
aes_context;


/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef GET_ULONG_LE
#define GET_ULONG_LE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ]       )        \
        | ( (unsigned long) (b)[(i) + 1] <<  8 )        \
        | ( (unsigned long) (b)[(i) + 2] << 16 )        \
        | ( (unsigned long) (b)[(i) + 3] << 24 );       \
}
#endif

#ifndef PUT_ULONG_LE
#define PUT_ULONG_LE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n)       );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
}
#endif


/*
 * Forward S-box & tables
 */
static unsigned char FSb[256];
static unsigned long FT0[256];
static unsigned long FT1[256];
static unsigned long FT2[256];
static unsigned long FT3[256];

/*
 * Reverse S-box & tables
 */
static unsigned char RSb[256];
static unsigned long RT0[256];
static unsigned long RT1[256];
static unsigned long RT2[256];
static unsigned long RT3[256];

/*
 * Round constants
 */
static unsigned long RCON[10];

/*
 * Tables generation code
 */
#define ROTL8(x) ( ( x << 8 ) & 0xFFFFFFFF ) | ( x >> 24 )
#define XTIME(x) ( ( x << 1 ) ^ ( ( x & 0x80 ) ? 0x1B : 0x00 ) )
#define MUL(x,y) ( ( x && y ) ? pow[(log[x]+log[y]) % 255] : 0 )

static int aes_init_done = 0;

static void aes_gen_tables( void )
{
    int i, x, y, z;
    int pow[256];
    int log[256];

    /*
     * compute pow and log tables over GF(2^8)
     */
    for ( i = 0, x = 1; i < 256; i++ ) {
        pow[i] = x;
        log[x] = i;
        x = ( x ^ XTIME( x ) ) & 0xFF;
    }

    /*
     * calculate the round constants
     */
    for ( i = 0, x = 1; i < 10; i++ ) {
        RCON[i] = (unsigned long) x;
        x = XTIME( x ) & 0xFF;
    }

    /*
     * generate the forward and reverse S-boxes
     */
    FSb[0x00] = 0x63;
    RSb[0x63] = 0x00;

    for ( i = 1; i < 256; i++ ) {
        x = pow[255 - log[i]];

        y  = x; y = ( (y << 1) | (y >> 7) ) & 0xFF;
        x ^= y; y = ( (y << 1) | (y >> 7) ) & 0xFF;
        x ^= y; y = ( (y << 1) | (y >> 7) ) & 0xFF;
        x ^= y; y = ( (y << 1) | (y >> 7) ) & 0xFF;
        x ^= y ^ 0x63;

        FSb[i] = (unsigned char) x;
        RSb[x] = (unsigned char) i;
    }

    /*
     * generate the forward and reverse tables
     */
    for ( i = 0; i < 256; i++ ) {
        x = FSb[i];
        y = XTIME( x ) & 0xFF;
        z =  ( y ^ x ) & 0xFF;

        FT0[i] = ( (unsigned long) y       ) ^
                 ( (unsigned long) x <<  8 ) ^
                 ( (unsigned long) x << 16 ) ^
                 ( (unsigned long) z << 24 );

        FT1[i] = ROTL8( FT0[i] );
        FT2[i] = ROTL8( FT1[i] );
        FT3[i] = ROTL8( FT2[i] );

        x = RSb[i];

        RT0[i] = ( (unsigned long) MUL( 0x0E, x )       ) ^
                 ( (unsigned long) MUL( 0x09, x ) <<  8 ) ^
                 ( (unsigned long) MUL( 0x0D, x ) << 16 ) ^
                 ( (unsigned long) MUL( 0x0B, x ) << 24 );

        RT1[i] = ROTL8( RT0[i] );
        RT2[i] = ROTL8( RT1[i] );
        RT3[i] = ROTL8( RT2[i] );
    }
}


/*
 * AES key schedule (encryption)
 */
static void aes_setkey_enc( aes_context *ctx, unsigned char *key, int keysize )
{
    int i;
    unsigned long *RK;

    if ( aes_init_done == 0 ) {
        aes_gen_tables();
        aes_init_done = 1;
    }

    switch ( keysize ) {
        case 128: ctx->nr = 10; break;
        case 192: ctx->nr = 12; break;
        case 256: ctx->nr = 14; break;
        default : return;
    }

    ctx->rk = RK = ctx->buf;

    for ( i = 0; i < (keysize >> 5); i++ ) {
        GET_ULONG_LE( RK[i], key, i << 2 );
    }

    switch ( ctx->nr ) {
        case 10:

            for ( i = 0; i < 10; i++, RK += 4 ) {
                RK[4]  = RK[0] ^ RCON[i] ^
                    ( FSb[ ( RK[3] >>  8 ) & 0xFF ]       ) ^
                    ( FSb[ ( RK[3] >> 16 ) & 0xFF ] <<  8 ) ^
                    ( FSb[ ( RK[3] >> 24 ) & 0xFF ] << 16 ) ^
                    ( FSb[ ( RK[3]       ) & 0xFF ] << 24 );

                RK[5]  = RK[1] ^ RK[4];
                RK[6]  = RK[2] ^ RK[5];
                RK[7]  = RK[3] ^ RK[6];
            }
            break;

        case 12:

            for ( i = 0; i < 8; i++, RK += 6 ) {
                RK[6]  = RK[0] ^ RCON[i] ^
                    ( FSb[ ( RK[5] >>  8 ) & 0xFF ]       ) ^
                    ( FSb[ ( RK[5] >> 16 ) & 0xFF ] <<  8 ) ^
                    ( FSb[ ( RK[5] >> 24 ) & 0xFF ] << 16 ) ^
                    ( FSb[ ( RK[5]       ) & 0xFF ] << 24 );

                RK[7]  = RK[1] ^ RK[6];
                RK[8]  = RK[2] ^ RK[7];
                RK[9]  = RK[3] ^ RK[8];
                RK[10] = RK[4] ^ RK[9];
                RK[11] = RK[5] ^ RK[10];
            }
            break;

        case 14:

            for ( i = 0; i < 7; i++, RK += 8 ) {
                RK[8]  = RK[0] ^ RCON[i] ^
                    ( FSb[ ( RK[7] >>  8 ) & 0xFF ]       ) ^
                    ( FSb[ ( RK[7] >> 16 ) & 0xFF ] <<  8 ) ^
                    ( FSb[ ( RK[7] >> 24 ) & 0xFF ] << 16 ) ^
                    ( FSb[ ( RK[7]       ) & 0xFF ] << 24 );

                RK[9]  = RK[1] ^ RK[8];
                RK[10] = RK[2] ^ RK[9];
                RK[11] = RK[3] ^ RK[10];

                RK[12] = RK[4] ^
                    ( FSb[ ( RK[11]       ) & 0xFF ]       ) ^
                    ( FSb[ ( RK[11] >>  8 ) & 0xFF ] <<  8 ) ^
                    ( FSb[ ( RK[11] >> 16 ) & 0xFF ] << 16 ) ^
                    ( FSb[ ( RK[11] >> 24 ) & 0xFF ] << 24 );

                RK[13] = RK[5] ^ RK[12];
                RK[14] = RK[6] ^ RK[13];
                RK[15] = RK[7] ^ RK[14];
            }
            break;

        default:

            break;
    }
}

/*
 * AES key schedule (decryption)
 */
static void aes_setkey_dec( aes_context *ctx, unsigned char *key, int keysize )
{
    int i, j;
    aes_context cty;
    unsigned long *RK;
    unsigned long *SK;

    switch ( keysize ) {
        case 128: ctx->nr = 10; break;
        case 192: ctx->nr = 12; break;
        case 256: ctx->nr = 14; break;
        default : return;
    }

    ctx->rk = RK = ctx->buf;

    aes_setkey_enc( &cty, key, keysize );
    SK = cty.rk + cty.nr * 4;

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

    for ( i = ctx->nr, SK -= 8; i > 1; i--, SK -= 8 ) {
        for ( j = 0; j < 4; j++, SK++ ) {
            *RK++ = RT0[ FSb[ ( *SK       ) & 0xFF ] ] ^
                    RT1[ FSb[ ( *SK >>  8 ) & 0xFF ] ] ^
                    RT2[ FSb[ ( *SK >> 16 ) & 0xFF ] ] ^
                    RT3[ FSb[ ( *SK >> 24 ) & 0xFF ] ];
        }
    }

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

    memset( &cty, 0, sizeof( aes_context ) );
}

#define AES_FROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
    X0 = *RK++ ^ FT0[ ( Y0       ) & 0xFF ] ^   \
                 FT1[ ( Y1 >>  8 ) & 0xFF ] ^   \
                 FT2[ ( Y2 >> 16 ) & 0xFF ] ^   \
                 FT3[ ( Y3 >> 24 ) & 0xFF ];    \
                                                \
    X1 = *RK++ ^ FT0[ ( Y1       ) & 0xFF ] ^   \
                 FT1[ ( Y2 >>  8 ) & 0xFF ] ^   \
                 FT2[ ( Y3 >> 16 ) & 0xFF ] ^   \
                 FT3[ ( Y0 >> 24 ) & 0xFF ];    \
                                                \
    X2 = *RK++ ^ FT0[ ( Y2       ) & 0xFF ] ^   \
                 FT1[ ( Y3 >>  8 ) & 0xFF ] ^   \
                 FT2[ ( Y0 >> 16 ) & 0xFF ] ^   \
                 FT3[ ( Y1 >> 24 ) & 0xFF ];    \
                                                \
    X3 = *RK++ ^ FT0[ ( Y3       ) & 0xFF ] ^   \
                 FT1[ ( Y0 >>  8 ) & 0xFF ] ^   \
                 FT2[ ( Y1 >> 16 ) & 0xFF ] ^   \
                 FT3[ ( Y2 >> 24 ) & 0xFF ];    \
}

#define AES_RROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
    X0 = *RK++ ^ RT0[ ( Y0       ) & 0xFF ] ^   \
                 RT1[ ( Y3 >>  8 ) & 0xFF ] ^   \
                 RT2[ ( Y2 >> 16 ) & 0xFF ] ^   \
                 RT3[ ( Y1 >> 24 ) & 0xFF ];    \
                                                \
    X1 = *RK++ ^ RT0[ ( Y1       ) & 0xFF ] ^   \
                 RT1[ ( Y0 >>  8 ) & 0xFF ] ^   \
                 RT2[ ( Y3 >> 16 ) & 0xFF ] ^   \
                 RT3[ ( Y2 >> 24 ) & 0xFF ];    \
                                                \
    X2 = *RK++ ^ RT0[ ( Y2       ) & 0xFF ] ^   \
                 RT1[ ( Y1 >>  8 ) & 0xFF ] ^   \
                 RT2[ ( Y0 >> 16 ) & 0xFF ] ^   \
                 RT3[ ( Y3 >> 24 ) & 0xFF ];    \
                                                \
    X3 = *RK++ ^ RT0[ ( Y3       ) & 0xFF ] ^   \
                 RT1[ ( Y2 >>  8 ) & 0xFF ] ^   \
                 RT2[ ( Y1 >> 16 ) & 0xFF ] ^   \
                 RT3[ ( Y0 >> 24 ) & 0xFF ];    \
}

/*
 * AES-ECB block encryption/decryption
 */
static void aes_crypt_ecb( aes_context *ctx,
                    int mode,
                    unsigned char input[16],
                    unsigned char output[16] )
{
    int i;
    unsigned long *RK, X0, X1, X2, X3, Y0, Y1, Y2, Y3;


    RK = ctx->rk;

    GET_ULONG_LE( X0, input,  0 ); X0 ^= *RK++;
    GET_ULONG_LE( X1, input,  4 ); X1 ^= *RK++;
    GET_ULONG_LE( X2, input,  8 ); X2 ^= *RK++;
    GET_ULONG_LE( X3, input, 12 ); X3 ^= *RK++;

    if ( mode == AES_ENCRYPT ) {
        for ( i = (ctx->nr >> 1); i > 1; i-- ) {
            AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
            AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
        }

        AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );

        X0 = *RK++ ^ ( FSb[ ( Y0       ) & 0xFF ]       ) ^
                     ( FSb[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( FSb[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
                     ( FSb[ ( Y3 >> 24 ) & 0xFF ] << 24 );

        X1 = *RK++ ^ ( FSb[ ( Y1       ) & 0xFF ]       ) ^
                     ( FSb[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( FSb[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
                     ( FSb[ ( Y0 >> 24 ) & 0xFF ] << 24 );

        X2 = *RK++ ^ ( FSb[ ( Y2       ) & 0xFF ]       ) ^
                     ( FSb[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( FSb[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
                     ( FSb[ ( Y1 >> 24 ) & 0xFF ] << 24 );

        X3 = *RK++ ^ ( FSb[ ( Y3       ) & 0xFF ]       ) ^
                     ( FSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( FSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
                     ( FSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );
    }else {
        /* AES_DECRYPT */
        for ( i = (ctx->nr >> 1); i > 1; i-- ) {
            AES_RROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
            AES_RROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
        }

        AES_RROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );

        X0 = *RK++ ^ ( RSb[ ( Y0       ) & 0xFF ]       ) ^
                     ( RSb[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( RSb[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
                     ( RSb[ ( Y1 >> 24 ) & 0xFF ] << 24 );

        X1 = *RK++ ^ ( RSb[ ( Y1       ) & 0xFF ]       ) ^
                     ( RSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( RSb[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
                     ( RSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );

        X2 = *RK++ ^ ( RSb[ ( Y2       ) & 0xFF ]       ) ^
                     ( RSb[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( RSb[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
                     ( RSb[ ( Y3 >> 24 ) & 0xFF ] << 24 );

        X3 = *RK++ ^ ( RSb[ ( Y3       ) & 0xFF ]       ) ^
                     ( RSb[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
                     ( RSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
                     ( RSb[ ( Y0 >> 24 ) & 0xFF ] << 24 );
    }

    PUT_ULONG_LE( X0, output,  0 );
    PUT_ULONG_LE( X1, output,  4 );
    PUT_ULONG_LE( X2, output,  8 );
    PUT_ULONG_LE( X3, output, 12 );
}
static int Secure_AES_ECB_Decrypt_SW(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    DEBUG("IN\n");
    if (0 != u32Len%16) {
         DEBUG("u32InLen should align 16\n");
         return -1;
    }
    aes_context aes_ctx;
    memset(&aes_ctx, 0, sizeof(aes_context));
    if (NULL == bKey) {
        // use hwKey to handle CustomerKeyBank
        DEBUG("Use SW EFUSE Key\n");
        aes_setkey_dec( &aes_ctx, hwKey, 128 );
    } else {
        aes_setkey_dec( &aes_ctx, bKey, 128 );
    }
    int offset;
    unsigned char *ptr = (unsigned char*)u32Addr;
    for( offset = 0; offset < u32Len; offset += 16 ) {
        aes_crypt_ecb( &aes_ctx, AES_DECRYPT, ptr, ptr );
        ptr += 16;
    }
    DEBUG("OK\n");
    return 0;
}
static int Secure_AES_ECB_Encrypt_SW(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    DEBUG("IN\n");
    if (0 != u32Len%16) {
         DEBUG("u32InLen should align 16\n");
         return -1;
    }
    aes_context aes_ctx;
    memset(&aes_ctx, 0,sizeof(aes_context));
    if (NULL == bKey) {
        // use hwKey to handle CustomerKeyBank
        DEBUG("Use SW EFUSE Key\n");
        aes_setkey_enc( &aes_ctx, hwKey, 128 );
    } else {
        aes_setkey_enc( &aes_ctx, bKey, 128 );
    }
    int offset;
    unsigned char *ptr = (unsigned char*)u32Addr;
    for ( offset = 0; offset < u32Len; offset += 16 ) {
        aes_crypt_ecb( &aes_ctx, AES_ENCRYPT, ptr, ptr );
        ptr += 16;
    }
    DEBUG("OK\n");
    return 0;
}

#endif

int Secure_AES_ECB_Decrypt(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    int ret=0;
    DEBUG("IN\n");
    #if defined(BUILD_WITH_SECURE_AESDMA)
    //AESDMA
    ret = Secure_AES_ECB_Decrypt_HW(u32Addr,u32Len,bKey);
    #else
    //SW
    ret = Secure_AES_ECB_Decrypt_SW(u32Addr,u32Len,bKey);
    #endif
    if (ret==0) {
        DEBUG("OK\n");
    } else {
        DEBUG("AES Decrypt Error\n");
    }

    return ret;
}

int Secure_AES_ECB_Encrypt(MS_U32 u32Addr, MS_U32 u32Len, MS_U8 *bKey)
{
    int ret=0;
    DEBUG("IN\n");
    #if defined(BUILD_WITH_SECURE_AESDMA)
    //AESDMA
    ret = Secure_AES_ECB_Encrypt_HW(u32Addr,u32Len,bKey);
    #else
    //SW
    ret = Secure_AES_ECB_Encrypt_SW(u32Addr,u32Len,bKey);
    #endif
    if (ret==0) {
        DEBUG("OK\n");
    } else {
        DEBUG("AES Encrypt fail\n");
    }

    return ret;
}

void Secure_AES_ECB_Test(void)
{
    MS_U8 u8Plaintext[] = {
        0x53 ,0x45 ,0x43 ,0x55 ,0x52 ,0x49 ,0x54 ,0x59 ,0x00 ,0x00 ,0x00 ,0x00 ,0x40 ,0x03 ,0x00 ,0x00,
        0x5f ,0x09 ,0x4c ,0x87 ,0x86 ,0xdc ,0x0a ,0xa9 ,0x59 ,0x44 ,0x32 ,0x7a ,0xc8 ,0xa8 ,0x98 ,0x05};
    MS_U8 u8Plaintextbackup[]= {
        0x53 ,0x45 ,0x43 ,0x55 ,0x52 ,0x49 ,0x54 ,0x59 ,0x00 ,0x00 ,0x00 ,0x00 ,0x40 ,0x03 ,0x00 ,0x00,
        0x5f ,0x09 ,0x4c ,0x87 ,0x86 ,0xdc ,0x0a ,0xa9 ,0x59 ,0x44 ,0x32 ,0x7a ,0xc8 ,0xa8 ,0x98 ,0x05};
    MS_U32 length = 32;
    int ret = -1;
    // encrypt
    //ret = Secure_AES_ECB_Encrypt((MS_U32)u8Plaintext,length,NULL);
    ret = Secure_AES_ECB_Encrypt((MS_U32)u8Plaintext,length,NULL);
    if(0 != ret) {
        DEBUG("encrypt error\n");
        return;
    }
    DEBUG("data after encrypt:\n");
    mem_dump((MS_U32)u8Plaintext,length);
    // decrypt
    //ret = Secure_AES_ECB_Decrypt((MS_U32)u8Plaintext,length,NULL);
    ret = Secure_AES_ECB_Decrypt((MS_U32)u8Plaintext,length,NULL);
    DEBUG("data after decrypt:\n");
    mem_dump((MS_U32)u8Plaintext,length);
    // check data equal
    if (0 != memcmp(u8Plaintext,u8Plaintextbackup,length)) {
        DEBUG("data not equal\n");
        DEBUG("u8Plaintextbackup data:");
        mem_dump((MS_U32)u8Plaintextbackup,length);
    } else {
        DEBUG("data equal");
    }
}
