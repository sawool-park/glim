
// CircleDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Circle.h"
#include "CircleDlg.h"
#include "afxdialogex.h"
#include <stdexcept>
#include <exception>
#include <algorithm>

using namespace Gdiplus;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std; // 소스 파일에서만 `std` 네임스페이스 사용

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonInitial();
	afx_msg void OnBnClickedButtonRandomMove();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


CCircleDlg::CCircleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CIRCLE_DIALOG, pParent), 
	m_circleRadius(_T("50")), 
	m_BorderThickness(_T("10")), 
	m_nRandomCounter(RANDOM_MOVE_EXPIRE_NUM),
	m_lastTick(0)
{	
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pComposer = nullptr;
	m_drawer = CCircleDrawer();
}

void CCircleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RADIUS, m_circleRadius);
	DDX_Text(pDX, IDC_EDIT_BOARDER_THICKNESS, m_BorderThickness);
}

BEGIN_MESSAGE_MAP(CCircleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT_RADIUS, &CCircleDlg::OnEnChangeEditRadius)
	ON_EN_KILLFOCUS(IDC_EDIT_RADIUS, &CCircleDlg::OnEnKillfocusEditRadius)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_BOARDER_THICKNESS, &CCircleDlg::OnEnChangeEditBorderThickness)
	ON_EN_KILLFOCUS(IDC_EDIT_BOARDER_THICKNESS, &CCircleDlg::OnEnKillfocusEditBorderThickness)
	ON_BN_CLICKED(IDC_BUTTON_INITIAL, &CCircleDlg::OnBnClickedButtonInitial)
	ON_BN_CLICKED(IDC_BUTTON_RANDOM_MOVE, &CCircleDlg::OnBnClickedButtonRandomMove)
	ON_STN_CLICKED(IDC_DRAW_AREA, &CCircleDlg::OnStnClickedDrawArea)
	ON_WM_DRAWITEM()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_USER_UPDATE_UI, &CCircleDlg::OnUpdateUI)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CCircleDlg::OnBnClickedButtonExit)
END_MESSAGE_MAP()

BOOL CCircleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// 윈도우 기본 크기를 설정한다.
	SetWindowPos(NULL, 0, 0, 1280, 720, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	// 윈도우 크기 변경 방지
	// 프레임버퍼 동적 변경에 대한 처리가 되지않아 크기를 고정하였다. 
	LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
	style &= ~WS_SIZEBOX;        // 사이즈 박스 제거
	style &= ~WS_MAXIMIZEBOX;    // 최대화 버튼 제거
	SetWindowLong(m_hWnd, GWL_STYLE, style);

	// 프레임버퍼 생성하고 컴포저 스레드를 시작한다.
	CRect drawArea;
	GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&drawArea);
	ScreenToClient(&drawArea);
	m_drawer.CreateFrameBuffer(drawArea);
	RunComposerWorker();

	return TRUE;
}

void CCircleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CCircleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CCircleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCircleDlg::OnEnChangeEditRadius()
{
	KillTimer(RADIUS_INPUT_TIMER_ID);
	SetTimer(RADIUS_INPUT_TIMER_ID, EDIT_INPUT_DEBOUNCING, nullptr);
}

void CCircleDlg::OnBnClickedButtonInitial()
{
	KillTimer(RANDOM_MOVE_TIMER_ID);
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	PostThreadMessage(m_pComposer->m_nThreadID, WM_COMPOSER_DELETE_SCREEN, (WPARAM)hEvent, 0);
	if (hEvent) {
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);
	}
	m_nRandomCounter = RANDOM_MOVE_EXPIRE_NUM;
}

void CCircleDlg::OnBnClickedButtonRandomMove()
{
	if (m_drawer.IsOuterDrawable()) {
		SetTimer(RANDOM_MOVE_TIMER_ID, RANDOM_MOVE_DURATION, nullptr);
		m_nRandomCounter = RANDOM_MOVE_EXPIRE_NUM;
	}
}

void CCircleDlg::OnEnChangeEditBorderThickness()
{
	KillTimer(TICKNESS_INPUT_TIMER_ID);
	SetTimer(TICKNESS_INPUT_TIMER_ID, EDIT_INPUT_DEBOUNCING, nullptr);
}

