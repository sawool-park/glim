#include "pch.h"
#include "CCircleDrawer.h"

using namespace std;


CCircleDrawer::CCircleDrawer() : m_index(0), m_crcOuter(nullptr) {
    m_crcInners.clear();
    m_crcFrames.clear();
}

CCircleDrawer::~CCircleDrawer() {
    for (auto& circle : m_crcInners) {
        if (circle) delete circle;
    }
    for (auto& frame : m_crcFrames) {
        if (frame) delete frame;
    }
    for (auto& image : m_frames) {
        if (image) delete image;
    }
    if (m_crcOuter) delete m_crcOuter;
    m_crcInners.clear();
    m_crcFrames.clear();
    m_frames.clear();
}

void CCircleDrawer::CreateFrameBuffer(CRect frame) {
    if (frame.Width() > 0 && frame.Height() > 0) {
        for (int i = 0; i < NUMBER_OF_FRAME_BUFFER; i++) {
            CImage* image = new CImage();
            image->Create(frame.Width(), frame.Height(), 32);
            ClearFrameImage(image, BACKGROUND_COLOR);
            m_frames.push_back(image);
            m_crcFrames.push_back(CRect());
        }
    }
}

void CCircleDrawer::ClearFrameImage(CImage* image, COLORREF color)
{
    int width = image->GetWidth();
    int height = image->GetHeight();
    int pitch = image->GetPitch();
    BYTE* pBits = (BYTE*)image->GetBits();

    for (int y = 0; y < height; ++y) {
        BYTE* pLine = pBits + y * pitch;
        for (int x = 0; x < width; ++x) {
            BYTE* pixel = pLine + x * 4;
            pixel[0] = GetBValue(color);
            pixel[1] = GetGValue(color);
            pixel[2] = GetRValue(color);
            pixel[3] = 0;  // A (투명)
        }
    }
}

void CCircleDrawer::FillCircleImage(CImage* image, CircleObject* object, COLORREF color) 
{
    int width = image->GetWidth();
    int height = image->GetHeight();
    int pitch = image->GetPitch();
    BYTE* frame = (BYTE*)image->GetBits();

    int inner = object->m_radius * object->m_radius;

    BYTE src[4];
    src[0] = GetBValue(color);
    src[1] = GetGValue(color);
    src[2] = GetRValue(color);
    src[3] = 255;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int dx = x - object->m_radius;
            int dy = y - object->m_radius;
            int distance = dx * dx + dy * dy;
            BYTE* line = frame + y * pitch;
            BYTE* dst = line + x * 4;
            if (distance <= inner) {
                AlphaBlending(src, dst, dst);
            }
        }
    }
}

void CCircleDrawer::DrawCircleWithThickness(CImage* image, CircleObject* object, COLORREF color)
{
    if (image == nullptr) return;
    int width = image->GetWidth();
    if (width == 0) return;
    int height = image->GetHeight();
    int pitch = image->GetPitch();
    BYTE* frame = (BYTE*)image->GetBits();

    int inner = (object->m_radius - object->m_thickness / 2) * (object->m_radius - object->m_thickness / 2);
    int outer = (object->m_radius + object->m_thickness  / 2) * (object->m_radius + object->m_thickness / 2);
     
    BYTE src[4];
    src[0] = GetBValue(color);
    src[1] = GetGValue(color);
    src[2] = GetRValue(color);
    src[3] = ALPHA_OUTER;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int dx = x - object->m_center.x;
            int dy = y - object->m_center.y;
            int distance = dx * dx + dy * dy;
            BYTE* line = frame + y * pitch;
            BYTE* dst = line + x * 4;
            if (distance >= inner && distance <= outer) {
                AlphaBlending(src, dst, dst);
            }
        }
    }
}

bool CCircleDrawer::CheckOverlappedCircle(CPoint center, int radius) 
{
    for (auto& circle : m_crcInners) {
        CRect rect = circle->GetBoundingRect();
        if (rect.PtInRect(center)) {
            return true;
        }
    }
    return false;
}

