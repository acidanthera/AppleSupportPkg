/** @file

AppleEventDxe

Copyright (c) 2018, CupertinoNet

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <AppleMacEfi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "AppleEventInternal.h"

// APPLE_EVENT_QUEUE_SIGNATURE
#define APPLE_EVENT_QUEUE_SIGNATURE  SIGNATURE_32 ('A', 'E', 'v', 'Q')

// APPLE_EVENT_QUEUE_FROM_LIST_ENTRY
#define APPLE_EVENT_QUEUE_FROM_LIST_ENTRY(ListEntry) \
  CR ((ListEntry), APPLE_EVENT_QUEUE, Link, APPLE_EVENT_QUEUE_SIGNATURE)

// APPLE_EVENT_QUEUE
typedef struct {
  UINT32                  Signature;     ///<
  LIST_ENTRY              Link;          ///<
  APPLE_EVENT_INFORMATION *Information;  ///<
} APPLE_EVENT_QUEUE;

// mQueueEvent
STATIC EFI_EVENT mQueueEvent = NULL;

// mQueueEventCreated
STATIC BOOLEAN mQueueEventCreated = FALSE;

// mEventQueueList
STATIC LIST_ENTRY mQueue = INITIALIZE_LIST_HEAD_VARIABLE (mQueue);

// mQueueLock
STATIC EFI_LOCK mQueueLock = {
  0,
  0,
  FALSE
};

// InternalSignalAndCloseQueueEvent
VOID
InternalSignalAndCloseQueueEvent (
  VOID
  )
{
  DEBUG ((EFI_D_ERROR, "InternalSignalAndCloseQueueEvent\n"));

  gBS->SignalEvent (mQueueEvent);

  if (mQueueEventCreated && (mQueueEvent != NULL)) {
    gBS->CloseEvent (mQueueEvent);
  }
}

// InternalQueueEventNotifyFunction
STATIC
VOID
EFIAPI
InternalQueueEventNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *EventQueueEntry;
  APPLE_EVENT_QUEUE *EventQueue;

  DEBUG ((EFI_D_ERROR, "InternalQueueEventNotifyFunction\n"));

  if (mQueueEventCreated) {
    do {
      Status = EfiAcquireLockOrFail (&mQueueLock);

      // BUG: Should use EFI_ERROR().
    } while (Status != EFI_SUCCESS);

    InternalFlagAllEventsReady ();

    EventQueueEntry = GetFirstNode (&mQueue);

    while (!IsNull (&mQueue, EventQueueEntry)) {
      EventQueue = APPLE_EVENT_QUEUE_FROM_LIST_ENTRY (EventQueueEntry);

      InternalSingalEvents (EventQueue->Information);

      if (((EventQueue->Information->EventType & APPLE_ALL_KEYBOARD_EVENTS) != 0)
       && (EventQueue->Information->EventData.KeyData != NULL)) {
        FreePool (
          (VOID *)EventQueue->Information->EventData.KeyData
          );
      }

      EventQueueEntry = RemoveEntryList (EventQueueEntry);
      FreePool ((VOID *)EventQueue->Information);
      FreePool ((VOID *)EventQueue);
    }

    InternalRemoveUnregisteredEvents ();
    EfiReleaseLock (&mQueueLock);
  }
}

// InternalCreateQueueEvent
VOID
InternalCreateQueueEvent (
  VOID
  )
{
  EFI_STATUS Status;

  DEBUG ((EFI_D_ERROR, "InternalCreateQueueEvent\n"));

  EfiInitializeLock (&mQueueLock, TPL_NOTIFY);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  InternalQueueEventNotifyFunction,
                  NULL,
                  &mQueueEvent
                  );

  if (!EFI_ERROR (Status)) {
    mQueueEventCreated = TRUE;
  }
}


// EventCreateAppleEventQueueInfo
APPLE_EVENT_INFORMATION *
EventCreateAppleEventQueueInfo (
  IN APPLE_EVENT_DATA    EventData,
  IN APPLE_EVENT_TYPE    EventType,
  IN DIMENSION           *PointerPosition,
  IN APPLE_MODIFIER_MAP  Modifiers
  )
{
  APPLE_EVENT_INFORMATION *QueueInfo;
  EFI_TIME                CreationTime;

  DEBUG ((EFI_D_ERROR, "EventCreateAppleEventQueueInfo\n"));

  QueueInfo = AllocateZeroPool (sizeof (*QueueInfo));

  if (QueueInfo != NULL) {
    gRT->GetTime (&CreationTime, NULL);

    QueueInfo->EventType           = EventType;
    QueueInfo->EventData           = EventData;
    QueueInfo->Modifiers           = Modifiers;
    QueueInfo->CreationTime.Year   = CreationTime.Year;
    QueueInfo->CreationTime.Month  = CreationTime.Month;
    QueueInfo->CreationTime.Day    = CreationTime.Day;
    QueueInfo->CreationTime.Hour   = CreationTime.Hour;
    QueueInfo->CreationTime.Minute = CreationTime.Minute;
    QueueInfo->CreationTime.Second = CreationTime.Second;
    QueueInfo->CreationTime.Pad1   = CreationTime.Pad1;

    if (PointerPosition != NULL) {
      CopyMem (
        (VOID *)&QueueInfo->PointerPosition,
        (VOID *)PointerPosition,
        sizeof (*PointerPosition)
        );
    }
  }

  return QueueInfo;
}

// EventAddEventToQueue
VOID
EventAddEventToQueue (
  IN APPLE_EVENT_INFORMATION  *Information
  )
{
  EFI_STATUS        Status;
  APPLE_EVENT_QUEUE *EventQueue;

  DEBUG ((EFI_D_ERROR, "EventAddEventToQueue\n"));

  if (mQueueEventCreated) {
    do {
      Status = EfiAcquireLockOrFail (&mQueueLock);
    } while (Status != EFI_SUCCESS);

    EventQueue = AllocatePool (sizeof (*EventQueue));

    if (EventQueue != NULL) {
      EventQueue->Signature   = APPLE_EVENT_QUEUE_SIGNATURE;
      EventQueue->Information = Information;

      InsertTailList (&mQueue, &EventQueue->Link);
    }

    EfiReleaseLock (&mQueueLock);
    gBS->SignalEvent (mQueueEvent);
  }
}

// EventCreateEventQueue
EFI_STATUS
EventCreateEventQueue (
  IN APPLE_EVENT_DATA    EventData,
  IN APPLE_EVENT_TYPE    EventType,
  IN APPLE_MODIFIER_MAP  Modifiers
  )
{
  EFI_STATUS                    Status;
  APPLE_EVENT_INFORMATION       *Information;

  DEBUG ((EFI_D_ERROR, "EventCreateEventQueue\n"));

  Status = EFI_INVALID_PARAMETER;

  if (EventData.Raw != 0 || Modifiers != 0) {
    Information = EventCreateAppleEventQueueInfo (
                    EventData,
                    EventType,
                    NULL,
                    Modifiers
                    );

    Status = EFI_OUT_OF_RESOURCES;

    if (Information != NULL) {
      EventAddEventToQueue (Information);

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}
