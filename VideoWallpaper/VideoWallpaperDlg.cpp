
// VideoWallpaperDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "VideoWallpaper.h"
#include "VideoWallpaperDlg.h"
#include "UTF8CStringConv.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 定义系统托盘消息
#define WM_SYSTEM_TRAY (WM_USER + 1)

// 背景窗体句柄
static HWND s_hProgmanWnd = NULL;
static HWND s_hWorkerWnd = NULL;
static HWND s_hVLCMainWnd = NULL;

BOOL CALLBACK EnumWindowProcFindDesktopWindow(HWND hTop, LPARAM lparam)
{
	// 查找 SHELLDLL_DefView 窗体句柄
	// 存在多个WorkerW窗体，只有底下存在SHELLDLL_DefView的才是
	HWND hWndShl = ::FindWindowEx(
		hTop, NULL, _T("SHELLDLL_DefView"), NULL);
	if (hWndShl == NULL) { return true; }

	// XP 直接查找SysListView32窗体
	// g_hWorker = ::FindWindowEx(hWndShl, NULL, _T("SysListView32"),_T("FolderView"));

	// win7或更高版本
	// 查找 WorkerW 窗口句柄(以桌面窗口为父窗口)
	s_hWorkerWnd = ::FindWindowEx(NULL, hTop, _T("WorkerW"), NULL);
	return s_hWorkerWnd == false;
}

BOOL CALLBACK EnumChildProcFindVLC(HWND hwndChild, LPARAM lParam)
{
	TCHAR classname[256];
	int len = ::GetClassName(hwndChild, classname, 255);
	classname[14] = 0;
	if (StrCmp(classname, _T("VLC video main")) != 0) {
		return true;
	}
	//::SetParent(hwndChild, NULL);
	//::SendMessage(hwndChild, WM_CLOSE, NULL, NULL);
	s_hVLCMainWnd = hwndChild;
	return false;
}


BEGIN_MESSAGE_MAP(CVideoWallpaperDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	/* 添加托盘消息映射 */
	ON_MESSAGE(WM_SYSTEM_TRAY, &CVideoWallpaperDlg::OnSystemTray)
	ON_COMMAND(ID_MENU_TRAY_EXIT, &CVideoWallpaperDlg::OnMenuTrayExit)
	ON_COMMAND(ID_MENU_TRAY_PAUSE, &CVideoWallpaperDlg::OnMenuTrayPause)
	ON_COMMAND(ID_MENU_TRAY_MUTE, &CVideoWallpaperDlg::OnMenuTrayMute)
	ON_COMMAND(ID_MENU_TRAY_STOP, &CVideoWallpaperDlg::OnMenuTrayStop)
	/* 按钮单击事件 */
	ON_BN_CLICKED(IDC_BUTTON_SELECT_FILE, &CVideoWallpaperDlg::OnBnClickedButtonSelectFile)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CVideoWallpaperDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CVideoWallpaperDlg::OnMenuTrayStop)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CVideoWallpaperDlg::OnMenuTrayExit)
	/* 音频轨道选择 */
	ON_CBN_SELCHANGE(IDC_CBOX_AUDIO_TRACK, &CVideoWallpaperDlg::OnCbnSelchangeCboxAudioChannel)
	/* 横向滑块滑动消息 */
	ON_WM_HSCROLL()
	/* 定时器触发消息 */
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CVideoWallpaperDlg 对话框

CVideoWallpaperDlg::CVideoWallpaperDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIDEOWALLPAPER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoWallpaperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PATH, mMediaPath);
	DDX_Control(pDX, IDC_BUTTON_PLAY, mPlayButton);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, mVolumeSliderCtrl);
	DDX_Control(pDX, IDC_SLIDER_PLAY_POS, mPlayposSliderCtrl);
	DDX_Control(pDX, IDC_BUTTON_STOP, mStopButton);
	DDX_Control(pDX, IDC_CBOX_AUDIO_TRACK, mAudioTrackComBox);
	DDX_Control(pDX, IDC_STATIC_PLAY_TIME, mPlayTimeLText);
}

// CVideoWallpaperDlg 消息处理程序

