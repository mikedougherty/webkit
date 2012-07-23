/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#include "Screen.h"
#include "IntRect.h"
#include "FloatRect.h"
#include "HostWindow.h"
#include "ScrollView.h"
#include "Widget.h"

#include <wx/defs.h>
#include <wx/display.h>
#include <wx/gdicmn.h>

namespace WebCore {

FloatRect scaleScreenRectToWidget(FloatRect rect, Widget*)
{
    return rect;
}

FloatRect scaleWidgetRectToScreen(FloatRect rect, Widget*)
{
    return rect;
}

static int displayForWidget(Widget* widget)
{
    return wxDisplay::GetFromWindow(widget->root()->hostWindow()->platformPageClient());
}

FloatRect screenRect(Widget* widget)
{
    int displayNum = displayForWidget(widget);
    if (displayNum != wxNOT_FOUND)
        return wxDisplay(displayNum).GetGeometry();

    return FloatRect();
}

int screenDepth(Widget* widget)
{
    return wxDisplayDepth();
}

int screenDepthPerComponent(Widget*)
{
    return wxDisplayDepth();
}

bool screenIsMonochrome(Widget* widget)
{
    return wxColourDisplay();
}

FloatRect screenAvailableRect(Widget* widget)
{
    int displayNum = displayForWidget(widget);
    if (displayNum != wxNOT_FOUND)
        return wxDisplay(displayNum).GetClientArea();

    return FloatRect();
}

float scaleFactor(Widget*)
{
    return 1.0f;

}

}