void CCircleDrawer::DrawInnerCircle(CPoint center, int radius, CPoint offset, int thickness)
{
    if (m_crcInners.size() >= 3) return;
    if (CheckOverlappedCircle(center, radius)) return;
    
    CircleObject* circle = new CircleObject(center, radius, offset, thickness);
    CRect rect = circle->GetBoundingRect();
    circle->m_image = new CImage();
    circle->m_image->Create(rect.Width(), rect.Height(), 32);
    COLORREF color;
    switch (m_crcInners.size()) {
    case 0:
        color = RGB(255, 0, 0);
        break;
    case 1:
        color = RGB(0, 255, 0);
        break;
    case 2:
        color = RGB(0, 0, 255);
        break;
    default:
        color = RGB(0, 0, 0);
    }
    FillCircleImage(circle->m_image, circle, color);
    m_crcInners.push_back(circle);
}

bool CCircleDrawer::DrawCircumOuterCircle(int thickness) {
    if (m_crcInners.size() != NUMBER_OF_DRAGGABLE_CIRCLES) return false;

    CPoint A = m_crcInners[0]->m_center;
    CPoint B = m_crcInners[1]->m_center;
    CPoint C = m_crcInners[2]->m_center;

    double D = 2.0 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
    if (D == 0) return false; // 일직선

    double Ux = ((A.x * A.x + A.y * A.y) * (B.y - C.y) +
        (B.x * B.x + B.y * B.y) * (C.y - A.y) +
        (C.x * C.x + C.y * C.y) * (A.y - B.y)) / D;

    double Uy = ((A.x * A.x + A.y * A.y) * (C.x - B.x) +
        (B.x * B.x + B.y * B.y) * (A.x - C.x) +
        (C.x * C.x + C.y * C.y) * (B.x - A.x)) / D;

    CPoint center = CPoint(static_cast<int>(Ux), static_cast<int>(Uy));
    int radius = static_cast<int>(sqrt((A.x - Ux) * (A.x - Ux) + (A.y - Uy) * (A.y - Uy)));
   
    CircleObject circle = CircleObject(center, radius, 0, thickness);
    circle.SetColor(RGB(128, 128, 128));
    if (m_crcOuter == nullptr) {
        m_crcOuter = new CircleObject();
    }
    *m_crcOuter = circle;
    CImage* back = GetBackBuffer();
    if (back) {
        DrawCircleWithThickness(GetBackBuffer(), m_crcOuter, RGB(128, 128, 128));
    }
    return true;
}

CImage* CCircleDrawer::GetFrontBuffer() 
{
    return m_frames[m_index];
}

CImage* CCircleDrawer::GetBackBuffer() {
    int index = (m_index + 1) % 2;
    return m_frames[index];
}

CircleObject* CCircleDrawer::GetOuterCircle() {
    if (m_crcOuter) return m_crcOuter;
    m_crcOuter = new CircleObject();
    return m_crcOuter;
}

void CCircleDrawer::ComposeFrameBuffer(CImage* back)
{
    ClearFrameImage(back, BACKGROUND_COLOR);

    if (!IsCircumOuterOnTop) {
        if (m_crcOuter) {
            DrawCircleWithThickness(back, m_crcOuter, m_crcOuter->m_color);
        }
    }

    int width = back->GetWidth();
    int height = back->GetHeight();
    int pitch = back->GetPitch();
    BYTE* frame = (BYTE*)back->GetBits();

    for (const auto& circle : m_crcInners) {
        CImage* object = circle->m_image;
        BYTE* region = (BYTE*)object->GetBits();
        int w = object->GetWidth();
        int h = object->GetHeight();
        int dx = circle->m_center.x - w / 2;
        int dy = circle->m_center.y - h / 2;
        int regionPitch = object->GetPitch();
        for (int y = 0; y < h; ++y) {
            if (dy + y < 0 || dy + y >= height) continue;
            BYTE* srcLine = region + y * regionPitch;
            BYTE* dstLine = frame + (dy + y) * pitch;
            for (int x = 0; x < w; ++x) {
                if (dx + x < 0 || dx + x >= width) continue;
                BYTE* src = srcLine + x * 4;
                BYTE* dst = dstLine + (dx + x) * 4;
                AlphaBlending(src, dst, dst);
            }
        }
     }

    if (IsCircumOuterOnTop) {
        if (m_crcOuter) {
            DrawCircleWithThickness(back, m_crcOuter, m_crcOuter->m_color);
        }
    }
    int index = GetFrameBufferIndex(back);
    if (index >= 0) {
        StoreRenderingRegionForFrame(index, GetBoundingRect(GetCircleRectList()));
    }
}

