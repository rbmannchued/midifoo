% simulate_dsp.m
pkg load signal   % necessário para firfilt em Octave

clear; clc; close all;

% ===== verificar argumentos =====
if nargin < 1
    fprintf("Uso: octave simulate_dsp.m <arquivo_wav>\n");
    fprintf("Exemplo: octave simulate_dsp.m samples/A_110_strato_neck.wav\n");
    fprintf("Exemplo: octave simulate_dsp.m samples/D_146_strato_neck.wav\n");
    return;
end

filename = argv(){1};

% ===== parâmetros do "dsp.h" =====
FRAME_LEN   = 2048;
SAMPLE_RATE = 4000;
ADC_MAX_VAL = 4095;
NUM_TAPS    = 64;

% ===== carregar o áudio (já em 4000 Hz) =====
[x, fs] = audioread(filename);
if fs ~= SAMPLE_RATE
  error("Sample rate do arquivo (%d Hz) diferente de %d Hz esperado", fs, SAMPLE_RATE);
end

% garantir canal mono
if size(x,2) > 1
  x = mean(x,2);
end

% normalizar para simular ADC 12 bits
adc_buffer = round((x - min(x)) / (max(x)-min(x)) * ADC_MAX_VAL);
adc_buffer = adc_buffer(1:FRAME_LEN);

% ===== função: preprocess_signal =====
function y = preprocess_signal(buffer, FRAME_LEN, ADC_MAX_VAL)
  mean_val = mean(double(buffer));
  y = zeros(FRAME_LEN,1);
  for i=1:FRAME_LEN
    window = 0.5 * (1 - cos(2*pi*(i-1)/(FRAME_LEN-1)));
    sample = double(buffer(i)) / ADC_MAX_VAL;
    y(i) = (sample - (mean_val / ADC_MAX_VAL)) * window;
  end
end

% ===== FIR coeffs (copiados do seu fir_coeffs.c) =====
fir_coeffs = [
    -0.00019963,
    0.00046691,
    0.00035070,
    -0.00076233,
    -0.00207077,
    -0.00222905,
    -0.00083510,
    0.00048556,
    -0.00073123,
    -0.00475865,
    -0.00825601,
    -0.00727923,
    -0.00245701,
    -0.00001404,
    -0.00556880,
    -0.01639876,
    -0.02201229,
    -0.01516454,
    -0.00235712,
    0.00000327,
    -0.01658760,
    -0.03930061,
    -0.04323853,
    -0.01878963,
    0.01117643,
    0.00836715,
    -0.03936117,
    -0.09263793,
    -0.08430548,
    0.02052339,
    0.18146407,
    0.30212950,
    0.30212950,
    0.18146407,
    0.02052339,
    -0.08430548,
    -0.09263793,
    -0.03936117,
    0.00836715,
    0.01117643,
    -0.01878963,
    -0.04323853,
    -0.03930061,
    -0.01658760,
    0.00000327,
    -0.00235712,
    -0.01516454,
    -0.02201229,
    -0.01639876,
    -0.00556880,
    -0.00001404,
    -0.00245701,
    -0.00727923,
    -0.00825601,
    -0.00475865,
    -0.00073123,
    0.00048556,
    -0.00083510,
    -0.00222905,
    -0.00207077,
    -0.00076233,
    0.00035070,
    0.00046691,
    -0.00019963
];

% ===== aplicar pipeline =====
preprocessed = preprocess_signal(adc_buffer, FRAME_LEN, ADC_MAX_VAL);

filtered = filter(fir_coeffs, 1, preprocessed);

NFFT = FRAME_LEN;
fft_out = fft(filtered, NFFT);
mag = abs(fft_out(1:NFFT/2));

% ===== função: HPS adaptativo =====
function hps = apply_hps_adaptive(mag, sample_rate, frame_len)
  hps_len = length(mag);
  hps = mag;
  bin_hz = sample_rate / frame_len;
  for i=1:hps_len
    freq = (i-1) * bin_hz;
    max_harm = 1;
    if freq < 200
      max_harm = 3;
    elseif freq < 600
      max_harm = 2;
    end
    for dec=2:max_harm
      idx = i*dec;
      if idx <= hps_len
        hps(i) *= mag(idx);
      end
    end
  end
end

hps_mag = apply_hps_adaptive(mag, SAMPLE_RATE, FRAME_LEN);

% ===== detectar pico =====
[peak_val, peak_idx] = max(hps_mag(6:end));
peak_idx = peak_idx + 5; % compensar offset
freq_detected = (peak_idx-1) * SAMPLE_RATE / FRAME_LEN;

fprintf("Arquivo analisado: %s\n", filename);
fprintf("Frequência detectada: %.2f Hz\n", freq_detected);

% ===== plots =====
t = (0:FRAME_LEN-1)/SAMPLE_RATE;

figure;
subplot(3,1,1); plot(t, double(adc_buffer)/ADC_MAX_VAL); title(["Sinal original - " filename]); xlabel("Tempo (s)");
subplot(3,1,2); plot(t, preprocessed); title("Após pré-processamento"); xlabel("Tempo (s)");
subplot(3,1,3); plot(t, filtered); title("Após FIR"); xlabel("Tempo (s)");

f = linspace(0, SAMPLE_RATE/2, FRAME_LEN/2);

figure;
subplot(2,1,1); plot(f, mag); title("Magnitude da FFT"); xlabel("Hz");
subplot(2,1,2); plot(f, hps_mag); hold on;
plot(freq_detected, peak_val, "ro", "MarkerSize", 8, "LineWidth", 2);
title(["HPS - Frequência detectada: " num2str(freq_detected) " Hz"]); xlabel("Hz");

% ===== manter janelas abertas =====
fprintf("\nPressione Enter para fechar os gráficos...\n");
pause;
