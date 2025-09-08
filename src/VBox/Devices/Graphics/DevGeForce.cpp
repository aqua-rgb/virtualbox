/* $Id$ */
/** @file
 * DevGeForce - VBox GeForce FX 5900 (NV35) device.
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
 * to VirtualBox device framework, focusing on the GeForce FX 5900 (NV35).
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_VGA
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pgm.h>
#include <VBox/AssertGuest.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/uuid.h>

#include "DevGeForce.h"

/* Ensure we're in the right context for device registration. */
#ifdef IN_RING3


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Default GeForce FX 5900 straps. */
#define GEFORCE_STRAPS0_DEFAULT         0x0002e3ff


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Initialize CRTC registers to default values.
 *
 * @param   pThis           The GeForce device state.
 */
static void geforceInitCrtc(PGEFORCESTATE pThis)
{
    /* Clear all CRTC registers. */
    pThis->crtc.index = GEFORCE_CRTC_MAX + 1;
    RT_ZERO(pThis->crtc.reg);
    
    /* Set some basic VGA-compatible defaults. */
    pThis->crtc.reg[0x00] = 0x5F;   /* Horizontal Total */
    pThis->crtc.reg[0x01] = 0x4F;   /* Horizontal Display End */
    pThis->crtc.reg[0x02] = 0x50;   /* Start Horizontal Blanking */
    pThis->crtc.reg[0x03] = 0x82;   /* End Horizontal Blanking */
    pThis->crtc.reg[0x04] = 0x54;   /* Start Horizontal Retrace */
    pThis->crtc.reg[0x05] = 0x80;   /* End Horizontal Retrace */
    pThis->crtc.reg[0x06] = 0x0B;   /* Vertical Total (low) */
    pThis->crtc.reg[0x07] = 0x3E;   /* Overflow */
    pThis->crtc.reg[0x08] = 0x00;   /* Preset Row Scan */
    pThis->crtc.reg[0x09] = 0x40;   /* Maximum Scan Line */
    pThis->crtc.reg[0x0C] = 0x00;   /* Start Address High */
    pThis->crtc.reg[0x0D] = 0x00;   /* Start Address Low */
    pThis->crtc.reg[0x0E] = 0x00;   /* Cursor Location High */
    pThis->crtc.reg[0x0F] = 0x00;   /* Cursor Location Low */
    pThis->crtc.reg[0x10] = 0xEA;   /* Vertical Retrace Start (low) */
    pThis->crtc.reg[0x11] = 0x8C;   /* Vertical Retrace End */
    pThis->crtc.reg[0x12] = 0xDF;   /* Vertical Display End (low) */
    pThis->crtc.reg[0x13] = 0x28;   /* Offset */
    pThis->crtc.reg[0x14] = 0x00;   /* Underline Location */
    pThis->crtc.reg[0x15] = 0xE7;   /* Start Vertical Blanking (low) */
    pThis->crtc.reg[0x16] = 0x04;   /* End Vertical Blanking */
    pThis->crtc.reg[0x17] = 0xE3;   /* CRTC Mode Control */
    pThis->crtc.reg[0x18] = 0xFF;   /* Line Compare */
    
    /* Initialize basic display mode (640x480x16). */
    pThis->mode.xres = 640;
    pThis->mode.yres = 480;
    pThis->mode.bpp = 16;
    pThis->mode.pitch = 640 * 2;
}

/**
 * Read from CRTC register.
 *
 * @returns Register value.
 * @param   pThis           The GeForce device state.
 * @param   index           Register index.
 */
static uint8_t geforceReadCrtc(PGEFORCESTATE pThis, uint32_t index)
{
    if (index <= GEFORCE_CRTC_MAX)
        return pThis->crtc.reg[index];
    return 0xFF;
}

/**
 * Write to CRTC register.
 *
 * @param   pThis           The GeForce device state.
 * @param   index           Register index.
 * @param   value           Value to write.
 */
static void geforceWriteCrtc(PGEFORCESTATE pThis, uint32_t index, uint8_t value)
{
    if (index <= GEFORCE_CRTC_MAX)
    {
        pThis->crtc.reg[index] = value;
        
        /* Handle mode changes for basic VGA registers. */
        if (index <= VGA_CRTC_MAX)
        {
            LogFlow(("GeForce: CRTC[0x%02X] = 0x%02X\n", index, value));
            /* Basic VGA mode detection could be added here. */
        }
    }
}


