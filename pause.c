#include "pause.h"

void pause(EFI_SYSTEM_TABLE* SystemTable)
{
	// empty the console input buffer
	SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);

	// wait for a key
	UINTN Event;
	SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Event);
}