void CVideoWallpaperDlg::OnCancel()
{
	// 隐藏窗口
	this->ShowWindow(HIDE_WINDOW);
	// CDialogEx::OnCancel(); // 默认动作
}

BOOL CVideoWallpaperDlg::DestroyWindow()
{
	// 移除托盘图标
	Shell_NotifyIcon(NIM_DELETE, &mNotifyIcon);
	return CDialogEx::DestroyWindow();
}

BOOL CVideoWallpaperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//设置窗口透明度
	ModifyStyleEx(0, WS_EX_LAYERED);
	SetLayeredWindowAttributes(RGB(0, 0, 0), 150, LWA_ALPHA);

	// TODO: 在此添加额外的初始化代码
	// 给播放按钮设置图标
	HICON hIcon = LoadIcon(
		AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_ICON_PLAY));
	mPlayButton.SetIcon(hIcon);
	hIcon = LoadIcon(
		AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_ICON_STOP));
	mStopButton.SetIcon(hIcon);
	// 设置音量和进度滑块范围
	mVolumeSliderCtrl.SetRange(0, 100, TRUE);
	mVolumeSliderCtrl.SetPos(60);
	mPlayposSliderCtrl.SetRange(0, 1000, TRUE);
	mPlayposSliderCtrl.SetPos(0);
	mTimerElapse = -1;
	// 加载系统托盘菜单
	if (!mTrayMenu.LoadMenu(IDR_MENU)) {
		return FALSE; // 加载失败
	}
	CMenu* pSubMenu = mTrayMenu.GetSubMenu(0);
	if (pSubMenu) {
		pSubMenu->CheckMenuItem(ID_MENU_TRAY_MUTE, MF_BYCOMMAND | MF_UNCHECKED);
	}
	// 设置系统托盘
	mNotifyIcon.cbSize = sizeof(NOTIFYICONDATA);
	mNotifyIcon.hIcon = m_hIcon;
	mNotifyIcon.hWnd = m_hWnd;
	lstrcpy(mNotifyIcon.szTip, _T("视频壁纸"));
	mNotifyIcon.uCallbackMessage = WM_SYSTEM_TRAY; // 设置回掉消息
	mNotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	Shell_NotifyIcon(NIM_ADD, &mNotifyIcon); // 添加到系统托盘

	// 遍历桌面所有顶层窗口去查找桌面背景窗口句柄
	if (s_hProgmanWnd == NULL) {
		// https://www.codeproject.com/articles/856020/draw-behind-desktop-icons-in-windows
		// 先找到Progman 窗口
		s_hProgmanWnd = ::FindWindowEx(NULL, NULL, _T("Progman"), NULL);
		if (s_hProgmanWnd == NULL) {
			MessageBox(_T("找不到Progman窗体"), _T("错误"), MB_OK);
			return FALSE;
		}
		// 发送消息到Program Manager窗口
		// 要在桌面图标和壁纸之间触发创建WorkerW窗口，必须向Program Manager发送一个消息
		// 这个消息没有一个公开的WindowsAPI来执行，只能使用SendMessageTimeout来发送0x052C
		// win10 1903 下未能成功（无法分离）
		DWORD_PTR lpdwResult = 0;
		// ::SendMessage(s_hProgmanWnd, 0x052C, NULL, NULL);
		SendMessageTimeout(s_hProgmanWnd, 0x052C, NULL, NULL, SMTO_NORMAL, 1000, &lpdwResult);

		// 查找到 WorkerW 窗口，设置显示
		EnumWindows(EnumWindowProcFindDesktopWindow, NULL);
		::ShowWindowAsync(s_hWorkerWnd, 0);
	}
	if (s_hProgmanWnd == NULL) {
		MessageBox(_T("找不到桌面壁纸层窗体"), _T("错误"), MB_OK);
		// 发送命令消息-退出按钮按下
		PostMessage(WM_COMMAND,MAKEWPARAM(IDC_BUTTON_EXIT,BN_CLICKED),NULL);
		return FALSE;
	}

	// 第一次播放前保存背景图像
	//if (mOldBackgroud.IsNull()) {
	//	HDC hDC = ::GetWindowDC(s_hProgmanWnd);
	//	int nBitPerPixel = GetDeviceCaps(hDC, BITSPIXEL);
	//	int nWidth = GetDeviceCaps(hDC, HORZRES);
	//	int nHeight = GetDeviceCaps(hDC, VERTRES);

	//	mOldBackgroud.Create(nWidth, nHeight, nBitPerPixel);
	//	BitBlt(mOldBackgroud.GetDC(), 0, 0, nWidth, nHeight, hDC, 0, 0, SRCCOPY);
	//	mOldBackgroud.ReleaseDC();
	//}

	// 初始化VLC播放器
	if (!mVlcPlayer.init()) {
		CString text(_T("VLC初始化失败\n"));
		text.Append(UTF8CStringConv::ConvertUTF8ToCString(
			mVlcPlayer.getLastError()));
		MessageBox(text, _T("错误"), MB_OK);
		//CWnd* exit = GetDlgItem(ID_MENU_TRAY_EXIT);
		//exit->PostMessage(WM_COMMAND);
		return FALSE;
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CVideoWallpaperDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CVideoWallpaperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CVideoWallpaperDlg::OnSystemTray(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_RBUTTONDOWN: /*鼠标右键按下*/
	{
		CMenu menu;
		if (!menu.LoadMenu(IDR_MENU)) { // 加载菜单
			// 加载失败
			break;
		}
		CPoint pos;
		GetCursorPos(&pos);
		// SetForegroundWindow(); // 设置当前窗口为前台窗口
		// 显示托盘菜单
		if (mTrayMenu.GetSubMenu(0) != NULL) {
			// 必须确保菜单资源加载成功，且第一个子菜单是有子菜单的才行(确保其是POPUP的)
			// 这里在IDR_MENU资源中已经添加了"托盘菜单"这一个弹出菜单项
			mTrayMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this);
		}
		// 用完后销毁菜单资源  
		// DestroyMenu(menu);
	}
	break;
	case WM_LBUTTONDBLCLK: /*鼠标左键双击*/
	{
		// 显示窗口
		AfxGetApp()->m_pMainWnd->ShowWindow(SW_SHOW);
	}
	break;
	default:
		break;
	}
	return LRESULT(0);
}