/*********************************************************************************************************************************
*   I/O Port Handlers                                                                                                            *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNIOMIOPORTNEWOUT, VGA CRTC index port write.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceIoPortCrtcIndexWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t u32, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser, offPort);
    
    if (cb == 1)
    {
        pThis->crtc.index = u32 & 0xFF;
        LogFlow(("GeForce: CRTC index = 0x%02X\n", pThis->crtc.index));
    }
    
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTNEWIN, VGA CRTC index port read.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceIoPortCrtcIndexRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t *pu32, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser, offPort);
    
    if (cb == 1)
    {
        *pu32 = pThis->crtc.index;
        LogFlow(("GeForce: CRTC index read = 0x%02X\n", *pu32));
    }
    else
        *pu32 = UINT32_MAX;
    
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTNEWOUT, VGA CRTC data port write.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceIoPortCrtcDataWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t u32, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser, offPort);
    
    if (cb == 1)
    {
        geforceWriteCrtc(pThis, pThis->crtc.index, u32 & 0xFF);
        LogFlow(("GeForce: CRTC[0x%02X] = 0x%02X\n", pThis->crtc.index, u32 & 0xFF));
    }
    
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTNEWIN, VGA CRTC data port read.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceIoPortCrtcDataRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t *pu32, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser, offPort);
    
    if (cb == 1)
    {
        *pu32 = geforceReadCrtc(pThis, pThis->crtc.index);
        LogFlow(("GeForce: CRTC[0x%02X] read = 0x%02X\n", pThis->crtc.index, *pu32));
    }
    else
        *pu32 = UINT32_MAX;
    
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   MMIO Handlers                                                                                                                *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNIOMMMIONEWREAD, GeForce MMIO read.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void *pv, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser);
    
    uint32_t *pu32 = (uint32_t *)pv;
    uint32_t value = 0;
    
    LogFlow(("GeForce: MMIO read at 0x%RGp, cb=%u\n", off, cb));
    
    /* Validate access size and alignment. */
    if (cb != 1 && cb != 2 && cb != 4)
    {
        LogRel(("GeForce: Invalid MMIO read size %u at offset 0x%RGp\n", cb, off));
        return VINF_IOM_MMIO_UNUSED_FF;
    }
    
    /* Handle basic registers for the GeForce FX 5900. */
    switch (off)
    {
        case 0x000000:  /* PMC_BOOT_0 */
            value = 0x03520000 | (pThis->card_type << 16);  /* NV35 = 0x35 */
            LogFlow(("GeForce: PMC_BOOT_0 read = 0x%08X\n", value));
            break;
            
        case 0x101000:  /* STRAPS */
            value = GEFORCE_STRAPS0_DEFAULT;
            LogFlow(("GeForce: STRAPS read = 0x%08X\n", value));
            break;
            
        case 0x400100:  /* Graphics interrupt status */
            value = 0;
            LogFlow(("GeForce: Graphics interrupt status read = 0x%08X\n", value));
            break;
            
        case 0x60013c:  /* Graphics interrupt enable */
            value = 0;
            LogFlow(("GeForce: Graphics interrupt enable read = 0x%08X\n", value));
            break;
            
        default:
            value = 0;
            LogFlow(("GeForce: Unhandled MMIO read at 0x%RGp, returning 0\n", off));
            break;
    }
    
    if (cb == 4)
        *pu32 = value;
    else if (cb == 1)
        *(uint8_t *)pv = value & 0xFF;
    else if (cb == 2)
        *(uint16_t *)pv = value & 0xFFFF;
    
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMMMIONEWWRITE, GeForce MMIO write.}
 */