void CCircleDrawer::ChangeFrameBuffer() {
    m_index = (m_index + 1) % 2;
}

std::vector<CRect> CCircleDrawer::GetCircleRectList() {
    std::vector<CRect> list;

    for (const auto circle : m_crcInners) {
        CRect rect = CircleObject::GetRectWithThickness(circle->m_center, circle->m_radius, circle->m_thickness);
        list.push_back(rect);
    }
    if (m_crcOuter) {
        CRect rect = CircleObject::GetRectWithThickness(m_crcOuter->m_center, m_crcOuter->m_radius, m_crcOuter->m_thickness);
        list.push_back(rect);
    }
    return list;
}

CRect CCircleDrawer::GetBoundingRect(const std::vector<CRect>& rects)
{
    if (rects.empty()) return CRect(0, 0, 0, 0);

    int left = rects[0].left;
    int top = rects[0].top;
    int right = rects[0].right;
    int bottom = rects[0].bottom;

    for (size_t i = 1; i < rects.size(); ++i) {
        left = min(left, rects[i].left);
        top = min(top, rects[i].top);
        right = max(right, rects[i].right);
        bottom = max(bottom, rects[i].bottom);
    }

    return CRect(left, top, right, bottom);
}

void CCircleDrawer::ClearFrontBuffer(CRect rect)
{
    if (rect.Width() > 0 && rect.Height() > 0) {
        CImage* front = GetFrontBuffer();
        int width = front->GetWidth();
        int height = front->GetHeight();
        BYTE* pBits = (BYTE*)front->GetBits();
        int pitch = front->GetPitch();

        for (int y = rect.top; y < rect.bottom; ++y) {
            BYTE* pLine = pBits + y * pitch;
            for (int x = rect.left; x < rect.right; ++x) {
                BYTE* pixel = pLine + x * 4;
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 0;
            }
        }
    }
}

void CCircleDrawer::ClearFrameBuffer(int index)
{
    CImage* pFrame = GetFrameBuffer(index);
    CRect rect = GetRenderingRegionForFrame(index);

    if (pFrame == nullptr || rect.Width() <= 0 || rect.Height() <= 0) return;

    int width = pFrame->GetWidth();
    int height = pFrame->GetHeight();
    BYTE* pBits = (BYTE*)pFrame->GetBits();
    int pitch = pFrame->GetPitch();

    for (int y = 0; y < height; ++y) {
        BYTE* pLine = pBits + y * pitch;
        for (int x = 0; x < height; ++x) {
            BYTE* pixel = pLine + x * 4;
            pixel[0] = 255;
            pixel[1] = 255;
            pixel[2] = 255;
            pixel[3] = 0;
        }
    }
    ClearRenderingRegionForFrames(index);
}

void CCircleDrawer::DrawToDevice(HDC hdc)
{
    CImage* front = GetFrontBuffer();
    front->Draw(hdc, 0, 0);
}

bool CCircleDrawer::TrySetDraggable(CPoint point) {
    for (auto& circle : m_crcInners) {
        int dx = point.x - circle->m_center.x;
        int dy = point.y - circle->m_center.y;
        // 원 내부에서 클릭했는지 확인
        if (dx * dx + dy * dy <= circle->m_radius * circle->m_radius) {
            circle->m_dragging = true;
            circle->m_offset = point - circle->m_center;
            return true;
        }
    }
    return false;
}

bool CCircleDrawer::TryFreeDraggable() {
    for (auto& circle : m_crcInners) {
        if (circle->m_dragging) {
            circle->m_dragging = false;
            return true;
        }
    }
    return false;
}
