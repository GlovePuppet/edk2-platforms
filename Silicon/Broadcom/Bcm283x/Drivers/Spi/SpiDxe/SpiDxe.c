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
#include "SpiDxe.h"

SPI_MASTER *mSpiMasterInstance;

STATIC
EFI_STATUS
SpiSetBaudRate (
  IN SPI_DEVICE *Slave,
  IN UINT32 CpuClock,
  IN UINT32 MaxFreq
  )
{
  UINTN  SpiRegBase = Slave->HostRegisterBaseAddress;
  MmioWrite32 (SpiRegBase + SPI_CLK_REG, 0);    /* Slowest possible speed - for now */
  return EFI_SUCCESS;
}

STATIC
VOID
SpiActivateCs (
  IN SPI_DEVICE *Slave
  )
{
  UINT32 Reg;
  UINTN  SpiRegBase = Slave->HostRegisterBaseAddress;

  Reg = MmioRead32 (SpiRegBase + SPI_CS_REG);
  /* Cs must be in the range 0-2 */
  Reg |= Slave->Cs;
  MmioWrite32 (SpiRegBase + SPI_CS_REG, Reg);
}

STATIC
VOID
SpiDeactivateCs (
  IN SPI_DEVICE *Slave
  )
{
  UINT32 Reg;
  UINTN  SpiRegBase = Slave->HostRegisterBaseAddress;
  /* Use the undefined value to de-assert all CS */
  Reg = MmioRead32 (SpiRegBase + SPI_CS_REG);
  Reg |= SPI_CS_UNDEFINED;
  MmioWrite32 (SpiRegBase + SPI_CS_REG, Reg);
}

STATIC
VOID
SpiSetupTransfer (
  IN BCM283X_SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE *Slave
  )
{
  SPI_MASTER *SpiMaster;
  UINT32 Reg, CoreClock, SpiMaxFreq;
  UINTN SpiRegBase;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);

  // Initialize values from PCDs
  SpiRegBase  = Slave->HostRegisterBaseAddress;

  CoreClock   = Slave->CoreClock;
  SpiMaxFreq  = Slave->MaxFreq;
  EfiAcquireLock (&SpiMaster->Lock);

  SpiSetBaudRate (Slave, CoreClock, SpiMaxFreq);

  Reg = MmioRead32 (SpiRegBase + SPI_CS_REG);

  /* Cs must be in the range 0-2 */
  /* Here we assume CS is active low */
  Reg &= ~(SPI_CS_CSPOL0 << Slave->Cs);

  /* Clear the mode bits */
  Reg &= ~(SPI_CS_CPOL | SPI_CS_CPHA);

  switch (Slave->Mode) {
  case SPI_MODE0:
    break;
  case SPI_MODE1:
    Reg |= SPI_CS_CPHA;
    break;
  case SPI_MODE2:
    Reg |= SPI_CS_CPOL;
    break;
  case SPI_MODE3:
    Reg |= SPI_CS_CPOL;
    Reg |= SPI_CS_CPHA;
    break;
  }

  MmioWrite32 (SpiRegBase + SPI_CS_REG, Reg);

  EfiReleaseLock (&SpiMaster->Lock);
}