static DECLCALLBACK(VBOXSTRICTRC) geforceMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void const *pv, unsigned cb)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    RT_NOREF(pvUser);
    
    uint32_t value = 0;
    if (cb == 4)
        value = *(const uint32_t *)pv;
    else if (cb == 1)
        value = *(const uint8_t *)pv;
    else if (cb == 2)
        value = *(const uint16_t *)pv;
    
    LogFlow(("GeForce: MMIO write at 0x%RGp = 0x%X, cb=%u\n", off, value, cb));
    
    /* Handle basic registers for the GeForce FX 5900. */
    switch (off)
    {
        case 0x400100:  /* Graphics interrupt status */
            /* Clear interrupts on write. */
            break;
            
        case 0x60013c:  /* Graphics interrupt enable */
            /* Store interrupt enable mask. */
            break;
            
        default:
            LogFlow(("GeForce: Unhandled MMIO write at 0x%RGp = 0x%X\n", off, value));
            break;
    }
    
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Device Interface                                                                                                             *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) geforceQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PGEFORCESTATE pThis = RT_FROM_MEMBER(pInterface, GEFORCESTATE, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIDISPLAYPORT, &pThis->IPort);
    return NULL;
}

/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnUpdateDisplay}
 */
static DECLCALLBACK(int) geforcePortUpdateDisplay(PPDMIDISPLAYPORT pInterface)
{
    RT_NOREF(pInterface);
    /* Basic implementation - just return success for now. */
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnUpdateDisplayAll}
 */
static DECLCALLBACK(int) geforcePortUpdateDisplayAll(PPDMIDISPLAYPORT pInterface, bool fFailOnResize)
{
    RT_NOREF(pInterface, fFailOnResize);
    /* Basic implementation - just return success for now. */
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnQueryVideoMode}
 */
static DECLCALLBACK(int) geforcePortQueryVideoMode(PPDMIDISPLAYPORT pInterface, uint32_t *pcx, uint32_t *pcy, uint32_t *pcBits)
{
    PGEFORCESTATE pThis = RT_FROM_MEMBER(pInterface, GEFORCESTATE, IPort);
    
    if (pcx)
        *pcx = pThis->mode.xres;
    if (pcy)
        *pcy = pThis->mode.yres;
    if (pcBits)
        *pcBits = pThis->mode.bpp;
        
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnSetRefreshRate}
 */
static DECLCALLBACK(int) geforcePortSetRefreshRate(PPDMIDISPLAYPORT pInterface, uint32_t cMilliesInterval)
{
    RT_NOREF(pInterface, cMilliesInterval);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIDISPLAYPORT,pfnTakeScreenshot}
 */
static DECLCALLBACK(int) geforcePortTakeScreenshot(PPDMIDISPLAYPORT pInterface, uint8_t **ppbData, size_t *pcbData,
                                                   uint32_t *pcx, uint32_t *pcy)
{
    RT_NOREF(pInterface, ppbData, pcbData, pcx, pcy);
    return VERR_NOT_SUPPORTED;
}


