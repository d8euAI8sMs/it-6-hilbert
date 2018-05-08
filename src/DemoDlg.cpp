// DemoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DemoDlg.h"
#include "afxdialogex.h"

#include "fft.h"
#include "plot.h"

using namespace common;

// DemoDlg dialog

IMPLEMENT_DYNAMIC(DemoDlg, CDialog)

simple_list_plot
        signal_plot       = simple_list_plot::curve(RGB(150, 150, 150), 1),
        envelope_plot     = simple_list_plot::connected_points(RGB(180, 0, 0), 3, 2),
        demodulation_plot = simple_list_plot::connected_points(RGB(0, 0, 180), 3, 2),
        spectrum_plot     = simple_list_plot::connected_points(RGB(0, 0, 0), 1, 1),
        orthogonal_plot   = simple_list_plot::connected_points(RGB(255, 155, 0), 1, 1);

inline sinT_t to_sin_params(gaussian_t g)
{
    return { g.a, g.s, g.t0 };
}

DemoDlg::DemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(DemoDlg::IDD, pParent)
    , sample_count(1024)
    , snr_percents(0)
    , modulation_amplitude(0.5)
    , frequency(0.3)
    , phase(M_PI / 3)
    , deviation_frequency(0)
    , show_signal(true)
    , show_orthogonal(false)
    , link_deviation_period_and_signal_frequency(true)
    , centered_lfm (false)
    , sin_modulation_fn(false)
{
    input_params[0] = { 1, 100, 100 };
    input_params[1] = { 1, 200, 300 };
    input_params[2] = { 1, 300, 600 };
    input_params[3] = { 0, 100, 0 };
    input_params[4] = { 0, 100, 0 };

    deviation_period = 1. / frequency;
    
    demo_output_plot_ctrl.plot_layer.with(
        ((((plot::plot_builder() << demodulation_plot) << envelope_plot) << orthogonal_plot) << signal_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
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

DemoDlg::~DemoDlg()
{
}

void DemoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PLOT_DEMO_OUTPUT, demo_output_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_DEMO_SPECTRUM, spectrum_plot_ctrl);
    DDX_Control(pDX, IDC_SLIDER_FREQUENCY, frequency_slider_ctrl);
    DDX_Control(pDX, IDC_SLIDER_DEVIATION, frequency_deviation_slider_ctrl);
    DDX_Control(pDX, IDC_SLIDER_PHASE, phase_slider_ctrl);
    DDX_Control(pDX, IDC_SLIDER_DEVIATION_PERIOD, deviation_period_slider_ctrl);
    DDX_Text(pDX, IDC_MODULATION_AMPLITUDE, modulation_amplitude);
    DDX_Text(pDX, IDC_SIGNAL_FREQUENCY, frequency);
    DDX_Text(pDX, IDC_SIGNAL_PHASE, phase);
    DDX_Text(pDX, IDC_DEVIATION_FREQUENCY, deviation_frequency);
    DDX_Text(pDX, IDC_DEVIATION_PERIOD, deviation_period);
    DDX_Check(pDX, IDC_SHOW_SIGNAL, show_signal);
    DDX_Check(pDX, IDC_SHOW_ORTH, show_orthogonal);
    DDX_Check(pDX, IDC_SHOW_LINK_DEVIATION_AND_FREQUENCY, link_deviation_period_and_signal_frequency);
    DDX_Check(pDX, IDC_SHOW_LINEAR_SCALE, linear_scale);
    DDX_Check(pDX, IDC_CENTERED_LFM, centered_lfm);
    DDX_Check(pDX, IDC_SIN_MOD_FN, sin_modulation_fn);
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
    DDX_Text(pDX, IDC_ERROR, error);
    if (!pDX->m_bSaveAndValidate)
    {
        frequency_slider_ctrl.SetPos(frequency / M_PI * SHORT_MAX);
        frequency_deviation_slider_ctrl.SetPos(deviation_frequency / M_PI * SHORT_MAX);
        phase_slider_ctrl.SetPos(phase / M_PI / 2 * SHORT_MAX);
        deviation_period_slider_ctrl.SetPos(deviation_period / 1000. * SHORT_MAX);
    }
}


BEGIN_MESSAGE_MAP(DemoDlg, CDialog)
    ON_BN_CLICKED(IDC_BUTTON1, &DemoDlg::OnBnClickedButton1)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_SHOW_SIGNAL, &DemoDlg::OnBnClickedShowSignal)
    ON_BN_CLICKED(IDC_SHOW_ORTH, &DemoDlg::OnBnClickedShowOrth)
    ON_BN_CLICKED(IDC_SHOW_LINK_DEVIATION_AND_FREQUENCY, &DemoDlg::OnBnClickedShowLinkDeviationAndFrequency)
    ON_BN_CLICKED(IDC_SHOW_LINEAR_SCALE, &DemoDlg::OnBnClickedShowLinearScale)
    ON_BN_CLICKED(IDC_CENTERED_LFM, &DemoDlg::OnBnClickedCenteredLfm)
    ON_BN_CLICKED(IDC_SIN_MOD_FN, &DemoDlg::OnBnClickedSinModFn)
END_MESSAGE_MAP()


// DemoDlg message handlers