void CAboutDlg::OnBnClickedButtonInitial()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CAboutDlg::OnBnClickedButtonRandomMove()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CCircleDlg::OnEnKillfocusEditRadius()
{
	KillTimer(RADIUS_INPUT_TIMER_ID); 
	UpdateData(TRUE);
	int radius = _ttoi(m_circleRadius);
	if (radius < 3) {
		AfxMessageBox(_T("내원의 반지름은 3 보다 커야 합니다."));
	}
	UpdateData(FALSE);
}


void CCircleDlg::OnEnKillfocusEditBorderThickness()
{
	KillTimer(TICKNESS_INPUT_TIMER_ID);
	UpdateData(TRUE);
	// 정원 두께 변경되면, 다시 그리라 요청한다. 
	int thickness = _ttoi(m_BorderThickness);
	if (thickness > 0) {
		RequestRedrawOuterToComposer();
	}
	RequestRedrawOuterToComposer();
	UpdateData(FALSE);
}

void CCircleDlg::OnTimer(UINT_PTR nIDEvent)
{
	// 사용자 입력 디바운싱 
	if (nIDEvent == RADIUS_INPUT_TIMER_ID) {
		KillTimer(RADIUS_INPUT_TIMER_ID);
		UpdateData(TRUE);
		int radius = _ttoi(m_circleRadius);
		if (radius < MINIMUM_RADIUS) {
			AfxMessageBox(_T("내원의 반지름은 3 보다 커야 합니다."));
		}
		UpdateData(FALSE);
	}
	else if (nIDEvent == TICKNESS_INPUT_TIMER_ID) {
		KillTimer(TICKNESS_INPUT_TIMER_ID);
		UpdateData(TRUE);
		int thickness = _ttoi(m_BorderThickness);
		if (thickness > 0) {
			RequestRedrawOuterToComposer();
		} else {
			AfxMessageBox(_T("정원 선 두께 값은 0 보다 커야 합니다."));
		}
		UpdateData(FALSE);
	}
	else if (nIDEvent == RANDOM_MOVE_TIMER_ID) {
		PostThreadMessage(m_pComposer->m_nThreadID, WM_COMPOSER_DRAW_RANDOM, 0, 0);
		m_nRandomCounter--;
		if (m_nRandomCounter <= 0) {
			KillTimer(RANDOM_MOVE_TIMER_ID);
			m_nRandomCounter = RANDOM_MOVE_EXPIRE_NUM;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

BOOL CCircleDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		// 엔터키 무시 (다이얼로그 닫히지 않음)
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CCircleDlg::OnStnClickedDrawArea()
{
}

void CCircleDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	// 더블 버퍼링용 메모리 DC 생성
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

	// 배경 흰색
	memDC.FillSolidRect(&rect, DEFAULT_DRAW_AREA_BACKGROUND_COLOR);
	m_drawer.DrawToDevice(memDC.GetSafeHdc());

	const CircleObject* circle = m_drawer.GetDraggingInnerCircle();
	if (circle) {
		PrintTextCircleCenter(circle, &memDC);
	}

	// 백버퍼 → 실제 화면
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBitmap);
	bitmap.DeleteObject();
	memDC.DeleteDC();
}

void CCircleDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect drawRect;
	GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&drawRect);
	ScreenToClient(&drawRect);

	int radius = _ttoi(m_circleRadius);
	// 점이 3개 미만일 경우에만 점을 추가
	if (radius >= MINIMUM_RADIUS) {
		if (m_drawer.CanAddNewCircle()) {
			CPoint local = point - drawRect.TopLeft();
			if (drawRect.PtInRect(point) && !m_drawer.CheckRegionOverlapped(local)) {
				HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				PostThreadMessage(m_pComposer->m_nThreadID, WM_COMPOSER_DRAW_INNER, (WPARAM)new CPoint(local), (LPARAM)hEvent);
				if (hEvent) {
					WaitForSingleObject(hEvent, INFINITE);
					CloseHandle(hEvent);
				}
			}
		}
	}
	// 3개의 점이 다 채워졌으면 드래그 가능하도록 설정
	// 선택된 원을 찾고, dragging = true 만들어 드래깅
	if (m_drawer.IsDraggable()) {
		CPoint local = point - drawRect.TopLeft();
		if (m_drawer.TrySetDraggable(local)) {
			SetCapture();
		}
	}
	RequestRedrawOuterToComposer();

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CCircleDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON) {
		// 드래깅 중인 원을 찾아 이동
		int index = m_drawer.IsDraggingInnerCircle();
		if (index >= 0) {
			ULONGLONG tick = GetTickCount64();
			// 마지막 드래그 시각을 기준으로 과도한 랜더링 방지
			if (IsOverSpeed()) return;
			CRect area;
			GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&area);
			ScreenToClient(&area);
			CPoint local = point - area.TopLeft();
			m_drawer.ChangeInnerCircleCentroid(index, local);
			int thickness = GetNewCirlceBorderThickness();
			PostThreadMessage(m_pComposer->m_nThreadID, WM_COMPOSER_DRAW_OUTER, thickness, 0);
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CCircleDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_drawer.TryFreeDraggable();
	ReleaseCapture();
	GetDlgItem(IDC_DRAW_AREA)->Invalidate();
	GetDlgItem(IDC_DRAW_AREA)->UpdateWindow();

	CDialogEx::OnLButtonUp(nFlags, point);
}

