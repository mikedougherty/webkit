/*
 * Copyright (C) 2007 Apple Computer, Kevin Ollivier.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Image.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"

#include <math.h>
#include <stdio.h>

#include <wx/defs.h>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include <wx/image.h>
#include <wx/thread.h>
#include <wx/rawbmp.h>

#include <windows.h>
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#include <gdiplus.h>
using namespace Gdiplus;

namespace WebCore {

// this is in GraphicsContextWx.cpp
int getWxCompositingOperation(CompositeOperator op, bool hasAlpha);

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}

// ================================================
// Image Class
// ================================================

PassRefPtr<Image> Image::loadPlatformResource(const char *name)
{
    // FIXME: We need to have some 'placeholder' graphics for things like missing
    // plugins or broken images.
    Vector<char> arr;
    RefPtr<Image> img = BitmapImage::create();
    RefPtr<SharedBuffer> buffer = SharedBuffer::create(arr.data(), arr.size());
    img->setData(buffer, true);
    return img.release();
}

void BitmapImage::initPlatformData()
{
    m_cachedBitmap = 0;
    m_cachedDstSize = FloatSize(0, 0);
    m_cachedSrc = FloatRect(0, 0, 0, 0);
}

// Drawing Routines
//
#ifdef __WXMSW__
#define PREMULTIPLY(p, a)   ((p) * (a) / 0xff)
static void premultiplyAlpha(wxBitmap& bitmap)
{
    const int width = bitmap.GetWidth();
    const int height = bitmap.GetHeight();

    wxAlphaPixelData pixData(bitmap, wxPoint(0,0), wxSize(width, height));
    wxAlphaPixelData::Iterator p(pixData);
    for (int y=0; y<height; y++) {
        wxAlphaPixelData::Iterator rowStart = p;
        for (int x=0; x<width; x++) {
            if (p.Red() || p.Green() || p.Blue() || p.Alpha()) {
                byte a = p.Alpha();
                p.Red() = PREMULTIPLY(p.Red(), a);
                p.Green() = PREMULTIPLY(p.Green(), a);
                p.Blue() = PREMULTIPLY(p.Blue(), a);
            }
            ++p; 
        }
        p = rowStart;
        p.OffsetY(pixData, 1);
    }
}

wxBitmap* gdiplusResize(wxBitmap* bitmap, const FloatSize& size)
{
    IntSize intSize = IntSize(ceilf(size.width()), ceilf(size.height()));
    if (intSize.width() == bitmap->GetWidth() && intSize.height() == bitmap->GetHeight())
        return new wxBitmap(*bitmap); // just return a copy if the asked for the same size

    wxBitmap* newBitmap = new wxBitmap(intSize.width(), intSize.height(), 32);
    newBitmap->UseAlpha();

    wxMemoryDC memdc(*newBitmap);
    wxGraphicsContext* gc = wxGraphicsContext::Create(memdc);
    wxGraphicsBitmap gcbitmap(gc->CreateBitmap(*bitmap));

    Graphics* graphics = (Graphics*)gc->GetNativeContext();
    graphics->Clear(Gdiplus::Color(0, 0, 0, 0));

    // Only use HighQualityBicubic when downsampling.
    InterpolationMode mode;
    if (size.width() < (int)bitmap->GetWidth() && size.height() < (int)bitmap->GetHeight())
        mode = InterpolationModeHighQualityBicubic;
    else
        mode = InterpolationModeBicubic;

    graphics->SetInterpolationMode(mode);
    graphics->DrawImage((Bitmap*)gcbitmap.GetNativeBitmap(), 0.0, 0.0, size.width(), size.height());
    
    premultiplyAlpha(*newBitmap);

    delete gc;

    return newBitmap;
}
#endif

wxBitmap* BitmapImage::getCachedResizedBitmap(wxBitmap* bitmap, const FloatSize& dstSize, const FloatRect& src) const
{
    if (dstSize.width() == bitmap->GetWidth() &&
        dstSize.height() == bitmap->GetHeight() &&
        src.x() == 0 &&
        src.y() == 0 &&
        src.width() == bitmap->GetWidth() &&
        src.height() == bitmap->GetHeight())
        return bitmap;

    if (m_cachedBitmap) {
        if (m_frameCount == 1 &&
            m_cachedDstSize == dstSize &&
            m_cachedSrc == src) {
            return m_cachedBitmap;
        } else {
            delete m_cachedBitmap;
            m_cachedBitmap = 0;
        }
    }

    wxBitmap subBitmap(bitmap->GetSubBitmap(IntRect(src)));

#ifdef __WXMSW__
    // avoid wxImage::Rescale's blurry bicubic resampling on windows
    // by using Gdiplus
    m_cachedBitmap = gdiplusResize(&subBitmap, dstSize);
#else
    wxImage img(subBitmap.ConvertToImage());
    img.Rescale(size.width(), size.height(), wxIMAGE_QUALITY_HIGH);
    m_cachedBitmap = new wxBitmap(img);
#endif

    m_cachedDstSize = dstSize;
    m_cachedSrc = src;

    return m_cachedBitmap;
}

void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op)
{
    if (!m_source.initialized())
        return;

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, dst, solidColor(), styleColorSpace, op);
        return;
    }

#if USE(WXGC)
    wxGCDC* context = (wxGCDC*)ctxt->platformContext();
    wxGraphicsContext* gc = context->GetGraphicsContext();
    wxGraphicsBitmap* bitmap = frameAtIndex(m_currentFrame);
#else
    wxDC* context = ctxt->platformContext();
    wxBitmap* bitmap = frameAtIndex(m_currentFrame);
#endif

    startAnimation();
    if (!bitmap) // If it's too early we won't have an image yet.
        return;
    
    // If we're drawing a sub portion of the image or scaling then create
    // a pattern transformation on the image and draw the transformed pattern.
    // Test using example site at http://www.meyerweb.com/eric/css/edge/complexspiral/demo.html
    // FIXME: NYI
   
    ctxt->save();

    // Set the compositing operation.
    ctxt->setCompositeOperation(op);
    
#if USE(WXGC)
    float scaleX = src.width() / dst.width();
    float scaleY = src.height() / dst.height();

    FloatRect adjustedDestRect = dst;
    FloatSize selfSize = currentFrameSize();
    
    if (src.size() != selfSize) {
        adjustedDestRect.setLocation(FloatPoint(dst.x() - src.x() / scaleX, dst.y() - src.y() / scaleY));
        adjustedDestRect.setSize(FloatSize(selfSize.width() / scaleX, selfSize.height() / scaleY));
    }

    gc->Clip(dst.x(), dst.y(), dst.width(), dst.height());
#if wxCHECK_VERSION(2,9,0)
    gc->DrawBitmap(*bitmap, adjustedDestRect.x(), adjustedDestRect.y(), adjustedDestRect.width(), adjustedDestRect.height());
#else
    gc->DrawGraphicsBitmap(*bitmap, adjustedDestRect.x(), adjustedDestRect.y(), adjustedDestRect.width(), adjustedDestRect.height());
#endif

#else // USE(WXGC)
    bitmap = getCachedResizedBitmap(bitmap, dst.size(), src);
    context->DrawBitmap(*bitmap, dst.x(), dst.y(), true);
#endif

    ctxt->restore();

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace, CompositeOperator, const FloatRect& dstRect)
{
#if USE(WXGC)
    wxGCDC* context = (wxGCDC*)ctxt->platformContext();
    wxGraphicsBitmap* bitmap = nativeImageForCurrentFrame();
#else
    wxDC* context = ctxt->platformContext();
    wxBitmap* bitmap = nativeImageForCurrentFrame();
#endif

    if (!bitmap) // If it's too early we won't have an image yet.
        return;

    ctxt->save();
    ctxt->clip(IntRect(dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height()));
    
    float currentW = 0;
    float currentH = 0;
    
#if USE(WXGC)
    wxGraphicsContext* gc = context->GetGraphicsContext();

    float adjustedX = phase.x() + srcRect.x() *
                      narrowPrecisionToFloat(patternTransform.a());
    float adjustedY = phase.y() + srcRect.y() *
                      narrowPrecisionToFloat(patternTransform.d());
                      
    gc->ConcatTransform(patternTransform);
#else
    float adjustedX = phase.x();
    float adjustedY = phase.y();
    wxMemoryDC mydc;
    mydc.SelectObject(*bitmap);

    ctxt->concatCTM(patternTransform);
#endif

    //wxPoint origin(context->GetDeviceOrigin());
    AffineTransform mat(ctxt->getCTM());
    wxPoint origin(mat.mapPoint(IntPoint(0, 0)));
    wxSize clientSize(context->GetSize());

    wxCoord w = srcRect.width();
    wxCoord h = srcRect.height();
    wxCoord srcx = srcRect.x();
    wxCoord srcy = srcRect.y();

    while (currentW < dstRect.right() - phase.x() && origin.x + adjustedX + currentW < clientSize.x) {
        while (currentH < dstRect.bottom() - phase.y() && origin.y + adjustedY + currentH < clientSize.y) {
#if USE(WXGC)
#if wxCHECK_VERSION(2,9,0)
            gc->DrawBitmap(*bitmap, adjustedX + currentW, adjustedY + currentH, (wxDouble)srcRect.width(), (wxDouble)srcRect.height());
#else
            gc->DrawGraphicsBitmap(*bitmap, adjustedX + currentW, adjustedY + currentH, (wxDouble)srcRect.width(), (wxDouble)srcRect.height());
#endif
#else
            context->Blit(adjustedX + currentW, adjustedY + currentH,
                          w, h, &mydc, srcx, srcy, wxCOPY, true);
#endif
            currentH += srcRect.height();
        }
        currentW += srcRect.width();
        currentH = 0;
    }
    ctxt->restore();

#if !USE(WXGC)
    mydc.SelectObject(wxNullBitmap);
#endif    
    
    // NB: delete is causing crashes during page load, but not during the deletion
    // itself. It occurs later on when a valid bitmap created in frameAtIndex
    // suddenly becomes invalid after returning. It's possible these errors deal
    // with reentrancy and threding problems.
    //delete bitmap;

    startAnimation();

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void BitmapImage::checkForSolidColor()
{
    if (m_checkedForSolidColor)
        return;

    m_checkedForSolidColor = true;
    m_isSolidColor = false;

    if (frameCount() != 1 )
        return;

    const int width = currentFrameSize().width();
    const int height = currentFrameSize().height();

    if (width > 10 || height > 10)
        return;

#if !USE(WXGC)
    // wxBitmap check: use PixelData class from wx/rawbmp.h
    wxBitmap* bitmap = frameAtIndex(0);

    typedef wxPixelData<wxBitmap, wxAlphaPixelFormat> PixelData;

    PixelData data(*bitmap);
    if (!data)
        return;

    PixelData::Iterator p(data);
    wxColour lastColor(p.Red(), p.Green(), p.Blue(), p.Alpha());

    for (int y = 0; y < height; ++y) {
        PixelData::Iterator rowStart = p;
        for (int x = 0; x < width; ++x, ++p) {
            wxColour color(p.Red(), p.Green(), p.Blue(), p.Alpha());
            if (color != lastColor)
                return;
            else
                lastColor = color;
        }
        p = rowStart;
        p.OffsetY(data, 1);
    }

    // every pixel was the same color
    m_isSolidColor = true;
    m_solidColor = lastColor;
#endif
}

void BitmapImage::invalidatePlatformData()
{
    if (m_cachedBitmap) {
        delete m_cachedBitmap;
        m_cachedBitmap = 0;
    }
}

}
