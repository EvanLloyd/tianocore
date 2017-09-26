/** @file
  This file contains the platform independent parts of PL111Lcd

  Copyright (c) 2011-2017, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/MemoryAllocationLib.h>

#include "PL111Lcd.h"

/** Check for presence of PL111.

  @retval EFI_SUCCESS          Returns success if platform implements a
                               PL111 controller.

  @retval EFI_NOT_FOUND        PL111 display controller not found the plaform.
**/
EFI_STATUS
LcdIdentify (VOID)
{
  DEBUG ((EFI_D_WARN, "Probing ID registers at 0x%lx for a PL111\n",
    PL111_REG_CLCD_PERIPH_ID_0));

  // Check if this is a PL111
  if (MmioRead8 (PL111_REG_CLCD_PERIPH_ID_0) == PL111_CLCD_PERIPH_ID_0 &&
      MmioRead8 (PL111_REG_CLCD_PERIPH_ID_1) == PL111_CLCD_PERIPH_ID_1 &&
     (MmioRead8 (PL111_REG_CLCD_PERIPH_ID_2) & 0xf) == PL111_CLCD_PERIPH_ID_2 &&
      MmioRead8 (PL111_REG_CLCD_PERIPH_ID_3) == PL111_CLCD_PERIPH_ID_3 &&
      MmioRead8 (PL111_REG_CLCD_P_CELL_ID_0) == PL111_CLCD_P_CELL_ID_0 &&
      MmioRead8 (PL111_REG_CLCD_P_CELL_ID_1) == PL111_CLCD_P_CELL_ID_1 &&
      MmioRead8 (PL111_REG_CLCD_P_CELL_ID_2) == PL111_CLCD_P_CELL_ID_2 &&
      MmioRead8 (PL111_REG_CLCD_P_CELL_ID_3) == PL111_CLCD_P_CELL_ID_3) {
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

/** Initialize display.

  @param[in]  VramBaseAddress    Address of the frame buffer.

  @retval EFI_SUCCESS            Initialization of display successful.
**/
EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS   VramBaseAddress
  )
{
  // Define start of the VRAM. This never changes for any graphics mode
  MmioWrite32 (PL111_REG_LCD_UP_BASE, (UINT32)VramBaseAddress);
  MmioWrite32 (PL111_REG_LCD_LP_BASE, 0); // We are not using a double buffer

  // Disable all interrupts from the PL111
  MmioWrite32 (PL111_REG_LCD_IMSC, 0);

  return EFI_SUCCESS;
}

/** Set requested mode of the display.

  @param[in] ModeNumbe           Display mode number.

  @retval EFI_SUCCESS            Display set mode successfuly.
  @retval EFI_DEVICE_ERROR       It returns an error if display timing
                                 information is not available.
**/
EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  )
{
  EFI_STATUS        Status;
  UINT32            HRes;
  UINT32            HSync;
  UINT32            HBackPorch;
  UINT32            HFrontPorch;
  UINT32            VRes;
  UINT32            VSync;
  UINT32            VBackPorch;
  UINT32            VFrontPorch;
  UINT32            LcdControl;
  LCD_BPP           LcdBpp;

  // Set the video mode timings and other relevant information
  Status = LcdPlatformGetTimings (
             ModeNumber,
             &HRes,
             &HSync,
             &HBackPorch,
             &HFrontPorch,
             &VRes,
             &VSync,
             &VBackPorch,
             &VFrontPorch
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = LcdPlatformGetBpp (ModeNumber, &LcdBpp);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  // Disable the CLCD_LcdEn bit
  LcdControl = MmioRead32 (PL111_REG_LCD_CONTROL);
  MmioWrite32 (PL111_REG_LCD_CONTROL, LcdControl & ~1);

  // Set Timings
  MmioWrite32 (
    PL111_REG_LCD_TIMING_0,
    HOR_AXIS_PANEL (HBackPorch, HFrontPorch, HSync, HRes)
    );

  MmioWrite32 (
    PL111_REG_LCD_TIMING_1,
    VER_AXIS_PANEL (VBackPorch, VFrontPorch, VSync, VRes)
    );

  MmioWrite32 (PL111_REG_LCD_TIMING_2, CLK_SIG_POLARITY (HRes));
  MmioWrite32 (PL111_REG_LCD_TIMING_3, 0);

  // PL111_REG_LCD_CONTROL
  LcdControl = PL111_CTRL_LCD_EN | PL111_CTRL_LCD_BPP (LcdBpp)
               | PL111_CTRL_LCD_TFT | PL111_CTRL_BGR;
  MmioWrite32 (PL111_REG_LCD_CONTROL, LcdControl);

  // Turn on power to the LCD Panel
  LcdControl |= PL111_CTRL_LCD_PWR;
  MmioWrite32 (PL111_REG_LCD_CONTROL, LcdControl);

  return EFI_SUCCESS;
}

/** De-initializes the display.
*/
VOID
LcdShutdown (VOID)
{
  // Disable the controller
  MmioAnd32 (PL111_REG_LCD_CONTROL, ~PL111_CTRL_LCD_EN);
}
