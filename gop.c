// https://blog.fpmurphy.com/2015/05/check-available-text-and-graphics-modes-from-uefi-shell.html
#include "gop.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL*
GetGOP()
{
    EFI_HANDLE* HandleBuffer = NULL;
    UINTN HandleCount = 0;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop;


    // get from ConsoleOutHandle?
    Status = gBS->HandleProtocol(gST->ConsoleOutHandle,
        &gEfiGraphicsOutputProtocolGuid,
        (VOID**)&Gop);
    if (EFI_ERROR(Status)) {
        // Print(L"No GOP handle found via HandleProtocol\n");
    }
    else {
        return Gop;
    }

    // try locating directly
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
        NULL,
        (VOID**)&Gop);
    if (EFI_ERROR(Status) || Gop == NULL) {
        // Print(L"No GOP handle found via LocateProtocol\n");
    }
    else {
        return Gop;
    }

    // try locating by handle
    Status = gBS->LocateHandleBuffer(ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer);
    if (EFI_ERROR(Status)) {
        // Print(L"No GOP handles found via LocateHandleBuffer\n");
    }
    else {
        // Print(L"Found %d GOP handles via LocateHandleBuffer\n", HandleCount);

        for (int i = 0; i < HandleCount; i++) {
            Status = gBS->HandleProtocol(HandleBuffer[i],
                &gEfiGraphicsOutputProtocolGuid,
                (VOID*)&Gop);
            if (!EFI_ERROR(Status)) {
                break;
            }
        }
        
        FreePool(HandleBuffer);
        return Gop;
    }

    return NULL;
}
