
// VideoWallpaperDlg.h: 头文件
//

#pragma once
#include "VlcPlayer.h"

// CVideoWallpaperDlg 对话框
class CVideoWallpaperDlg : public CDialogEx
{
// 构造
public:
	CVideoWallpaperDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEOWALLPAPER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	// 重载OnCancel，实现点击X时候隐藏窗口
	afx_msg void OnCancel();
	// 重载DestroyWindow，实现退出的时候移除托盘图标
	afx_msg BOOL DestroyWindow();
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	// 系统托盘消息函数
	afx_msg LRESULT OnSystemTray(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	// 托盘菜单对象
	CMenu	mTrayMenu;
	// 系统托盘数据对象
	NOTIFYICONDATA mNotifyIcon;
	// 原本的背景图像
	CImage mOldBackgroud;
	// VLC播放器
	VlcPlayer mVlcPlayer;
	// 媒体文件路径
	CString mMediaPath;
	// 播放按钮控制对象
	CButton mPlayButton;
	// 停止按钮控制对象
	CButton mStopButton;
	// 音量调节滑块
	CSliderCtrl mVolumeSliderCtrl;
	// 进度控制滑块
	CSliderCtrl mPlayposSliderCtrl;
	// 定时器定时时长(如果小于0则表示未开启)
	int64_t mTimerElapse;
	// 当前播放媒体文件的总时长
	int64_t mMediaTotalTime;
	// 音频轨道控制
	CComboBox mAudioTrackComBox;
	// 播放时间显示控制
	CStatic mPlayTimeLText;
public:
	afx_msg void OnMenuTrayExit();
	afx_msg void OnMenuTrayPause();
	afx_msg void OnMenuTrayMute();
	afx_msg void OnBnClickedButtonSelectFile();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnMenuTrayStop();
	afx_msg void OnPlayRateControl();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCbnSelchangeCboxAudioChannel();
};
