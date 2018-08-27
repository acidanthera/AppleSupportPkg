/** @file

AppleUiSupport

Copyright (c) 2018, savvas.<BR>


All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef APPLE_IMAGE_CODEC_H
#define APPLE_IMAGE_CODEC_H

//
// Efi Graphics Image
//
typedef struct {
    INTN            Width;
    INTN            Height;
    BOOLEAN         HasAlpha;
    EFI_UGA_PIXEL   *PixelData;
} EG_IMAGE;

#endif //APPLE_IMAGE_CODEC_H