BOOL CCircleDlg::WaitForThreadMessageQueue(DWORD threadId, DWORD timeout) {
	DWORD start = DWORD(GetTickCount64());
	while (GetTickCount64() - start < timeout) {
		if (PostThreadMessage(threadId, WM_NULL, 0, 0))
			return TRUE;
		Sleep(10);
	}
	return FALSE;
}

UINT CCircleDlg::DrawAreaComposer(LPVOID pParam) {
	DrawAreaComposerParam* pArgs = reinterpret_cast<DrawAreaComposerParam*>(pParam);
	CCircleDlg* pDlg = pArgs->m_pDlg;
	HANDLE hReadyEvent = pArgs->m_hReadyEvent;
	if (hReadyEvent) {
		SetEvent(hReadyEvent);
	}
	delete pParam;

	ULONGLONG lastTick = 0;
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	try {
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message != WM_COMPOSER_RESIZE_SCREEN) {
				MSG pmsg;
				if (PeekMessage(&pmsg, NULL,WM_COMPOSER_RESIZE_SCREEN, WM_COMPOSER_RESIZE_SCREEN, PM_NOREMOVE)) {
					switch (pmsg.message) {
					case WM_COMPOSER_RESIZE_SCREEN:
						if (pmsg.wParam) {
							CSize* size = reinterpret_cast<CSize*>(pmsg.wParam);
							ULONGLONG tick = static_cast<ULONGLONG>(pmsg.lParam);
							if (tick <= lastTick) break;
							if (lastTick < tick) lastTick = tick;
							// do something here ...
							if (size) {
								TRACE(_T("우선순위 메시지 수신함 (크기변경됨, %d, %d)\n"), size->cx, size->cy);
								delete size;
							}
						}
						break;
					default:
						TRACE(_T("우선순위 메시지 수신함 (%d)\n"), pmsg.message);
						break;
					}
				}
			}
			switch (msg.message) {
			case WM_COMPOSER_DRAW_OUTER:
				pDlg->ComposerOuterCircleHandle((int)msg.wParam);
				break;
			case WM_COMPOSER_DRAW_INNER:
				pDlg->ComposerInnerCircleHandle((CPoint*)msg.wParam, (HANDLE)msg.lParam);
				break;
			case WM_COMPOSER_UPDATE_SCREEN:
				pDlg->ComposerAllObjectsHandle();
				break;
			case WM_COMPOSER_CLEAR_FRAME:
				pDlg->ComposerClearFrameHandle((int)msg.wParam);
			case WM_COMPOSER_DELETE_SCREEN:
				// 다른 메시지 처리
				pDlg->ComposerDeleteScreenHandle((HANDLE)msg.wParam);
				break;  // ✅ break 추가
			case WM_COMPOSER_CLEAR_SCREEN:
				pDlg->ComposerClearScreenHandle();
				break;
			case WM_COMPOSER_DRAW_RANDOM:
				pDlg->ComposerRandomMoveHandle();
				break;
			default:
				// 기타 메시지
				// 우선순위 메시지는 PeekMessage() 에서 이미 처리하였기로 버린다
				break;
			}
		}
	}
	catch (const exception& ex) {
		TRACE(_T("스레드 예외 발생: %S\n"), ex.what());
	}
	return 0;
}