void CVideoWallpaperDlg::OnMenuTrayExit()
{
	// 停止播放并恢复原有桌面背景
	OnMenuTrayStop();
	// 按下退出窗口
	DestroyWindow();
}

void CVideoWallpaperDlg::OnMenuTrayPause()
{
if (mVlcPlayer.isPlaying()) {
	// 暂停播放
	mVlcPlayer.pausePlay();
	// 设置菜单项文本为 继续
	CMenu* pSubMenu = mTrayMenu.GetSubMenu(0);
	if (pSubMenu) {
		pSubMenu->ModifyMenu(ID_MENU_TRAY_PAUSE,
			MF_BYCOMMAND, ID_MENU_TRAY_PAUSE, _T("继续"));
	}
}
else {
	// 继续播放
	mVlcPlayer.continuePlay();
	// 设置菜单项的文本为 暂停
	CMenu* pSubMenu = mTrayMenu.GetSubMenu(0);
	if (pSubMenu) {
		pSubMenu->ModifyMenu(ID_MENU_TRAY_PAUSE,
			MF_BYCOMMAND, ID_MENU_TRAY_PAUSE, _T("暂停"));
	}
}
// 遍历桌面所有顶层窗口去查找桌面背景窗口句柄
//if( hWorkerW == NULL) EnumWindows(EnumWindowCallback, NULL);
//if (hWorkerW == NULL) return;


/*
//CImage image;
//image.Load(_T("D:\\Pictures\\3-15121Q52031.jpg"));
HDC hDC = ::GetWindowDC(hWorkerW);

// 保存背景图像
if (mOldBackgroud.IsNull()) {
	int nBitPerPixel = GetDeviceCaps(hDC, BITSPIXEL);
	int nWidth = GetDeviceCaps(hDC, HORZRES);
	int nHeight = GetDeviceCaps(hDC, VERTRES);

	mOldBackgroud.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(mOldBackgroud.GetDC(), 0, 0, nWidth, nHeight, hDC, 0, 0, SRCCOPY);
	mOldBackgroud.ReleaseDC();
}
::SetStretchBltMode(hDC, COLORONCOLOR);
if (image.IsNull()) { return; }
CRect rect;
::GetWindowRect(hWorkerW,&rect);
image.Draw(hDC, rect);
*/
//mVlcPlayer.startPlay("F:\\20171028_211358.ts", hWorkerW);
}


