/*******************************************************************************
Copyright (C) 2016 Marvell International Ltd.

Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Marvell nor the names of its contributors may be
  used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/Spi.h>

#define SPI_MASTER_SIGNATURE                      SIGNATURE_32 ('M', 'S', 'P', 'I')
#define SPI_MASTER_FROM_SPI_MASTER_PROTOCOL(a)  CR (a, SPI_MASTER, SpiMasterProtocol, SPI_MASTER_SIGNATURE)



#define SPI_CS_LEN_LONG                 (1 << 25)
#define SPI_CS_DMA_LEN                  (1 << 24)
#define SPI_CS_CSPOL2                   (1 << 23)
#define SPI_CS_CSPOL1                   (1 << 22)
#define SPI_CS_CSPOL0                   (1 << 21)
#define SPI_CS_RXF                      (1 << 20)
#define SPI_CS_RXR                      (1 << 19)
#define SPI_CS_TXD                      (1 << 18)
#define SPI_CS_RXD                      (1 << 17)
#define SPI_CS_DONE                     (1 << 16)
#define SPI_CS_TE_EN                    (1 << 15)
#define SPI_CS_LMONO                    (1 << 14)
#define SPI_CS_LEN                      (1 << 13)
#define SPI_CS_REN                      (1 << 12)
#define SPI_CS_ADCS                     (1 << 11)
#define SPI_CS_INTR                     (1 << 10)
#define SPI_CS_INTD                     (1 << 9)
#define SPI_CS_DMAEN                    (1 << 8)
#define SPI_CS_TA                       (1 << 7)
#define SPI_CS_CSPOL                    (1 << 6)
#define SPI_CS_CLEAR_TX                 (1 << 5)
#define SPI_CS_CLEAR_RX                 (1 << 4)
#define SPI_CS_CPOL                     (1 << 3)
#define SPI_CS_CPHA                     (1 << 2)

#define SPI_CS_UNDEFINED                (3)
#define SPI_CS_CS2                      (2)
#define SPI_CS_CS1                      (1)
#define SPI_CS_CS0                      (0)

#define SPI_TRANSFER_BEGIN              0x01  // Assert CS before transfer
#define SPI_TRANSFER_END                0x02  // Deassert CS after transfers

#define SPI_TIMEOUT                     100000

typedef struct {
  BCM283X_SPI_MASTER_PROTOCOL SpiMasterProtocol;
  UINTN                   Signature;
  EFI_HANDLE              Handle;
  EFI_LOCK                Lock;
} SPI_MASTER;

EFI_STATUS
EFIAPI
Bcm283xSpiTransfer (
  IN BCM283X_SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE *Slave,
  IN UINTN DataByteCount,
  IN VOID *DataOut,
  IN VOID *DataIn,
  IN UINTN Flag
  );

EFI_STATUS
EFIAPI
Bcm283xSpiReadWrite (
  IN  BCM283X_SPI_MASTER_PROTOCOL *This,
  IN  SPI_DEVICE *Slave,
  IN  UINT8 *Cmd,
  IN  UINTN CmdSize,
  IN  UINT8 *DataOut,
  OUT UINT8 *DataIn,
  IN  UINTN DataSize
  );

EFI_STATUS
EFIAPI
Bcm283xSpiInit (
  IN BCM283X_SPI_MASTER_PROTOCOL     * This
  );

SPI_DEVICE *
EFIAPI
Bcm283xSpiSetupSlave (
  IN BCM283X_SPI_MASTER_PROTOCOL     * This,
  IN SPI_DEVICE *Slave,
  IN INTN Controller,
  IN UINTN Cs,
  IN SPI_MODE Mode
  );

EFI_STATUS
EFIAPI
Bcm283xSpiFreeSlave (
  IN SPI_DEVICE *Slave
  );

EFI_STATUS
EFIAPI
SpiMasterEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif // __SPI_MASTER_H__
