
// CircleDlg.h: 헤더 파일
//

#pragma once

#include <vector>
#include <afxwin.h>
#include <afxstr.h>
#include <atomic>
#include <memory>
#include <stdexcept>
#include "CCircleDrawer.h"

#define WM_USER_UPDATE_UI			(WM_USER + 1)
#define USER_RANDOM_DRAW			1
#define USER_STOP_NOTICE			2
#define USER_UPDATE_DRAW			3
#define USER_DELETE_DRAW			4
#define USER_RANDOM_DRAW_DURATION	500

#define WM_COMPOSER_UPDATE_SCREEN	(WM_USER + 100)
#define WM_COMPOSER_DELETE_SCREEN	(WM_USER + 101)
#define WM_COMPOSER_CLEAR_SCREEN	(WM_USER + 102)
#define WM_COMPOSER_DRAW_INNER		(WM_USER + 103)
#define WM_COMPOSER_DRAW_OUTER		(WM_USER + 104)
#define WM_COMPOSER_DRAW_RANDOM		(WM_USER + 105)
#define WM_COMPOSER_CLEAR_FRAME		(WM_USER + 106)
#define WM_COMPOSER_RESIZE_SCREEN	(WM_USER + 107)

const CString	DEFAULT_CIRCLE_RADIUS = _T("50");
const CString	DEFAULT_CIRCLE_BORDER_THICKNESS = _T("4");
const COLORREF	DEFAULT_DRAW_AREA_BACKGROUND_COLOR = RGB(255, 255, 255);
const float		DEFAULT_DRAW_FRAME_RATE = (1000.0f / 500);

class CCircleDlg : public CDialogEx
{
public:
	CCircleDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CIRCLE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL PreTranslateMessage(MSG* pMsg);		//

	static const UINT_PTR RADIUS_INPUT_TIMER_ID = 1;	// 반지름 입력 타이머 ID
	static const UINT_PTR TICKNESS_INPUT_TIMER_ID = 2;	// 테두리 선 두께 입력 타이머 ID
	static const UINT EDIT_INPUT_DEBOUNCING = 300;		// 300ms
	static const UINT_PTR RANDOM_MOVE_TIMER_ID = 3;		// 랜덤 이동 타이머 ID
	static const UINT RANDOM_MOVE_DURATION = 500;		// 500ms
	static const int  RANDOM_MOVE_EXPIRE_NUM = 2 * 10;	// 2회/초, 10번
	static const int  MINIMUM_RADIUS = 3;

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditRadius();
	afx_msg void OnBnClickedButtonInitial();
	afx_msg void OnBnClickedButtonRandomMove();
	afx_msg void OnEnChangeEditBorderThickness();
	afx_msg void OnEnKillfocusEditRadius();
	afx_msg void OnEnKillfocusEditBorderThickness();
	afx_msg void OnStnClickedDrawArea();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButtonExit();

	// 스레드에서 원을 그릴 때 사용
	afx_msg LRESULT OnUpdateUI(WPARAM wParam, LPARAM lParam);
	static UINT AFX_CDECL DrawAreaComposer(LPVOID pParam);

	int GetNewCircleRadius() {
		if (m_circleRadius.IsEmpty()) {
			throw std::runtime_error("유효한 반지름 값을 입력해주세요.");
		}
		return _ttoi(m_circleRadius);
	}

	int GetNewCirlceBorderThickness() {
		if (m_BorderThickness.IsEmpty()) {
			throw std::runtime_error("유효한 정원 두께를 입력해주세요.");
		}
		return _ttoi(m_BorderThickness);
	}

	BOOL WaitForThreadMessageQueue(DWORD threadId, DWORD timeout = 5000);
	void RunComposerWorker();
	void ComposerOuterCircleHandle(int thickness);
	void ComposerInnerCircleHandle(CPoint* local, HANDLE hEvent);
	void RequestRedrawOuterToComposer();
	void ComposerAllObjectsHandle();
	void ComposerClearFrameHandle(int index);
	void ComposerClearScreenHandle();
	void ComposerDeleteScreenHandle(HANDLE hEvent);
	void ComposerRandomMoveHandle();

	void PrintTextCircleCenter(const CircleObject* circle, CDC* memDC);
	// 드래그 시각을 기준으로 과도한 랜더링 방지
	bool IsOverSpeed() {
		ULONGLONG tick = GetTickCount64();
		if (m_lastTick != 0 && tick < ULONGLONG(m_lastTick + DEFAULT_DRAW_FRAME_RATE)) return true;
		m_lastTick = tick;
		return false;
	}

	CString m_BorderThickness;
	CString m_circleRadius;
	CWinThread* m_pComposer;
	CCircleDrawer m_drawer;
	int m_nRandomCounter;
	ULONGLONG m_lastTick = 0;
};

class DrawAreaComposerParam {
public:
	DrawAreaComposerParam(CCircleDlg* pDlg, HANDLE hEvent) : m_pDlg(pDlg), m_hReadyEvent(hEvent) {}
	CCircleDlg* m_pDlg;
	HANDLE m_hReadyEvent;
};