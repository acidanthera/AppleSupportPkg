/** @file

AppleEventLib

Copyright (c) 2018, Download-Fritz

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <AppleMacEfi.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

// EventLibCreateTimerEvent
EFI_EVENT
EventLibCreateTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic,
  IN EFI_TPL           NotifyTpl
  )
{
  EFI_EVENT  Event;
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventLibCreateTimerEvent\n"));

  Event = NULL;

  if (NotifyTpl >= TPL_CALLBACK) {
    Status = gBS->CreateEvent (
                    ((NotifyFunction != NULL)
                      ? (EVT_TIMER | EVT_NOTIFY_SIGNAL)
                      : EVT_TIMER),
                    NotifyTpl,
                    NotifyFunction,
                    NotifyContext,
                    &Event
                    );

    if (!EFI_ERROR (Status)) {
      Status = gBS->SetTimer (
                      Event,
                      (SignalPeriodic ? TimerPeriodic : TimerRelative),
                      TriggerTime
                      );

      if (EFI_ERROR (Status)) {
        gBS->CloseEvent (Event);

        Event = NULL;
      }
    }
  }

  return Event;
}

// EventLibCreateNotifyTimerEvent
EFI_EVENT
EventLibCreateNotifyTimerEvent (
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN VOID              *NotifyContext,
  IN UINT64            TriggerTime,
  IN BOOLEAN           SignalPeriodic
  )
{
  DEBUG ((DEBUG_VERBOSE, "EventLibCreateNotifyTimerEvent\n"));

  return EventLibCreateTimerEvent (
           NotifyFunction,
           NotifyContext,
           TriggerTime,
           SignalPeriodic,
           TPL_NOTIFY
           );
}

// EventLibCancelEvent
VOID
EventLibCancelEvent (
  IN EFI_EVENT  Event
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_VERBOSE, "EventLibCancelEvent\n"));

  Status = gBS->SetTimer (Event, TimerCancel, 0);

  if (!EFI_ERROR (Status)) {
    gBS->CloseEvent (Event);
  }
}
