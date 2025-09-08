/* $Id$ */
/** @file
 * DevGeForce - VBox GeForce FX 5900 (NV35) device, internal header.
 */

/*
 * Copyright (C) 2025 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * --------------------------------------------------------------------
 *
 * This code ports VGA functionality from the Bochs GeForce emulation
 * to VirtualBox device framework.
 */

#ifndef VBOX_INCLUDED_SRC_Graphics_DevGeForce_h
#define VBOX_INCLUDED_SRC_Graphics_DevGeForce_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmdev.h>
#include <VBox/AssertGuest.h>
#include <iprt/assert.h>

/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** The GeForce FX 5900 device name. */
#define GEFORCE_DEV_NAME                "geforce"

/** Vendor ID for NVIDIA. */
#define GEFORCE_PCI_VENDOR_ID           0x10DE

/** Device ID for GeForce FX 5900 (NV35). */
#define GEFORCE_PCI_DEVICE_ID           0x0331

/** PCI class: Display controller / VGA compatible controller. */
#define GEFORCE_PCI_CLASS               0x0300

/** Size of MMIO region. */
#define GEFORCE_PNPMMIO_SIZE            0x1000000

/** Size of video memory. */
#define GEFORCE_VRAM_SIZE               (128 * _1M)

/** Maximum CRTC register index. */
#define GEFORCE_CRTC_MAX                0x9F

/** Standard VGA CRTC register count. */
#define VGA_CRTC_MAX                    0x18

/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * The device state for the GeForce FX 5900 device.
 */
typedef struct GEFORCESTATE
{
    /** Pointer to the device instance. */
    PPDMDEVINS                  pDevIns;
    
    /** The display port base interface. */
    PDMIBASE                    IBase;
    
    /** The display port interface. */
    PDMIDISPLAYPORT             IPort;
    
    /** Pointer to base interface of the driver. */
    R3PTRTYPE(PPDMIBASE)        pDrvBase;
    
    /** Pointer to display connector interface of the driver. */
    R3PTRTYPE(PPDMIDISPLAYCONNECTOR) pDrv;
    
    /** Video memory base address. */
    RTGCPHYS                    GCPhysVRAM;
    
    /** Video memory size. */
    uint32_t                    cbVRAM;
    
    /** Video memory pointer (host address). */
    R3PTRTYPE(uint8_t *)        pbVRAM;
    
    /** MMIO region handle. */
    IOMMMIOHANDLE               hMmio;
    
    /** I/O port handles for VGA compatibility. */
    IOMIOPORTHANDLE             hIoPortVgaCrt;
    IOMIOPORTHANDLE             hIoPortVgaGfx;
    IOMIOPORTHANDLE             hIoPortVgaSeq;
    IOMIOPORTHANDLE             hIoPortVgaAttr;
    
    /** CRTC registers. */
    struct
    {
        /** Current CRTC index register. */
        uint8_t                 index;
        /** CRTC data registers. */
        uint8_t                 reg[GEFORCE_CRTC_MAX + 1];
    } crtc;
    
    /** Current graphics mode parameters. */
    struct
    {
        /** Horizontal resolution. */
        uint32_t                xres;
        /** Vertical resolution. */
        uint32_t                yres;
        /** Bytes per pixel. */
        uint32_t                bpp;
        /** Line pitch in bytes. */
        uint32_t                pitch;
    } mode;
    
    /** Card type identifier (0x35 for NV35/FX5900). */
    uint32_t                    card_type;
    
    /** The PCI device. Must be last for compat reasons. */
    PDMPCIDEV                   PciDev;
    
} GEFORCESTATE;
/** Pointer to the device state. */
typedef GEFORCESTATE *PGEFORCESTATE;

AssertCompileMemberAlignment(GEFORCESTATE, PciDev, 8);

/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

#endif /* !VBOX_INCLUDED_SRC_Graphics_DevGeForce_h */