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

#ifndef __SECURE_UPDATER_H__
#define __SECURE_UPDATER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "minzip/Zip.h"
#include "crypto_common.h"
#include "roots.h"

#define FRAGMENT_NUM 8

typedef struct
{
    unsigned long u32Num;
    unsigned long u32Size;
}IMAGE_INFO;


typedef struct
{
    unsigned char u8RSAKernelPublicKey[RSA_PUBLIC_KEY_LEN];
    unsigned char u8RSARootFsPublicKey[RSA_PUBLIC_KEY_LEN];
    unsigned char u8RSAMSLIBPublicKey[RSA_PUBLIC_KEY_LEN];
    unsigned char u8RSACONFIGPublicKey[RSA_PUBLIC_KEY_LEN];
    unsigned char u8RSAAPPPublicKey[RSA_PUBLIC_KEY_LEN];
    unsigned char u8AESKernelKey[AES_KEY_LEN];
}SECURE_KEY_SET;


typedef struct
{
    unsigned char u8SecIdentify[8];
    IMAGE_INFO info;
    unsigned char u8Signature[SIGNATURE_LEN];
}_SUB_SECURE_INFO;


typedef struct
{
    _SUB_SECURE_INFO sInfo;
    _SUB_SECURE_INFO sInfo_Interleave;
}SUB_SECURE_INFO;


typedef struct
{
    _SUB_SECURE_INFO sInfo[FRAGMENT_NUM];
    _SUB_SECURE_INFO sInfo_Interleave[FRAGMENT_NUM];
}SUB_SECURE_INFO_FOR_PARTIAL_AUTH;


typedef struct
{
    _SUB_SECURE_INFO Reserve1; //Reserve this space is for backward compatible. It's for Key in the beginning
    #ifdef BUILD_WITH_TEE
    _SUB_SECURE_INFO tee;
    #else
    _SUB_SECURE_INFO Reserve2; //Reserve this space is for backward compatible. It's for uboot in the beginning
    #endif
    //secureboot
    _SUB_SECURE_INFO Recovery;
    _SUB_SECURE_INFO Boot;
    #if 0
    _SUB_SECURE_INFO Kernel;
    _SUB_SECURE_INFO RootFs[FRAGMENT_NUM];
    _SUB_SECURE_INFO MsLib[FRAGMENT_NUM];
    _SUB_SECURE_INFO Config[FRAGMENT_NUM];
    _SUB_SECURE_INFO App[FRAGMENT_NUM];
    #endif
    SECURE_KEY_SET Key;
}_SECURITY_INFO_DATA;


typedef struct
{
    unsigned long crc;                  /* CRC32 over data bytes    */
    _SECURITY_INFO_DATA data;
    _SECURITY_INFO_DATA data_interleave;
}SECURITY_INFO;

int  secure_verify(ZipArchive* zip,const char* path,char* updatefilepath);
int  update_secure_info(char* partition,char* filename);

#ifdef BUILD_WITH_TEE
typedef struct
{
    unsigned long Length;
    unsigned long Offset;
}ST_HEADER_INFO;

typedef struct
{
    ST_HEADER_INFO stSignature;
    ST_HEADER_INFO stConfig;
}ST_SIGN_CONFIG;

int update_nuttx_config(char* config,char* filename);
#endif
#ifdef __cplusplus
}
#endif

#endif //__SECURE_UPDATER_H__
