/*
 * Copyright (c) 2010 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>
#include <stdio.h>
#include "ddraw.h"

#include "main.h"
#include "palette.h"
#include "surface.h"
#include "clipper.h"

HRESULT __stdcall ddraw_GetCaps(IDirectDrawImpl *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDEmulCaps)
{
    printf("DirectDraw::GetCaps(This=%p, lpDDDriverCaps=%p, lpDDEmulCaps=%p)\n", This, lpDDDriverCaps, lpDDEmulCaps);

    if(lpDDDriverCaps)
    {
        lpDDDriverCaps->dwSize = sizeof(DDCAPS);
        lpDDDriverCaps->dwCaps = DDCAPS_BLT|DDCAPS_PALETTE;
        lpDDDriverCaps->dwCKeyCaps = 0;
        lpDDDriverCaps->dwPalCaps = DDPCAPS_8BIT|DDPCAPS_PRIMARYSURFACE;
        lpDDDriverCaps->dwVidMemTotal = 16777216;
        lpDDDriverCaps->dwVidMemFree = 16777216;
        lpDDDriverCaps->dwMaxVisibleOverlays = 0;
        lpDDDriverCaps->dwCurrVisibleOverlays = 0;
        lpDDDriverCaps->dwNumFourCCCodes = 0;
        lpDDDriverCaps->dwAlignBoundarySrc = 0;
        lpDDDriverCaps->dwAlignSizeSrc = 0;
        lpDDDriverCaps->dwAlignBoundaryDest = 0;
        lpDDDriverCaps->dwAlignSizeDest = 0;
    }

    if(lpDDEmulCaps)
    {
        lpDDEmulCaps->dwSize = 0;
    }

    return DD_OK;
}

HRESULT __stdcall ddraw_RestoreDisplayMode(IDirectDrawImpl *This)
{
    printf("DirectDraw::RestoreDisplayMode(This=%p)\n", This);

    return DD_OK;
}

HRESULT __stdcall ddraw_SetCooperativeLevel(IDirectDrawImpl *This, HWND hWnd, DWORD dwFlags)
{
    printf("DirectDraw::SetCooperativeLevel(This=%p, hWnd=0x%08X, dwFlags=0x%08X)\n", This, (unsigned int)hWnd, (unsigned int)dwFlags);

    /* Red Alert for some weird reason does this on Windows XP */
    if(hWnd == NULL)
    {
        return DDERR_INVALIDPARAMS;
    }

    This->hWnd = hWnd;

    if(IDirectDraw_SetCooperativeLevel(This->real_ddraw, hWnd, DDSCL_NORMAL) != DD_OK)
    {
        printf(" internal SetCooperativeLevel failed\n");
        return DDERR_GENERIC;
    }

    return DD_OK;
}

HRESULT __stdcall ddraw_SetDisplayMode(IDirectDrawImpl *This, DWORD width, DWORD height, DWORD bpp)
{
    printf("DirectDraw::SetDisplayMode(This=%p, width=%d, height=%d, bpp=%d)\n", This, (unsigned int)width, (unsigned int)height, (unsigned int)bpp);

    This->width = width;
    This->height = height;
    This->bpp = bpp;

    MoveWindow(This->hWnd, 0, 0, This->width, This->height, TRUE);

    return DD_OK;
}

HRESULT __stdcall ddraw_QueryInterface(IDirectDrawImpl *This, REFIID riid, void **obj)
{
    printf("DirectDraw::QueryInterface(This=%p, riid=%08X, obj=%p)\n", This, (unsigned int)riid, obj);
    return S_OK;
}

ULONG __stdcall ddraw_AddRef(IDirectDrawImpl *This)
{
    printf("DirectDraw::AddRef(This=%p)\n", This);

    This->Ref++;

    return This->Ref;
}

ULONG __stdcall ddraw_Release(IDirectDrawImpl *This)
{
    printf("DirectDraw::Release(This=%p)\n", This);

    This->Ref--;

    if(This->Ref == 0)
    {
        IDirectDraw_Release(This->real_ddraw);
        free(This);
        return 0;
    }

    return This->Ref;
}

HRESULT __stdcall null(void *This)
{
    printf("Warning: null method called for instance %p!\n", This);
    fflush(NULL);
    return DDERR_UNSUPPORTED;
}

struct IDirectDrawImplVtbl iface =
{
    /* IUnknown */
    ddraw_QueryInterface,
    ddraw_AddRef,
    ddraw_Release,
    /* IDirectDrawImpl */
    null, //Compact,
    ddraw_CreateClipper,
    ddraw_CreatePalette,
    ddraw_CreateSurface,
    null, //DuplicateSurface,
    null, //EnumDisplayModes,
    null, //EnumSurfaces,
    null, //FlipToGDISurface,
    ddraw_GetCaps,
    null, //GetDisplayMode,
    null, //GetFourCCCodes,
    null, //GetGDISurface,
    null, //GetMonitorFrequency,
    null, //GetScanLine,
    null, //GetVerticalBlankStatus,
    null, //Initialize,
    ddraw_RestoreDisplayMode,
    ddraw_SetCooperativeLevel,
    ddraw_SetDisplayMode,
    null  //WaitForVerticalBlank
};

int stdout_open = 0;
HRESULT WINAPI DirectDrawCreate(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter) 
{
#if _DEBUG
    if(!stdout_open)
    {
        freopen("stdout.txt", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0);
        stdout_open = 1;
    }
#endif

    printf("DirectDrawCreate(lpGUID=%p, lplpDD=%p, pUnkOuter=%p)\n", lpGUID, lplpDD, pUnkOuter);

    IDirectDrawImpl *This = (IDirectDrawImpl *)HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawImpl));
    This->lpVtbl = &iface;
    This->hWnd = NULL;
    printf(" This = %p\n", This);
    *lplpDD = (LPDIRECTDRAW)This;

    This->real_dll = LoadLibrary("system32\\ddraw.dll");
    if(!This->real_dll)
    {
        return DDERR_UNSUPPORTED;
    }

    This->real_DirectDrawCreate = (HRESULT WINAPI (*)(GUID FAR*, LPDIRECTDRAW FAR*, IUnknown FAR*))GetProcAddress(This->real_dll, "DirectDrawCreate");
    if(This->real_DirectDrawCreate(NULL, &This->real_ddraw, NULL) != DD_OK)
    {
        return DDERR_UNSUPPORTED;
    }

    This->Ref = 0;
    ddraw_AddRef(This);

    return DD_OK;
}
