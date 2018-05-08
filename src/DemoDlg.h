#pragma once

#include "afxwin.h"
#include "resource.h"
#include "PlotStatic.h"
#include "common.h"

// DemoDlg dialog

class DemoDlg : public CDialog
{
	DECLARE_DYNAMIC(DemoDlg)

public:
	DemoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~DemoDlg();

// Dialog Data
	enum { IDD = IDD_DEMODLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    PlotStatic demo_output_plot_ctrl;
    PlotStatic spectrum_plot_ctrl;
    CSliderCtrl frequency_slider_ctrl;
    CSliderCtrl phase_slider_ctrl;
    CSliderCtrl frequency_deviation_slider_ctrl;
    CSliderCtrl deviation_period_slider_ctrl;
    BOOL show_signal;
    BOOL show_orthogonal;
    BOOL link_deviation_period_and_signal_frequency;
    BOOL linear_scale;
    BOOL centered_lfm;
    BOOL sin_modulation_fn;
    size_t sample_count;
    double snr_percents;
    common::gaussian_t input_params[5];
    double modulation_amplitude;
    double frequency;
    double phase;
    double deviation_frequency;
    double deviation_period;
    double error;
	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedShowSignal();
    afx_msg void OnBnClickedShowOrth();
    afx_msg void OnBnClickedShowLinkDeviationAndFrequency();
    afx_msg void OnBnClickedShowLinearScale();
    afx_msg void OnBnClickedCenteredLfm();
    afx_msg void OnBnClickedSinModFn();
};