EFI_STATUS
EFIAPI
Bcm283xSpiTransfer (
  IN BCM283X_SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE *Slave,
  IN UINTN DataByteCount,
  IN VOID *DataOut,
  IN VOID *DataIn,
  IN UINTN Flag
  )
{
  SPI_MASTER *SpiMaster;
  UINTN   Length;
  UINT32  Iterator;
  UINT8   *DataOutPtr = (UINT8 *)DataOut;
  UINT8   *DataInPtr  = (UINT8 *)DataIn;
  UINT8   DataToSend  = 0;
  UINTN   SpiRegBase;

  SpiMaster = SPI_MASTER_FROM_SPI_MASTER_PROTOCOL (This);

  SpiRegBase = Slave->HostRegisterBaseAddress;

  Length = DataByteCount;

  if (!EfiAtRuntime ()) {
    EfiAcquireLock (&SpiMaster->Lock);
  }

  if (Flag & SPI_TRANSFER_BEGIN) {
    SpiActivateCs (Slave);
  }

  while (Length > 0) {
    if (DataOut != NULL) {
      DataToSend = *DataOutPtr & 0xFF;
    }

    Iterator = 0;
    /* Transmit Data - space available in FIFO? */
    if (MmioRead32 (SpiRegBase + SPI_CS_REG) & SPI_CS_TXD) {

      MmioWrite32 (SpiRegBase + SPI_FIFO_REG, DataToSend);

      /* Wait for RX FIFO not empty */
      while(((MmioRead32 (SpiRegBase + SPI_CS_REG) & SPI_CS_RXD) == 0) && (Iterator < SPI_TIMEOUT)) {
        Iterator++;
      }
      if (Iterator >= SPI_TIMEOUT) {
        DEBUG ((DEBUG_ERROR, "%a: Timeout\n", __FUNCTION__));
        return EFI_TIMEOUT;
      }

      if (DataInPtr != NULL) {
        *DataInPtr = MmioRead32 (SpiRegBase + SPI_FIFO_REG);
        DataInPtr++;
      }

      if (DataOutPtr != NULL) {
        DataOutPtr++;
      }
      Length--;
    }
  }

  if (Flag & SPI_TRANSFER_END) {
    SpiDeactivateCs (Slave);
  }

  if (!EfiAtRuntime ()) {
    EfiReleaseLock (&SpiMaster->Lock);
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS Status;
  Status = Bcm283xSpiTransfer (This, Slave, CmdSize, Cmd, NULL, SPI_TRANSFER_BEGIN);
  if (EFI_ERROR (Status)) {
    Print (L"Spi Transfer Error\n");
    return EFI_DEVICE_ERROR;
  }

  Status = Bcm283xSpiTransfer (This, Slave, DataSize, DataOut, DataIn, SPI_TRANSFER_END);
  if (EFI_ERROR (Status)) {
    Print (L"Spi Transfer Error\n");
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Bcm283xSpiInit (
  IN BCM283X_SPI_MASTER_PROTOCOL * This
  )
{

  return EFI_SUCCESS;
}

SPI_DEVICE *
EFIAPI
Bcm283xSpiSetupSlave (
  IN BCM283X_SPI_MASTER_PROTOCOL *This,
  IN SPI_DEVICE *Slave,
  IN UINTN Cs,
  IN SPI_MODE Mode
  )
{
  if (!Slave) {
    Slave = AllocateZeroPool (sizeof(SPI_DEVICE));
    if (Slave == NULL) {
      DEBUG((DEBUG_ERROR, "Cannot allocate memory\n"));
      return NULL;
    }

    Slave->Cs   = Cs;
    Slave->Mode = Mode;
  }

  Slave->HostRegisterBaseAddress = PcdGet32 (PcdSpiRegBase);
  Slave->CoreClock = PcdGet32 (PcdSpiClockFrequency);
  Slave->MaxFreq = PcdGet32 (PcdSpiMaxFrequency);

  SpiSetupTransfer (This, Slave);

  return Slave;
}

EFI_STATUS
EFIAPI
Bcm283xSpiFreeSlave (
  IN SPI_DEVICE *Slave
  )
{
  FreePool (Slave);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Bcm283xSpiConfigRuntime (
  IN SPI_DEVICE *Slave
  )
{
  /* Nothing to do here - memmapping done in RaspberryPiMem.c */
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SpiMasterInitProtocol (
  IN BCM283X_SPI_MASTER_PROTOCOL *SpiMasterProtocol
  )
{

  SpiMasterProtocol->Init        = Bcm283xSpiInit;
  SpiMasterProtocol->SetupDevice = Bcm283xSpiSetupSlave;
  SpiMasterProtocol->FreeDevice  = Bcm283xSpiFreeSlave;
  SpiMasterProtocol->Transfer    = Bcm283xSpiTransfer;
  SpiMasterProtocol->ReadWrite   = Bcm283xSpiReadWrite;
  SpiMasterProtocol->ConfigRuntime = Bcm283xSpiConfigRuntime;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Bcm283xSpiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;

  mSpiMasterInstance = AllocateRuntimeZeroPool (sizeof (SPI_MASTER));
  if (mSpiMasterInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock (&mSpiMasterInstance->Lock, TPL_NOTIFY);

  SpiMasterInitProtocol (&mSpiMasterInstance->SpiMasterProtocol);

  mSpiMasterInstance->Signature = SPI_MASTER_SIGNATURE;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &(mSpiMasterInstance->Handle),
                  &gBcm283xSpiMasterProtocolGuid,
                  &(mSpiMasterInstance->SpiMasterProtocol),
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiMasterInstance);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