void CVideoWallpaperDlg::OnMenuTrayMute()
{
	static bool bMute = false;
	// CheckMenuItem
	UINT check = MF_BYCOMMAND | MF_UNCHECKED;
	bMute = !bMute;
	if (bMute) { check = MF_BYCOMMAND | MF_CHECKED; }
	// 获取当前的设置状态
	CMenu* pSubMenu = mTrayMenu.GetSubMenu(0);
	if (pSubMenu) {
		pSubMenu->CheckMenuItem(ID_MENU_TRAY_MUTE, check);
	}
	mVlcPlayer.setMute(bMute);
}


void CVideoWallpaperDlg::OnBnClickedButtonSelectFile()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY,
		_T("视频 (*.asf,*.avi,*.divx,*.dv,*.flv,"
			"*.gxf,*.m1v,*.m2v,*.m2ts,*.m4v,*.mkv,*.mov,"
			"*.mp2,*.mp4,*.mpeg,*.mpeg1,*.mpeg2,*.mpeg4,"
			"*.mpg,*.mts,*.mxf,*.ogg,*.ogm,*.ps,*.ts,*.vob,"
			"*.wmv,*.a52,*.aac,*.ac3,*.dts,*.flac,*.m4a,"
			"*.m4p,*.mka,*.mod,*.mp1,*.mp2,*.mp3,*.ogg)|"
			"(*.asf;*.avi;*.divx;*.dv;*.flv;*.gxf;*.m1v;"
			"*.m2v;*.m2ts;*.m4v;*.mkv;*.mov;*.mp2;*.mp4;"
			"*.mpeg;*.mpeg1;*.mpeg2;*.mpeg4;*.mpg;*.mts;"
			"*.mxf;*.ogg;*.ogm;*.ps;*.ts;*.vob;*.wmv;*.a52;"
			"*.aac;*.ac3;*.dts;*.flac;*.m4a;*.m4p;*.mka;"
			"*.mod;*.mp1;*.mp2;*.mp3;*.ogg)|"
			"所有 (*.*)|*.*||"), this);
	if (dlg.DoModal()) {
		mMediaPath = dlg.GetPathName();
		UpdateData(FALSE);
		if (!mMediaPath.IsEmpty()) {
			OnBnClickedButtonPlay();
		}
	}
}


void CVideoWallpaperDlg::OnBnClickedButtonPlay()
{
	// 将路径转换为UTF8格式
	std::string path =
		UTF8CStringConv::ConvertCStringToUTF8(mMediaPath);
	if (path.empty()) { return; }

	// 播放视频
	if (!mVlcPlayer.startPlay(path, s_hWorkerWnd/*s_hProgmanWnd*/)) {
		::MessageBox(this->GetSafeHwnd(),
			UTF8CStringConv::ConvertUTF8ToCString(mVlcPlayer.getLastError()),
			_T("播放出错"), MB_OK);
		return;
	}

	// 查找VCL播放主窗口
	// ::EnumChildWindows(s_hProgmanWnd,EnumChildProcFindVLC, NULL);
	// // 设置子窗口
	// ::SetWindowLong(s_hProgmanWnd, WS_CLIPCHILDREN, 0);
	// // 置于最底层
	// ::SetWindowPos(s_hVLCMainWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);

	mPlayposSliderCtrl.SetPos(0);
	/* https://blog.csdn.net/leixiaohua1020/article/details/42363079
	在libVLC中可以通过libvlc_media_player_get_length()，libvlc_video_get_width()，
	libvlc_video_get_height()等函数获取到视频的时长，宽，高等信息。但是有一个很奇
	怪的现象：如果在调用完libvlc_media_player_play()之后立即调用上述3个函数的话，
	返回的值都是0，只有“等待”一段时间（例如调用sleep()）后再调用上述函数，才能得
	到正确的数值。对于这种现象，我觉得可能是libVLC加载完成之后，才能通过上述几个函
	数获得正确的值（自己推测的，还没有深究）。
	应该使用libvlc_media_parse解析一次媒体文件也行
	*/
	// 获取视频总时长
	mMediaTotalTime = -1;
	for(int waittime =0;
		waittime < 5000 && mMediaTotalTime <= 0;
		waittime += 100) {
		mMediaTotalTime = mVlcPlayer.getMediaTimeLength();
		Sleep(100);
	}
	mTimerElapse = mMediaTotalTime / 1000;
	if (mTimerElapse < 1000) { mTimerElapse = 1000;}  // 最低为1000 
	// 开启定时器，用于更新播放进度条
	KillTimer(0x1);
	SetTimer(0x1, 1000, NULL);

	// 获取音频通道
	std::map<int, std::string> audiotrtacks = mVlcPlayer.getMediaAudioTracks();
	mAudioTrackComBox.Clear();
	for (auto& track: audiotrtacks) {
		CString s;
		//s.Format(_T("%s"), audiochannels[i].c_str());
		s = UTF8CStringConv::ConvertUTF8ToCString(track.second);
		mAudioTrackComBox.InsertString(track.first,s);
	}
	if (!audiotrtacks.empty()) {
		int cur = mVlcPlayer.getMediaCurrentAudioTrack();
		mAudioTrackComBox.SetCurSel(cur);
	}
}


