/**
 * Hash service fix for AMI EFIs with broken SHA implementations.
 *
 * Forcibly reinstalls EFI_HASH_PROTOCOL with working MD5, SHA-1,
 * SHA-256 implementations.
 *
 * Author: Joel Höner <athre0z@zyantific.com>
 */

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Hash.h>

#include "HashServices.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

STATIC EFI_SERVICE_BINDING_PROTOCOL mHashBindingProto = {
  &HSCreateChild,
  &HSDestroyChild
};

EFI_STATUS
EFIAPI
HSGetHashSize (
  IN  CONST EFI_HASH_PROTOCOL *This,
  IN  CONST EFI_GUID          *HashAlgorithm,
  OUT UINTN                   *HashSize
  )
{
  if (!HashAlgorithm || !HashSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (&gEfiHashAlgorithmMD5Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_MD5_HASH);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha1Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_SHA1_HASH);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha256Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_SHA256_HASH);
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HSHash (
  IN CONST EFI_HASH_PROTOCOL  *This,
  IN CONST EFI_GUID           *HashAlgorithm,
  IN BOOLEAN                  Extend,
  IN CONST UINT8              *Message,
  IN UINT64                   MessageSize,
  IN OUT EFI_HASH_OUTPUT      *Hash
  )
{
  HS_PRIVATE_DATA  *PrivateData;
  HS_CONTEXT_DATA  CtxCopy;

  if (!This || !HashAlgorithm || !Message || !Hash || !MessageSize) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = HS_PRIVATE_FROM_PROTO (This);

  if (CompareGuid (&gEfiHashAlgorithmMD5Guid, HashAlgorithm)) {
    if (!Extend) {
      md5_init (&PrivateData->Ctx.Md5);
    }

    md5_update (&PrivateData->Ctx.Md5, Message, MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    md5_final (&CtxCopy.Md5, *Hash->Md5Hash);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha1Guid, HashAlgorithm)) {
    if (!Extend) {
      sha1_init (&PrivateData->Ctx.Sha1);
    }

    sha1_update (&PrivateData->Ctx.Sha1, Message, MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    sha1_final (&CtxCopy.Sha1, *Hash->Sha1Hash);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha256Guid, HashAlgorithm)) {
    if (!Extend) {
      sha256_init (&PrivateData->Ctx.Sha256);
    }

    sha256_update (&PrivateData->Ctx.Sha256, Message, MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    sha256_final (&CtxCopy.Sha256, *Hash->Sha256Hash);
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HSCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  HS_PRIVATE_DATA  *PrivateData;
  EFI_STATUS       Status;

  PrivateData = AllocateZeroPool (sizeof (*PrivateData));
  if (!PrivateData) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature         = HS_PRIVATE_SIGNATURE;
  PrivateData->Proto.GetHashSize = HSGetHashSize;
  PrivateData->Proto.Hash        = HSHash;

  Status = gBS->InstallProtocolInterface (
    ChildHandle,
    &gEfiHashProtocolGuid,
    EFI_NATIVE_INTERFACE,
    &PrivateData->Proto
    );

  if (EFI_ERROR (Status)) {
    FreePool (PrivateData);
  }

  return Status;
}

EFI_STATUS
EFIAPI
HSDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                   ChildHandle
  )
{
  EFI_HASH_PROTOCOL *HashProto;
  EFI_STATUS        Status;

  if (!This || !ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->HandleProtocol (
    ChildHandle,
    &gEfiHashProtocolGuid,
    (VOID **) &HashProto
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (
    ChildHandle,
    &gEfiHashProtocolGuid,
    HashProto
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreePool (HS_PRIVATE_FROM_PROTO (HashProto));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeHashServices (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      *OriginalHandles  = NULL;
  VOID            *OriginalProto    = NULL;
  UINTN           Index             = 0;
  UINTN           HandleBufferSize  = 0;
  EFI_HANDLE      NewHandle         = NULL;

  //
  // Uninstall all the existing protocol instances (which are not to be trusted).
  //
  HandleBufferSize = 0;

  Status = gBS->LocateHandle (
    ByProtocol,
    &gEfiHashServiceBindingProtocolGuid,
    NULL,
    &HandleBufferSize,
    NULL
    );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    OriginalHandles = (EFI_HANDLE *) AllocateZeroPool (HandleBufferSize);

    if (!OriginalHandles) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < HandleBufferSize / sizeof (EFI_HANDLE); Index++) {
      Status = gBS->HandleProtocol (
        OriginalHandles[Index],
        &gEfiHashServiceBindingProtocolGuid,
        &OriginalProto
        );

      if (EFI_ERROR (Status)) {
        break;
      }

      Status = gBS->UninstallProtocolInterface (
        OriginalHandles[Index],
        &gEfiHashServiceBindingProtocolGuid,
        OriginalProto
        );

      if (EFI_ERROR (Status)) {
        break;
      }
    }

    FreePool ((VOID *) OriginalHandles);
  }

  //
  // Install our own protocol binding
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gEfiHashServiceBindingProtocolGuid,
    &mHashBindingProto,
    NULL
    );

  return Status;
}
