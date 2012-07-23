#include "BufferedPaint.h"

#include "SoftLinking.h"

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include <wx/dcbuffer.h>
#include <wx/msw/private.h>

typedef enum _BP_BUFFERFORMAT {
  BPBF_COMPATIBLEBITMAP,
  BPBF_DIB,
  BPBF_TOPDOWNDIB,
  BPBF_TOPDOWNMONODIB 
} BP_BUFFERFORMAT;

typedef struct _BP_PAINTPARAMS {
  DWORD               cbSize;
  DWORD               dwFlags;
  const RECT          *prcExclude;
  const BLENDFUNCTION *pBlendFunction;
} BP_PAINTPARAMS, *PBP_PAINTPARAMS;

SOFT_LINK_LIBRARY(Uxtheme);
SOFT_LINK_OPTIONAL(Uxtheme, BufferedPaintInit, HRESULT, WINAPI, ());
SOFT_LINK_OPTIONAL(Uxtheme, BeginBufferedPaint, HPAINTBUFFER, WINAPI, (HDC, const RECT*, BP_BUFFERFORMAT, __in BP_PAINTPARAMS*, __out HDC*));
SOFT_LINK_OPTIONAL(Uxtheme, EndBufferedPaint, HRESULT, WINAPI, (HPAINTBUFFER, BOOL));
SOFT_LINK_OPTIONAL(Uxtheme, BufferedPaintSetAlpha, HRESULT, WINAPI, (HPAINTBUFFER, __in const RECT*, BYTE));

static bool s_didInitBufferedPaint = false;

bool BufferedPaint::hasSystemBuffering()
{
    static bool enabled = BufferedPaintInitPtr() &&
        BeginBufferedPaintPtr() &&
        EndBufferedPaintPtr() &&
        BufferedPaintSetAlphaPtr();

    return enabled;
}

BufferedPaint::BufferedPaint(wxWindow* window, unsigned char alpha)
    : m_window(window)
    , m_bufferDC(0)
    , m_paintDC(0)
    , m_hBufferedPaint(0)
    , m_alpha(alpha)
{
    if (!s_didInitBufferedPaint) {
        s_didInitBufferedPaint = true;
        if (BufferedPaintInitPtr())
            BufferedPaintInitPtr()();
    }
}

BufferedPaint::~BufferedPaint()
{
    if (m_hBufferedPaint) {
        BufferedPaintSetAlphaPtr()(m_hBufferedPaint, 0, m_alpha);
        EndBufferedPaintPtr()(m_hBufferedPaint, true);
        m_hBufferedPaint = 0;
    }

    if (m_bufferDC) {
        delete m_bufferDC;
        m_bufferDC = 0;
    }

    if (m_paintDC) {
        delete m_paintDC;
        m_paintDC = 0;
    }
}

wxDC& BufferedPaint::GetDC()
{
    if (!BufferedPaint::hasSystemBuffering()) {
        m_paintDC = new wxAutoBufferedPaintDC(m_window);
        return *m_paintDC;
    }

    m_paintDC = new wxPaintDC(m_window);
    HDC targetHDC = static_cast<HDC>(m_paintDC->GetHDC());

    wxRect targetRect(m_window->GetClientRect());
    RECT targetRECT;
    wxCopyRectToRECT(targetRect, targetRECT);

    HDC clientHDC;
    m_hBufferedPaint = BeginBufferedPaintPtr()(
        targetHDC,
        &targetRECT,
        BPBF_TOPDOWNDIB,
        0,
        &clientHDC);

    if (!clientHDC)
        wxLogLastError(L"BeginBufferedPaint");

    m_bufferDC = new wxDCTemp(static_cast<WXHDC>(clientHDC), targetRect.GetSize());
    return *m_bufferDC;
}

