#pragma once

#include <windows.h>
#include <atlimage.h>
#include <vector>
#include <afxwin.h>
#include <atomic>
#include <memory>
#include <algorithm>


const UINT NUMBER_OF_DRAGGABLE_CIRCLES = 3;
const int  NUMBER_OF_FRAME_BUFFER = 2;
const BYTE ALPHA_OUTER = 100;
const BYTE WHITE = 255;
const bool IsCircumOuterOnTop = true;

const COLORREF BACKGROUND_COLOR = RGB(255, 255, 255);
const float alpha_norm = 1 / 255.0f;
const float alpha_denorm = 255.0f;

class CircleObject {
public:
	CPoint m_center;
	CPoint m_offset;
	COLORREF m_color;
	int m_radius;
	int m_thickness;
	bool m_dragging;
	CImage* m_image;

	CircleObject(CPoint center = CPoint(0, 0), int radius = 0, CPoint offset = CPoint(0, 0), int thickness = 1)
		: m_center(center), 
		m_radius(radius), 
		m_offset(offset), 
		m_dragging(false), 
		m_color(RGB(0, 0, 0)),
		m_thickness(thickness), 
		m_image(nullptr) {
	}
	~CircleObject() {
		if (m_image) delete m_image;
	}

	CRect GetSizeWithRadius() {
		return CRect(0, 0, m_radius * 2, m_radius * 2);
	}

	CRect GetSizeWithThickness() {
		int outer = m_radius + (m_thickness + 1) / 2;
		return CRect(m_center.x - outer,
			m_center.y - outer,
			m_center.x + outer,
			m_center.y + outer);
	}

	static CRect GetRectWithThickness(CPoint center, int radius, int thickness) {
		int outer = radius + (thickness + 1) / 2;
		return CRect(center.x - outer,
			center.y - outer,
			center.x + outer,
			center.y + outer);
	}

	CPoint GetLeftTop() {
		return CPoint(m_center.x - m_radius, m_center.y - m_radius);
	}

	CRect GetBoundingRect() {
		return CRect(m_center.x - m_radius,
			m_center.y - m_radius,
			m_center.x + m_radius,
			m_center.y + m_radius);
	}

	CRect GetBoundingRectWithThickness() {
		int outer = m_radius + (m_thickness + 1) / 2;
		return CRect(m_center.x - outer,
			m_center.y - outer,
			m_center.x + outer,
			m_center.y + outer);
	}

	void SetColor(COLORREF color) {
		m_color = color;
	}

	bool Identical(const CircleObject& source) {
		if (m_center == source.m_center && 
			m_radius == source.m_radius && 
			m_thickness == source.m_thickness && 
			m_offset == source.m_offset) {
			if (m_image && m_image != source.m_image) {
				return false;
			}
			return true;
		} 
		return false;
	}
};

class CCircleDrawer {
public:
	std::vector<CircleObject*> m_crcInners;
	CircleObject* m_crcOuter;
	std::vector<CImage*> m_frames;
	int m_index;
	std::vector<CRect> m_crcFrames;

public:
	CCircleDrawer();
	~CCircleDrawer();
	void FillCircleImage(CImage* image, CircleObject* object, COLORREF color);
	void DrawCircleWithThickness(CImage* image, CircleObject* object, COLORREF color);
	bool CheckOverlappedCircle(CPoint center, int radius);
	void DrawInnerCircle(CPoint center, int radius, CPoint offset, int thickness = 0);
	bool DrawCircumOuterCircle(int thickness);
	void ComposeFrameBuffer(CImage* frame);
	void CreateFrameBuffer(CRect frame);
	CImage* GetFrontBuffer();
	CImage* GetBackBuffer();
	CircleObject* GetOuterCircle();
	void ChangeFrameBuffer();
	std::vector<CRect> GetCircleRectList();
	CRect GetBoundingRect(const std::vector<CRect>& rects);
	void ClearFrontBuffer(CRect rect);
	void DrawToDevice(HDC hdc);
	bool TrySetDraggable(CPoint point);
	void ClearFrameBuffer(int index);
	bool TryFreeDraggable();

	void ClearInnerCircle() {
		while (!m_crcInners.empty()) {
			CircleObject* circle = m_crcInners.back();
			delete circle;
			m_crcInners.pop_back();
		}
	}

	void DeleteOuter()
	{
		if (m_crcOuter) delete m_crcOuter;
		m_crcOuter = nullptr;
	}

	bool CanAddNewCircle() {
		return m_crcInners.size() < NUMBER_OF_DRAGGABLE_CIRCLES ? true : false;
	}