void CVideoWallpaperDlg::OnMenuTrayStop()
{
	// 停止视频播放
	mVlcPlayer.stopPlay();

	// 停止定时器
	KillTimer(0x1);
	// 应该是会找到的
	if (s_hProgmanWnd == NULL) return;

	// 绘制原来的背景
	//if (!mOldBackgroud.IsNull()) {
	//	HDC hDC = ::GetWindowDC(s_hProgmanWnd);
	//	::SetStretchBltMode(hDC, COLORONCOLOR);

	//	CRect rect;
	//	::GetWindowRect(s_hProgmanWnd, &rect);
	//	mOldBackgroud.Draw(hDC, rect);
	//}
	// 关闭VLC播放窗口
	if (s_hVLCMainWnd != NULL) {
		::SetParent(s_hVLCMainWnd, NULL);
		::SendMessage(s_hVLCMainWnd, WM_CLOSE, NULL, NULL);
	}

}

void CVideoWallpaperDlg::OnPlayRateControl()
{
}


void CVideoWallpaperDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CSliderCtrl* pSlider = (CSliderCtrl*)pScrollBar;
	switch (pScrollBar->GetDlgCtrlID())
	{
	case IDC_SLIDER_VOLUME: /* 音量控制 */
		mVlcPlayer.setVolume(pSlider->GetPos());
		break;
	case IDC_SLIDER_PLAY_POS: /* 进度控制 */
		float curPos;
		curPos = (float)pSlider->GetPos();
		curPos *= 0.001;
		mVlcPlayer.setPlayPos(curPos);
		break;
	default:
		break;
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CVideoWallpaperDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	int64_t pos = 0;
	pos = mVlcPlayer.getMediaPlayTime();
	mPlayposSliderCtrl.SetPos(pos/(mMediaTotalTime/1000));
	// TRACE(_T("pos = %d\n"), pos);
	if (pos > mMediaTotalTime - 5000) {
		mVlcPlayer.setPlayPos(0.0f);
	}

	// 更新时间
	int poshh = pos / 3600000;
	int posmm = (pos % 3600000) / 60000;
	int posss = (pos % 60000) / 1000;
	int thh = mMediaTotalTime / 3600000;
	int tmm = (mMediaTotalTime % 3600000) / 60000;
	int tss = (mMediaTotalTime % 60000) / 1000;
	CString timetext;
	timetext.Format(_T("%02d:%02d:%02d/%02d:%02d:%02d"), poshh, posmm, posss, thh, tmm, tss);
	mPlayTimeLText.SetWindowText(timetext);

	CDialogEx::OnTimer(nIDEvent);
}

void CVideoWallpaperDlg::OnCbnSelchangeCboxAudioChannel()
{
	int cur = mAudioTrackComBox.GetCurSel();
	if (cur != mVlcPlayer.getMediaCurrentAudioTrack()) {
		mVlcPlayer.setMediaAudioTrack(cur);
	}
}