/*********************************************************************************************************************************
*   Saved State                                                                                                                  *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) geforceR3LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    PCPDMDEVHLPR3 pHlp = pDevIns->pHlpR3;
    RT_NOREF(uPass);
    
    /* Save the basic configuration. */
    pHlp->pfnSSMPutU32(pSSM, pThis->cbVRAM);
    pHlp->pfnSSMPutU32(pSSM, pThis->card_type);
    
    return VINF_SSM_DONT_CALL_AGAIN;
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) geforceR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    PCPDMDEVHLPR3 pHlp = pDevIns->pHlpR3;
    
    /* Save CRTC state. */
    pHlp->pfnSSMPutU8(pSSM, pThis->crtc.index);
    pHlp->pfnSSMPutMem(pSSM, pThis->crtc.reg, sizeof(pThis->crtc.reg));
    
    /* Save mode state. */
    pHlp->pfnSSMPutU32(pSSM, pThis->mode.xres);
    pHlp->pfnSSMPutU32(pSSM, pThis->mode.yres);
    pHlp->pfnSSMPutU32(pSSM, pThis->mode.bpp);
    pHlp->pfnSSMPutU32(pSSM, pThis->mode.pitch);
    
    /* Save VRAM content. */
    pHlp->pfnSSMPutMem(pSSM, pThis->pbVRAM, pThis->cbVRAM);
    
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) geforceR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    PCPDMDEVHLPR3 pHlp = pDevIns->pHlpR3;
    int rc;
    RT_NOREF(uVersion);
    
    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;
    
    /* Load CRTC state. */
    rc = pHlp->pfnSSMGetU8(pSSM, &pThis->crtc.index);
    AssertRCReturn(rc, rc);
    rc = pHlp->pfnSSMGetMem(pSSM, pThis->crtc.reg, sizeof(pThis->crtc.reg));
    AssertRCReturn(rc, rc);
    
    /* Load mode state. */
    rc = pHlp->pfnSSMGetU32(pSSM, &pThis->mode.xres);
    AssertRCReturn(rc, rc);
    rc = pHlp->pfnSSMGetU32(pSSM, &pThis->mode.yres);
    AssertRCReturn(rc, rc);
    rc = pHlp->pfnSSMGetU32(pSSM, &pThis->mode.bpp);
    AssertRCReturn(rc, rc);
    rc = pHlp->pfnSSMGetU32(pSSM, &pThis->mode.pitch);
    AssertRCReturn(rc, rc);
    
    /* Load VRAM content. */
    rc = pHlp->pfnSSMGetMem(pSSM, pThis->pbVRAM, pThis->cbVRAM);
    AssertRCReturn(rc, rc);
    
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Device Construction and Destruction                                                                                         *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) geforceR3Destruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);
    
    /* Nothing special to clean up for now. */
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) geforceR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PGEFORCESTATE pThis = PDMDEVINS_2_DATA(pDevIns, PGEFORCESTATE);
    PCPDMDEVHLPR3 pHlp = pDevIns->pHlpR3;
    int rc;
    RT_NOREF(iInstance, pCfg);
    
    /*
     * Initialize the instance data.
     */
    pThis->pDevIns = pDevIns;
    pThis->cbVRAM = GEFORCE_VRAM_SIZE;
    pThis->card_type = 0x35;  /* NV35 = GeForce FX 5900 */
    
    /* Initialize the base interface. */
    pThis->IBase.pfnQueryInterface = geforceQueryInterface;
    
    /* Initialize the display port interface. */
    pThis->IPort.pfnUpdateDisplay = geforcePortUpdateDisplay;
    pThis->IPort.pfnUpdateDisplayAll = geforcePortUpdateDisplayAll;
    pThis->IPort.pfnQueryVideoMode = geforcePortQueryVideoMode;
    pThis->IPort.pfnSetRefreshRate = geforcePortSetRefreshRate;
    pThis->IPort.pfnTakeScreenshot = geforcePortTakeScreenshot;
    
    /*
     * PCI device setup.
     */
    PPDMPCIDEV pPciDev = pDevIns->apPciDevs[0];
    PDMPCIDEV_ASSERT_VALID(pDevIns, pPciDev);
    
    PDMPciDevSetVendorId(pPciDev, GEFORCE_PCI_VENDOR_ID);
    PDMPciDevSetDeviceId(pPciDev, GEFORCE_PCI_DEVICE_ID);
    PDMPciDevSetClassProg(pPciDev, 0x00);
    PDMPciDevSetClassSub(pPciDev, 0x00);     /* VGA compatible */
    PDMPciDevSetClassBase(pPciDev, 0x03);    /* Display controller */
    PDMPciDevSetHeaderType(pPciDev, 0x00);   /* Single function */
    PDMPciDevSetSubSystemVendorId(pPciDev, GEFORCE_PCI_VENDOR_ID);
    PDMPciDevSetSubSystemId(pPciDev, 0x297B); /* GeForce FX 5900 subsystem ID */
    PDMPciDevSetInterruptLine(pPciDev, 0x00);
    PDMPciDevSetInterruptPin(pPciDev, 0x01);
    
    /*
     * Allocate VRAM.
     */
    pThis->pbVRAM = (uint8_t *)RTMemAllocZ(pThis->cbVRAM);
    AssertReturn(pThis->pbVRAM, VERR_NO_MEMORY);
    
    /*
     * Register MMIO region for GeForce registers.
     */
    IOMMMIOHANDLE hMmioRegs;
    rc = PDMDevHlpMmioCreateEx(pDevIns, GEFORCE_PNPMMIO_SIZE, IOMMMIO_FLAGS_READ_DWORD_QWORD | IOMMMIO_FLAGS_WRITE_DWORD_QWORD_ZEROED,
                               pPciDev, 0 /* PCI BAR 0 */, geforceMMIOWrite, geforceMMIORead, NULL /*pfnFill*/,
                               NULL /*pvUser*/, "GeForce MMIO", &hMmioRegs);
    AssertRCReturn(rc, rc);
    
    rc = PDMDevHlpPCIIORegionRegisterMmio(pDevIns, 0, GEFORCE_PNPMMIO_SIZE, PCI_ADDRESS_SPACE_MEM, hMmioRegs,
                                          NULL /*pfnMapUnmap*/);
    AssertRCReturn(rc, rc);
    
    /*
     * Register VRAM as BAR 1.
     */
    rc = PDMDevHlpMmioCreateEx(pDevIns, pThis->cbVRAM, IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                               pPciDev, 1 /* PCI BAR 1 */, geforceMMIOWrite, geforceMMIORead, NULL /*pfnFill*/,
                               NULL /*pvUser*/, "GeForce VRAM", &pThis->hMmio);
    AssertRCReturn(rc, rc);
    
    rc = PDMDevHlpPCIIORegionRegisterMmio(pDevIns, 1, pThis->cbVRAM, PCI_ADDRESS_SPACE_MEM_PREFETCH, pThis->hMmio,
                                          NULL /*pfnMapUnmap*/);
    AssertRCReturn(rc, rc);
    
    /*
     * Register VGA-compatible I/O ports for CRTC access.
     */
    rc = PDMDevHlpIoPortCreateFlagsAndMap(pDevIns, 0x3d4, 1, IOM_IOPORT_F_ABS,
                                          geforceIoPortCrtcIndexWrite, geforceIoPortCrtcIndexRead,
                                          "GeForce CRTC Index", NULL /*paExtDescs*/, &pThis->hIoPortVgaCrt);
    AssertRCReturn(rc, rc);
    
    rc = PDMDevHlpIoPortCreateFlagsAndMap(pDevIns, 0x3d5, 1, IOM_IOPORT_F_ABS,
                                          geforceIoPortCrtcDataWrite, geforceIoPortCrtcDataRead,
                                          "GeForce CRTC Data", NULL /*paExtDescs*/, &pThis->hIoPortVgaCrt);
    AssertRCReturn(rc, rc);
    
    /*
     * Initialize CRTC and default mode.
     */
    geforceInitCrtc(pThis);
    
    /*
     * Try to attach to a display driver.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->IBase, &pThis->pDrvBase, "Display Port");
    if (RT_SUCCESS(rc))
    {
        pThis->pDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIDISPLAYCONNECTOR);
        if (pThis->pDrv)
        {
            LogRel(("GeForce: Display driver attached successfully\n"));
        }
        else
        {
            LogRel(("GeForce: Display driver does not provide PDMIDISPLAYCONNECTOR interface\n"));
        }
    }
    else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
    {
        LogRel(("GeForce: No display driver attached (headless mode)\n"));
        rc = VINF_SUCCESS;
    }
    else
    {
        LogRel(("GeForce: Failed to attach display driver, rc=%Rrc\n", rc));
        return rc;
    }
    
    /*
     * Register saved state handlers.
     */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, 1 /* version */, sizeof(*pThis), NULL,
                                NULL /*pfnLivePrep*/, geforceR3LiveExec, NULL /*pfnLiveDone*/,
                                NULL /*pfnSavePrep*/, geforceR3SaveExec, NULL /*pfnSaveDone*/,
                                NULL /*pfnLoadPrep*/, geforceR3LoadExec, NULL /*pfnLoadDone*/);
    AssertRCReturn(rc, rc);
    
    LogRel(("GeForce: Emulating GeForce FX 5900 (NV35) with %u MB VRAM\n", pThis->cbVRAM / _1M));
    LogRel(("GeForce: PCI device %04X:%04X configured with BARs:\n", GEFORCE_PCI_VENDOR_ID, GEFORCE_PCI_DEVICE_ID));
    LogRel(("GeForce:   BAR 0: MMIO registers (%u MB)\n", GEFORCE_PNPMMIO_SIZE / _1M));
    LogRel(("GeForce:   BAR 1: Video memory (%u MB)\n", pThis->cbVRAM / _1M));
    LogRel(("GeForce: VGA-compatible CRTC accessible via I/O ports 0x3D4/0x3D5\n"));
    
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Device Registration                                                                                                          *
*********************************************************************************************************************************/

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceGeForce =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 GEFORCE_DEV_NAME,
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RZ | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_GRAPHICS,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(GEFORCESTATE),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "NVIDIA GeForce FX 5900 (NV35) Graphics Adapter",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           geforceR3Construct,
    /* .pfnDestruct = */            geforceR3Destruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               NULL,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

#endif /* IN_RING3 */