	bool IsOuterDrawable() {
		return m_crcInners.size() == NUMBER_OF_DRAGGABLE_CIRCLES ? true : false;
	}

	int IsDraggingInnerCircle() {
		int index = 0;
		for (auto& circle : m_crcInners) {
			if (circle->m_dragging) {
				return index;
			}
			index++;
		}
		return -1;
	}

	const CircleObject* GetDraggingInnerCircle() {
		int index = 0;
		for (auto& circle : m_crcInners) {
			if (circle->m_dragging) {
				return circle;
			}
		}
		return nullptr;
	}

	const CircleObject* GetInnerCircle(int index) {
		if (index < m_crcInners.size()) {
			return m_crcInners[index];
		}
		return nullptr;
	}

	bool IsDraggable() {
		return IsOuterDrawable();
	}

	bool CheckRegionOverlapped(CPoint point) {
		bool exists = std::any_of(m_crcInners.begin(), m_crcInners.end(), [&](const CircleObject* c) {
			return c->m_center == point;
		});
		return exists;
	}

	CircleObject* GetLastAddedInnerCircle() {
		return m_crcInners.size() > 0 ? m_crcInners.back() : nullptr;
	}

	COLORREF GetCircleColor() {
		switch (m_crcInners.size()) {
		case 0:
			return RGB(255, 0, 0);
		case 1:
			return RGB(0, 255, 0);
		case 2:
			return RGB(0, 0, 255);
		default:
			return RGB(0, 0, 0);
		}
	}

	void RenderObjects() {
		ComposeFrameBuffer(GetBackBuffer());
		ChangeFrameBuffer();
	}

	void StoreRenderingRegionForFrame(int index, CRect bounding) {
		if (bounding.left < 0 || bounding.top < 0) {
			return;
		}
		m_crcFrames[index] = bounding;
	}

	void ClearRenderingRegionForFrames(int index) {
		m_crcFrames[index] = CRect();
	}

	CRect GetRenderingRegionForFrame(int index) {
		return m_crcFrames[index];
	}

	int GetFrameBufferIndex(CImage* buffer) {
		int index = 0;
		for (auto image : m_frames) {
			if (image == buffer) {
				return index;
			}
		}
		TRACE(_T("프레임 버퍼 인덱스 손실 발생\n"));
		return -1;
	}

	CImage* CCircleDrawer::GetFrameBuffer(int index) {
		if (index < 0 || index >= 2) return nullptr;
		return m_frames[index];
	}

	void ClearScreen() {
		int index = 0;
		for (auto image : m_frames) {
			ClearFrameBuffer(index);
			index++;
		}
		m_index = 0;
		for (auto& circle : m_crcInners) {
			if (circle) delete circle;
		}
		DeleteOuter();
		m_crcInners.clear();
	}

	void DeleteScreen() {
		int index = 0;
		for (auto image : m_frames) {
			ClearFrameBuffer(index);
			index++;
		}
		m_index = 0;
		for (auto& circle : m_crcInners) {
			if (circle) delete circle;
		}
		if (m_crcOuter) delete m_crcOuter;
		m_crcInners.clear();
		m_crcFrames.clear();
		m_frames.clear();
	}

	void RandomMoveInners(CRect area) {
		for (auto& circle : m_crcInners) {
			int x = max(1, rand() % (area.Width() - 1));
			int y = max(1, rand() % (area.Height() - 1));
			circle->m_center = CPoint(x, y);
		}
	}

	void ChangeInnerCircleCentroid(int index, CPoint center) {
		if (index < m_crcInners.size()) {
			m_crcInners[index]->m_center = center;
		}
	}

	void ClearFrameImage(CImage* image, COLORREF color);
	
	/* alpha blending
		- a : 0 .. 255 일 경우 한 해, af = af * alpha_norm, ab = ab * alpha_norm, ... 처리 후, 마지막에 ab = ao * alpha_denorm
		ao = af + ab * (1 - af);
		co = (cf * af + cb * ab * (1 - af)) / ao;
		cb = co;
		ab = ao;
	*/
	inline void AlphaBlending(BYTE* src, BYTE* bac, BYTE* dst) {
		float af = src[3] * alpha_norm;
		float ab = bac[3] * alpha_norm;
		float ao = af + ab * (1 - af);
		if (ao != 0) {
			int co = 0;
			for (int c = 0; c < 3; c++) {
				co = (int)((src[c] * af + bac[c] * ab * (1 - af)) / ao);
				if (co < 0) co = 0;
				if (co > 255) co = 255;
				dst[c] = (BYTE)co;
			}
			dst[3] = (BYTE)(ao * alpha_denorm);
		}
	}
};