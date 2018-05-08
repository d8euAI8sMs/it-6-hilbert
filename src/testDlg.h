
// testDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "common.h"

#include "PlotStatic.h"
#include "DemoDlg.h"

// CtestDlg dialog
class CtestDlg : public CDialog
{
// Construction
public:
	CtestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
// public:
    DemoDlg demoDlg;
    afx_msg void OnBnClickedStartPause();
    afx_msg void OnBnClickedStop();
    PlotStatic error_plot_ctrl;
    PlotStatic signals_plot_ctrl;
    PlotStatic spectrum_plot_ctrl;
    size_t sample_count;
    size_t data_points;
    double snr_percents;
    BOOL centered_lfm;
    BOOL randomize_frequency;
    BOOL draw_err_only;
    BOOL sin_modulation_fn;
    common::gaussian_t input_params[5];
    double modulation_amplitude;
    double frequency;
    double phase;
    double deviation_frequency;
    double min_deviation_frequency;
    double max_deviation_frequency;
    double selected_frequency;
    size_t repeat_count;
    size_t repeat_i;
    double error;
    void DoWork();
    LRESULT OnUpdateDataMessage(WPARAM wpD, LPARAM lpD);
    void RequestUpdateData(BOOL saveAndValidate);
    void DoWork0();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedButton1();
public:
    afx_msg void OnBnClickedCheck1();
};
