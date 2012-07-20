#ifndef BufferedPaint_h
#define BufferedPaint_h

#include <wx/dc.h>
#include <windows.h>

typedef HANDLE HPAINTBUFFER;

class BufferedPaint
{
public:
    static bool hasSystemBuffering();

    BufferedPaint(wxWindow* window, unsigned char alpha = 255);
    ~BufferedPaint();

    wxDC& GetDC();

protected:
    wxWindow* m_window;
    wxDC* m_bufferDC;
    wxDC* m_paintDC;
    HPAINTBUFFER m_hBufferedPaint;
    unsigned char m_alpha;
    
};

#endif
