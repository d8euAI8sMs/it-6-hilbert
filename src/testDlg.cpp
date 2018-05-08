
// testDlg.cpp : implementation file
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"
#include "afxdialogex.h"
#include "plot.h"

#include <iostream>
#include <array>
#include <numeric>
#include <atomic>
#include <thread>
#include <ctime>

#include "fft.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATEDATA_MESSAGE (WM_USER + 1000)

using namespace common;

namespace
{
    
    simple_list_plot
            signal_plot       = simple_list_plot::curve(RGB(150, 150, 150), 1),
            envelope_plot     = simple_list_plot::connected_points(RGB(180, 0, 0), 3, 2),
            demodulation_plot = simple_list_plot::connected_points(RGB(0, 0, 180), 3, 2),
            error_plot        = simple_list_plot::connected_points(0, 2, 1),
            spectrum_plot     = simple_list_plot::connected_points(RGB(0, 0, 0), 1, 1);

    std::atomic<bool> working;

    inline sinT_t to_sin_params(gaussian_t g)
    {
        return { g.a, g.s, g.t0 };
    }
}

CtestDlg::CtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestDlg::IDD, pParent)
    , sample_count(1024)
    , snr_percents(0)
    , modulation_amplitude(0.5)
    , phase(M_PI / 3)
    , deviation_frequency(0)
    , min_deviation_frequency(0)
    , max_deviation_frequency(0.3)
    , selected_frequency(0.3)
    , data_points(100)
    , repeat_count(100)
    , centered_lfm(false)
    , randomize_frequency(true)
    , sin_modulation_fn(false)
    , draw_err_only(false)
{
    srand(time(NULL));


    input_params[0] = { 1, 100, 100 };
    input_params[1] = { 1, 200, 300 };
    input_params[2] = { 1, 300, 600 };
    input_params[3] = { 0, 100, 0 };
    input_params[4] = { 0, 100, 0 };

    
    signals_plot_ctrl.plot_layer.with(
        (((plot::plot_builder() << demodulation_plot) << envelope_plot) << signal_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
    );
    error_plot_ctrl.plot_layer.with(
        (plot::plot_builder() << error_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 5, 5)
        .with_y_ticks(0, 5, 5)
        .build()
    );
    spectrum_plot_ctrl.plot_layer.with(
        (plot::plot_builder() << spectrum_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 5, 5)
        .with_y_ticks(0, 5, 5)
        .build()
    );
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PLOT_SIGNALS, signals_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_ERROR, error_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_SPECTRUM, spectrum_plot_ctrl);
    DDX_Text(pDX, IDC_SIGNAL_FREQUENCY, frequency);
    DDX_Text(pDX, IDC_SIGNAL_PHASE, phase);
    DDX_Text(pDX, IDC_DEVIATION_FREQUENCY, deviation_frequency);
    DDX_Text(pDX, IDC_MODULATION_AMPLITUDE, modulation_amplitude);
    DDX_Text(pDX, IDC_DEVIATION_FREQUENCY_MIN, min_deviation_frequency);
    DDX_Text(pDX, IDC_DEVIATION_FREQUENCY_MAX, max_deviation_frequency);
    DDX_Text(pDX, IDC_FREQUENCY_SELECTED, selected_frequency);
    DDX_Text(pDX, IDC_REPEAT, repeat_count);
    DDX_Text(pDX, IDC_REPEAT_I, repeat_i);
    DDX_Check(pDX, IDC_CENTERED_LFM, centered_lfm);
    DDX_Check(pDX, IDC_RANDOMIZE_FREQUENCY, randomize_frequency);
    DDX_Check(pDX, IDC_CHECK2, draw_err_only);
    DDX_Check(pDX, IDC_SIN_MOD_FN, sin_modulation_fn);
    DDX_Text(pDX, IDC_DATA_POINTS, data_points);
    DDX_Text(pDX, IDC_SAMPLE_COUNT, sample_count);
    DDX_Text(pDX, IDC_SNR, snr_percents);
    DDX_Text(pDX, IDC_SIGNAL_A1, input_params[0].a);
    DDX_Text(pDX, IDC_SIGNAL_A2, input_params[1].a);
    DDX_Text(pDX, IDC_SIGNAL_A3, input_params[2].a);
    DDX_Text(pDX, IDC_SIGNAL_A44, input_params[3].a);
    DDX_Text(pDX, IDC_SIGNAL_A5, input_params[4].a);
    DDX_Text(pDX, IDC_SIGNAL_S1, input_params[0].s);
    DDX_Text(pDX, IDC_SIGNAL_S2, input_params[1].s);
    DDX_Text(pDX, IDC_SIGNAL_S3, input_params[2].s);
    DDX_Text(pDX, IDC_SIGNAL_S44, input_params[3].s);
    DDX_Text(pDX, IDC_SIGNAL_S5, input_params[4].s);
    DDX_Text(pDX, IDC_SIGNAL_t1, input_params[0].t0);
    DDX_Text(pDX, IDC_SIGNAL_t2, input_params[1].t0);
    DDX_Text(pDX, IDC_SIGNAL_t3, input_params[2].t0);
    DDX_Text(pDX, IDC_SIGNAL_t44, input_params[3].t0);
    DDX_Text(pDX, IDC_SIGNAL_t5, input_params[4].t0);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_START_PAUSE, &CtestDlg::OnBnClickedStartPause)
    ON_BN_CLICKED(IDC_STOP, &CtestDlg::OnBnClickedStop)
    ON_MESSAGE(WM_UPDATEDATA_MESSAGE, &CtestDlg::OnUpdateDataMessage)
    ON_BN_CLICKED(IDOK, &CtestDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BUTTON1, &CtestDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CtestDlg message handlers

BOOL CtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    UpdateData(FALSE);
    UpdateData(TRUE);

    demoDlg.Create(DemoDlg::IDD, this);

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CtestDlg::OnPaint()
{
	CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestDlg::OnBnClickedStartPause()
{
    bool expected = false;
    if (!working.compare_exchange_strong(expected, true))
    {
        return;
    }
    std::thread worker(&CtestDlg::DoWork, this);
    worker.detach();
}


void CtestDlg::OnBnClickedStop()
{
    working = false;
}


void CtestDlg::DoWork()
{
    DoWork0();
}


void CtestDlg::DoWork0()
{
    RequestUpdateData(TRUE);

    srand(time(0) % UINT_MAX);

    sampled_t envelope_sampled = allocate_sampled(sample_count, 1);
    sampled_t input_sampled = allocate_sampled(sample_count, 1);
    sampled_t noise_sampled = allocate_sampled(sample_count, 1);
    sampled_t recovered_sampled = allocate_sampled(sample_count, 1);
    sampled_t error_sampled = allocate_sampled(data_points, (max_deviation_frequency - min_deviation_frequency) / data_points);
    sampled_t spectrum_sampled = allocate_sampled(sample_count / 2, 1. / sample_count * 2 * M_PI);
    sampled_t tmp = allocate_sampled(sample_count, 1);
    cmplx    *fft = new cmplx[sample_count];

    map(error_sampled, [] (size_t, double) { return 0; });

    continuous_t input_signals[5];
    if (!sin_modulation_fn)
    {
        input_signals[0] = gaussian(input_params[0]);
        input_signals[1] = gaussian(input_params[1]);
        input_signals[2] = gaussian(input_params[2]);
        input_signals[3] = gaussian(input_params[3]);
        input_signals[4] = gaussian(input_params[4]);
    }
    else
    {
        input_signals[0] = sin(to_sin_params(input_params[0]));
        input_signals[1] = sin(to_sin_params(input_params[1]));
        input_signals[2] = sin(to_sin_params(input_params[2]));
        input_signals[3] = sin(to_sin_params(input_params[3]));
        input_signals[4] = sin(to_sin_params(input_params[4]));
    }

    continuous_t modulation_fn = combine(5, input_signals);
    continuous_t envelope = [&] (double t) { return 1 + modulation_amplitude * modulation_fn(t); };
    sample(envelope, envelope_sampled);

    continuous_t lfm;

    if (centered_lfm)
    {
        lfm = [&] (double t)
        {
            double period = 1 / frequency / 2;
            int n = floor(t / period);
            t -= n * period;
            double result = 0;
            if ((n % 2) == 0)
            {
                result += (t * t / period / 2 - t / 2);
            }
            else
            {
                result += (t / 2 - t * t / period / 2);
            }
            return result;
        };
    }
    else
    {
        lfm = [&] (double t)
        {
            double period = 1 / frequency / 2;
            int n = floor(t / period);
            t -= n * period;
            double result = n * period / 2;
            if ((n % 2) == 0)
            {
                result += (t * t / period / 2);
            }
            else
            {
                result += (t - t * t / period / 2);
            }
            return result;
        };
    }

    frequency = selected_frequency;

    for (repeat_i = 0; repeat_i < repeat_count; repeat_i++)
    {
        for (size_t data_point = 0; (data_point < data_points) && working; data_point++)
        {
            deviation_frequency = min_deviation_frequency +
                ((max_deviation_frequency - min_deviation_frequency) / data_points) * data_point;

            if (centered_lfm && randomize_frequency)
            {
                frequency = deviation_frequency / 2 + ((double) rand() / RAND_MAX) * (M_PI - deviation_frequency / 2);
            }
            else if (randomize_frequency)
            {
                frequency = ((double) rand() / RAND_MAX) * (M_PI - deviation_frequency);
            }

            continuous_t signal = [&] (double t)
            {
                return envelope(t) * cos(frequency * t + phase + deviation_frequency * lfm(t));
            };
            double signal_power = sample(signal, input_sampled);
            if (snr_percents > 1e-12)
            {
                double noise_power = sample(noise(), noise_sampled);
                double alpha = std::sqrt(snr_percents / 100. * signal_power / noise_power);
                map(input_sampled, noise_sampled, [alpha] (size_t, double a, double b) { return a + b * alpha; });
            }

            for (size_t i = 0; i < sample_count; i++)
            {
                fft[i].real = input_sampled.samples[i];
                fft[i].image = 0;
            }

            fourier(fft, sample_count, -1);

            for (size_t i = 0; i < sample_count / 2; i++)
            {
                spectrum_sampled.samples[i] = log10(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
                fft[i].real *= 2; fft[i + sample_count / 2].real = 0;
                fft[i].image *= 2; fft[i + sample_count / 2].image = 0;
            }

            fft[0].real /= 2; fft[0].real /= 2;

            fourier(fft, sample_count, 1);

            map(recovered_sampled, [&] (size_t i, double)
            {
                return -sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
            });

            if (!draw_err_only)
            {
                setup(spectrum_plot, spectrum_sampled);
                setup(envelope_plot, envelope_sampled);
                setup(demodulation_plot, recovered_sampled);
                setup(signal_plot, input_sampled);
            }

            error = 0;
            map(envelope_sampled, recovered_sampled, [&] (size_t, double e, double r) {
                error += (e - abs(r)) * (e - abs(r));
                return e;
            });
            error /= sample_count;
            error = sqrt(error);

            error_sampled.samples[data_point] += error;
            setup(error_plot, error_sampled,
                  ((repeat_i == 0) ? (data_point + 1) : data_points), 0,
                  [&] (size_t, double e) { return e / (repeat_i + 1.); });

            if (!draw_err_only)
            {
                signals_plot_ctrl.RedrawWindow();
                spectrum_plot_ctrl.RedrawWindow();
            }

            error_plot_ctrl.RedrawWindow();

            RequestUpdateData(FALSE);
        }
    }

    free_sampled(envelope_sampled);
    free_sampled(noise_sampled);
    free_sampled(input_sampled);
    free_sampled(recovered_sampled);
    free_sampled(error_sampled);
    free_sampled(spectrum_sampled);
    free_sampled(tmp);
    delete[] fft;

    working = false;
}


LRESULT CtestDlg::OnUpdateDataMessage(WPARAM wpD, LPARAM lpD)
{
    UpdateData(wpD == TRUE);
    return 0;
}

void CtestDlg::RequestUpdateData(BOOL saveAndValidate)
{
    SendMessage(WM_UPDATEDATA_MESSAGE, saveAndValidate);
}

void CtestDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialog::OnOK();
}


void CtestDlg::OnBnClickedButton1()
{
    demoDlg.ShowWindow(SW_SHOW);
    demoDlg.UpdateWindow();
}


void CtestDlg::OnBnClickedCheck1()
{
    // TODO: Add your control notification handler code here
}