void CCircleDlg::ComposerOuterCircleHandle(int thickness) {
	m_drawer.DrawCircumOuterCircle(thickness);
	m_drawer.RenderObjects();
	if (!PostMessage(WM_USER_UPDATE_UI, USER_UPDATE_DRAW, 0)) {
		throw runtime_error("PostMessage failed");
	}
}

void CCircleDlg::ComposerInnerCircleHandle(CPoint* local, HANDLE hEvent)
{
	if (local == nullptr) {
		if (hEvent) SetEvent(hEvent);
		return;
	}
	m_drawer.DrawInnerCircle(*local, GetNewCircleRadius(), CPoint(0, 0));
	delete local;
	CircleObject* circle = m_drawer.GetLastAddedInnerCircle();
	if (circle) {
		circle->SetColor(m_drawer.GetCircleColor());
		m_drawer.RenderObjects();
		if (hEvent) SetEvent(hEvent);
		if (!PostMessage(WM_USER_UPDATE_UI, USER_UPDATE_DRAW, 0)) {
			throw runtime_error("PostMessage failed");
		}
		return;
	}
	if (hEvent) SetEvent(hEvent);
}

void CCircleDlg::ComposerAllObjectsHandle() 
{
	m_drawer.RenderObjects();
	if (!PostMessage(WM_USER_UPDATE_UI, USER_UPDATE_DRAW, 0)) {
		throw runtime_error("PostMessage failed");
	}
}

void CCircleDlg::ComposerClearFrameHandle(int index) 
{
	m_drawer.ClearFrameBuffer(index);
}

void CCircleDlg::ComposerClearScreenHandle()
{
	m_drawer.ClearScreen();
	m_drawer.RenderObjects();
	if (!PostMessage(WM_USER_UPDATE_UI, USER_UPDATE_DRAW, 0)) {
		throw runtime_error("PostMessage failed");
	}
}

void CCircleDlg::ComposerDeleteScreenHandle(HANDLE hEvent)
{
	if (hEvent) {
		SetEvent(hEvent);
	}
	m_drawer.ClearScreen();
	if (!PostMessage(WM_USER_UPDATE_UI, USER_DELETE_DRAW, 0)) {
		throw runtime_error("PostMessage failed");
	}
}

void CCircleDlg::ComposerRandomMoveHandle()
{
	CWnd* pDrawArea = GetDlgItem(IDC_DRAW_AREA);
	if (pDrawArea == nullptr) {
		throw runtime_error("Draw area control not found");
	}
	if (!::IsWindow(pDrawArea->GetSafeHwnd())) {
		throw runtime_error("Draw area window is invalid");
	}
	CRect area;
	pDrawArea->GetClientRect(&area);
	m_drawer.RandomMoveInners(area);
	m_drawer.DrawCircumOuterCircle(m_drawer.m_crcOuter->m_thickness);
	m_drawer.RenderObjects();
	if (!PostMessage(WM_USER_UPDATE_UI, USER_UPDATE_DRAW, 0)) {
		throw runtime_error("PostMessage failed");
	}
}

LRESULT CCircleDlg::OnUpdateUI(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case USER_UPDATE_DRAW:
		GetDlgItem(IDC_DRAW_AREA)->Invalidate();
		GetDlgItem(IDC_DRAW_AREA)->UpdateWindow();
	case USER_RANDOM_DRAW:
		GetDlgItem(IDC_DRAW_AREA)->Invalidate();
		GetDlgItem(IDC_DRAW_AREA)->UpdateWindow();
		break;
	case USER_DELETE_DRAW:		{
			CRect drawRect;
			GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&drawRect);
			ScreenToClient(&drawRect);
			CDC* pDC = GetDC();
			pDC->FillSolidRect(&drawRect, DEFAULT_DRAW_AREA_BACKGROUND_COLOR);
			ReleaseDC(pDC);
		}
		break;
	case USER_STOP_NOTICE:
		break;
	}
	return 0;
}

void CCircleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (GetSafeHwnd()) {
		CWnd* pDrawArea = GetDlgItem(IDC_DRAW_AREA);
		if (pDrawArea && pDrawArea->GetSafeHwnd()) {
			int margin = 20;
			int newWidth = cx - 135;
			int newHeight = cy - 2 * margin;
			TRACE(_T("윈도우 크기 변경됨 (%d, %d)\n"), newWidth, newHeight);
			if (m_pComposer) {
				PostThreadMessage(m_pComposer->m_nThreadID, 
					WM_COMPOSER_RESIZE_SCREEN, 
					(WPARAM)new CSize(newWidth, newHeight), 
					(LPARAM)GetTickCount64());
			}
			pDrawArea->MoveWindow(margin, margin, newWidth, newHeight);
		}
	}
}

void CCircleDlg::OnClose()
{
	CDialogEx::OnClose();
}

void CCircleDlg::OnBnClickedButtonExit()
{
	PostMessage(WM_CLOSE, 0, 0);
}

void CCircleDlg::RunComposerWorker()
{
	HANDLE hComposerReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	DrawAreaComposerParam* param = new DrawAreaComposerParam(this, hComposerReady);
	m_pComposer = AfxBeginThread(DrawAreaComposer, (LPVOID)param);
	if (hComposerReady) {
		WaitForSingleObject(hComposerReady, INFINITE);
		return;
	}
	WaitForThreadMessageQueue(m_pComposer->m_nThreadID, 1000);
	return;
}

void CCircleDlg::RequestRedrawOuterToComposer() 
{
	if (m_drawer.IsOuterDrawable()) {
		try {
			int thickness = GetNewCirlceBorderThickness();
			PostThreadMessage(m_pComposer->m_nThreadID, WM_COMPOSER_DRAW_OUTER, thickness, 0);
		}
		catch (const std::exception&) {
			// "정원 선 두께 값은 0보다 커야 합니다."
			return;
		}
	}
}

void CCircleDlg::PrintTextCircleCenter(const CircleObject* circle, CDC* memDC)
{
	CString draggingText;
	draggingText.Format(_T("center: (%d, %d)"), circle->m_center.x, circle->m_center.y);

	memDC->SetBkMode(TRANSPARENT);
	memDC->SetTextColor(RGB(0, 0, 0));

	// 드로잉 영역 중심 좌표 구하기
	CRect drawRect;
	GetDlgItem(IDC_DRAW_AREA)->GetWindowRect(&drawRect);
	ScreenToClient(&drawRect);
	CPoint drawCenter = drawRect.CenterPoint();

	// 중심 → 원 방향 벡터
	long dx = circle->m_center.x - drawCenter.x;
	long dy = circle->m_center.y - drawCenter.y;

	double dist = sqrt(dx * dx + dy * dy);
	if (dist == 0) dist = 1.0;

	int radius = circle->m_radius;
	int labelDistance = 40; // 텍스트 중심까지 거리

	// 접점 위치 (원 테두리 위)
	double t1 = radius / dist;
	int startX = int(round(circle->m_center.x - dx * t1));
	int startY = int(round(circle->m_center.y - dy * t1));

	// 텍스트 중심 위치
	double t2 = (radius + labelDistance) / dist;
	int textCenterX = int(round(circle->m_center.x - dx * t2));
	int textCenterY = int(round(circle->m_center.y - dy * t2));

	// 텍스트 박스 위치
	CSize textSize = memDC->GetTextExtent(draggingText);
	int textX = textCenterX - textSize.cx / 2;
	int textY = textCenterY - textSize.cy / 2;

	memDC->TextOut(textX, textY, draggingText);

	CPen dottedPen(PS_DOT, 1, RGB(255, 255, 0));
	CPen* pOldPen = memDC->SelectObject(&dottedPen);
	memDC->MoveTo(circle->m_center);
	memDC->LineTo(CPoint(startX, startY));

	CPen solidPen(PS_SOLID, 1, RGB(0, 0, 0));
	memDC->SelectObject(&solidPen);
	memDC->MoveTo(CPoint(startX, startY));
	memDC->LineTo(CPoint(textCenterX, textCenterY));

	memDC->SelectObject(pOldPen);
}