BOOL DemoDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    frequency_slider_ctrl.SetRange(0, SHORT_MAX, TRUE);
    frequency_deviation_slider_ctrl.SetRange(0, SHORT_MAX, TRUE);
    phase_slider_ctrl.SetRange(0, SHORT_MAX, TRUE);
    deviation_period_slider_ctrl.SetRange(0, SHORT_MAX, TRUE);
    frequency_slider_ctrl.SetLineSize(10);
    frequency_deviation_slider_ctrl.SetLineSize(10);
    phase_slider_ctrl.SetLineSize(10);
    deviation_period_slider_ctrl.SetLineSize(10);

    UpdateData(FALSE);
    UpdateData(TRUE);

    OnBnClickedButton1();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void DemoDlg::OnBnClickedButton1()
{
    UpdateData(TRUE);

    sampled_t envelope_sampled = allocate_sampled(sample_count, 1);
    sampled_t input_sampled = allocate_sampled(sample_count, 1);
    sampled_t noise_sampled = allocate_sampled(sample_count, 1);
    sampled_t recovered_sampled = allocate_sampled(sample_count, 1);
    sampled_t spectrum_sampled = allocate_sampled(sample_count / 2, 1. / sample_count * 2 * M_PI);
    sampled_t orthogonal_sampled = allocate_sampled(sample_count, 1);
    cmplx    *fft = new cmplx[sample_count];

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
            double period = deviation_period / 2;
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
            double period = deviation_period / 2;
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
    continuous_t signal = [&] (double t)
    {
        return envelope(t) * cos(frequency * t + phase + deviation_frequency * lfm(t));
    };
    double signal_power = sample(signal, input_sampled);
    double noise_power = sample(noise(), noise_sampled);
    double alpha = std::sqrt(snr_percents / 100. * signal_power / noise_power);
    map(input_sampled, noise_sampled, [alpha] (size_t, double a, double b) { return a + b * alpha; });
    
    for (size_t i = 0; i < sample_count; i++)
    {
        fft[i].real = input_sampled.samples[i];
        fft[i].image = 0;
    }

    fourier(fft, sample_count, -1);

    for (size_t i = 0; i < sample_count / 2; i++)
    {
        if (linear_scale)
        {
            spectrum_sampled.samples[i] = fft[i].real * fft[i].real + fft[i].image * fft[i].image;
        }
        else
        {
            spectrum_sampled.samples[i] = log10(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
        }
        fft[i].real *= 2; fft[i + sample_count / 2].real = 0;
        fft[i].image *= 2; fft[i + sample_count / 2].image = 0;
    }
    
    fft[0].real /= 2;fft[0].real /= 2;

    fourier(fft, sample_count, 1);

    map(recovered_sampled, input_sampled, [&] (size_t i, double r, double in)
    {
        orthogonal_sampled.samples[i] = fft[i].image;
        return -sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
    });
    
    orthogonal_plot.visible = show_orthogonal;
    signal_plot.visible = show_signal;
    setup(spectrum_plot, spectrum_sampled);
    setup(orthogonal_plot, orthogonal_sampled);
    setup(envelope_plot, envelope_sampled);
    setup(demodulation_plot, recovered_sampled);
    setup(signal_plot, input_sampled);

    error = 0;
    map(envelope_sampled, recovered_sampled, [&] (size_t, double e, double r) {
        error += (e - abs(r)) * (e - abs(r));
        return e;
    });
    error /= sample_count;
    error = sqrt(error);

    demo_output_plot_ctrl.RedrawWindow();
    spectrum_plot_ctrl.RedrawWindow();

    free_sampled(envelope_sampled);
    free_sampled(noise_sampled);
    free_sampled(input_sampled);
    free_sampled(recovered_sampled);
    free_sampled(spectrum_sampled);
    free_sampled(orthogonal_sampled);
    delete[] fft;

    UpdateData(FALSE);
}


void DemoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    deviation_frequency = ((double) frequency_deviation_slider_ctrl.GetPos()) / SHORT_MAX * M_PI;
    phase = ((double) phase_slider_ctrl.GetPos()) / SHORT_MAX * M_PI * 2;

    if (link_deviation_period_and_signal_frequency)
    {
        if (reinterpret_cast<CSliderCtrl*>(pScrollBar) == &frequency_slider_ctrl)
        {
            frequency = ((double) frequency_slider_ctrl.GetPos()) / SHORT_MAX * M_PI;
            deviation_period = 1. / frequency;
        }
        else if (reinterpret_cast<CSliderCtrl*>(pScrollBar) == &deviation_period_slider_ctrl)
        {
            deviation_period = ((double) deviation_period_slider_ctrl.GetPos()) / SHORT_MAX * 1000;
            frequency = 1. / deviation_period;
        }
    }
    else
    {
        frequency = ((double) frequency_slider_ctrl.GetPos()) / SHORT_MAX * M_PI;
        deviation_period = ((double) deviation_period_slider_ctrl.GetPos()) / SHORT_MAX * 1000;
    }

    UpdateData(FALSE);

    OnBnClickedButton1();

    //CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void DemoDlg::OnBnClickedShowSignal()
{
    OnBnClickedButton1();
}


void DemoDlg::OnBnClickedShowOrth()
{
    OnBnClickedButton1();
}


void DemoDlg::OnBnClickedShowLinkDeviationAndFrequency()
{
    OnBnClickedButton1();
}


void DemoDlg::OnBnClickedShowLinearScale()
{
    OnBnClickedButton1();
}


void DemoDlg::OnBnClickedCenteredLfm()
{
    OnBnClickedButton1();
}


void DemoDlg::OnBnClickedSinModFn()
{
    OnBnClickedButton1();
}
