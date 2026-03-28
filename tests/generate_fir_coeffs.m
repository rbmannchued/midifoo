% generate_fir_coeffs.m
%
% Gera coeficientes FIR lowpass para o afinador de guitarra + baixo.
% Substitui o filtro atual que atenua frequências abaixo de ~80 Hz,
% impedindo a detecção das cordas graves do baixo (E1=41Hz, A1=55Hz).
%
% Uso:
%   octave generate_fir_coeffs.m
%
% Saída:
%   - Gráfico da resposta em frequência
%   - Arquivo ../src/dsp/fir_coeffs.c com os novos coeficientes

pkg load signal;

% ===== Parâmetros (devem bater com dsp.h) =====
fs      = 4000;   % SAMPLE_RATE
N       = 64;     % NUM_TAPS
nyquist = fs / 2; % 2000 Hz

% ===== Notas de referência =====
note_names = {'Baixo E1','Baixo A1','Baixo D2','Baixo G2','Guit E2','Guit A2','Guit E4'};
note_freqs = [ 41.2,      55.0,      73.4,      98.0,      82.4,     110.0,    329.6 ];

% ===== Projeto do filtro =====
% Lowpass com corte em 1200 Hz: passa todas as notas de baixo e guitarra
% (incluindo harmônicos relevantes para o HPS até ~3x a nota mais grave)
% e atenua ruído acima de ~1400 Hz.
%
% Ganho DC ~1.0 garante que E1 (41 Hz) e A1 (55 Hz) passem sem atenuação.
% O IIR HPF (alpha=0.97, corte ~20 Hz) já remove o DC antes deste filtro.

f_cutoff = 1200;               % Hz — borda do passband
wn       = f_cutoff / nyquist; % normalizado: 0..1 (1 = Nyquist)

b = fir1(N - 1, wn, 'low', hamming(N));

% ===== Verificação de ganho nas notas críticas =====
fprintf('\n--- Ganho do filtro nas notas críticas ---\n');
fprintf('%-12s  %8s  %10s\n', 'Nota', 'Freq(Hz)', 'Ganho(dB)');
fprintf('%s\n', repmat('-', 1, 36));
for k = 1:length(note_freqs)
    f   = note_freqs(k);
    w   = 2 * pi * f / fs;
    H   = abs(sum(b .* exp(-1j * w * (0:N-1))));
    dB  = 20 * log10(H + 1e-12);
    fprintf('%-12s  %8.1f  %+10.2f dB\n', note_names{k}, f, dB);
end
fprintf('\n');

% ===== Ganho DC =====
dc_gain = sum(b);
fprintf('Ganho DC: %.4f  (ideal = 1.0)\n', dc_gain);
fprintf('N taps: %d  |  fs: %d Hz  |  corte: %d Hz\n\n', N, fs, f_cutoff);

% ===== Gráfico =====
[H_resp, f_axis] = freqz(b, 1, 4096, fs);
H_dB = 20 * log10(abs(H_resp) + 1e-12);

figure('Name', 'Resposta em frequência do novo filtro FIR');
subplot(2,1,1);
plot(f_axis, H_dB, 'b-', 'LineWidth', 1.5); hold on;
for k = 1:length(note_freqs)
    plot([note_freqs(k) note_freqs(k)], [-80 5], 'r--');
    text(note_freqs(k), -75 + mod(k,3)*8, note_names{k}, 'FontSize', 7, 'Rotation', 90);
end
ylim([-80 5]);
xlim([0 nyquist]);
xlabel('Frequência (Hz)');
ylabel('Ganho (dB)');
title(sprintf('Lowpass FIR — %d taps, corte %d Hz, fs %d Hz', N, f_cutoff, fs));
grid on;

subplot(2,1,2);
plot(f_axis, H_dB, 'b-', 'LineWidth', 1.5); hold on;
for k = 1:length(note_freqs)
    plot([note_freqs(k) note_freqs(k)], [-3 1], 'r--');
    text(note_freqs(k), -2.5 + mod(k,2)*0.8, note_names{k}, 'FontSize', 7, 'Rotation', 90);
end
ylim([-3 1]);
xlim([20 200]);
xlabel('Frequência (Hz)  [zoom: graves]');
ylabel('Ganho (dB)');
title('Zoom na região do baixo (20–200 Hz)');
grid on;

% ===== Comparação com o filtro antigo =====
fir_old = [ ...
    -0.00019963, 0.00046691, 0.00035070, -0.00076233, -0.00207077, ...
    -0.00222905, -0.00083510, 0.00048556, -0.00073123, -0.00475865, ...
    -0.00825601, -0.00727923, -0.00245701, -0.00001404, -0.00556880, ...
    -0.01639876, -0.02201229, -0.01516454, -0.00235712, 0.00000327, ...
    -0.01658760, -0.03930061, -0.04323853, -0.01878963, 0.01117643, ...
    0.00836715, -0.03936117, -0.09263793, -0.08430548, 0.02052339, ...
    0.18146407, 0.30212950, 0.30212950, 0.18146407, 0.02052339, ...
    -0.08430548, -0.09263793, -0.03936117, 0.00836715, 0.01117643, ...
    -0.01878963, -0.04323853, -0.03930061, -0.01658760, 0.00000327, ...
    -0.00235712, -0.01516454, -0.02201229, -0.01639876, -0.00556880, ...
    -0.00001404, -0.00245701, -0.00727923, -0.00825601, -0.00475865, ...
    -0.00073123, 0.00048556, -0.00083510, -0.00222905, -0.00207077, ...
    -0.00076233, 0.00035070, 0.00046691, -0.00019963 ...
];

[H_old, ~] = freqz(fir_old, 1, 4096, fs);

figure('Name', 'Comparação: filtro antigo vs novo');
plot(f_axis, 20*log10(abs(H_resp)+1e-12), 'b-', 'LineWidth', 2); hold on;
plot(f_axis, 20*log10(abs(H_old)+1e-12),  'r--', 'LineWidth', 2);
for k = 1:4  % só notas de baixo
    plot([note_freqs(k) note_freqs(k)], [-40 5], ':k');
end
ylim([-40 5]);
xlim([20 300]);
legend('Novo (lowpass 1200 Hz)', 'Antigo', 'Location', 'SouthEast');
xlabel('Frequência (Hz)');
ylabel('Ganho (dB)');
title('Comparação na região do baixo (20–300 Hz)');
grid on;

% ===== Gravar fir_coeffs.c =====
out_file = '../src/dsp/fir_coeffs.c';
fid = fopen(out_file, 'w');
if fid < 0
    error('Não foi possível abrir %s para escrita', out_file);
end

fprintf(fid, '#include "fir_coeffs.h"\n');
fprintf(fid, '/* Lowpass FIR — %d taps, corte %d Hz, fs %d Hz, janela Hamming */\n', N, f_cutoff, fs);
fprintf(fid, '/* Gerado por tests/generate_fir_coeffs.m                        */\n');
fprintf(fid, 'const float32_t fir_coeffs[NUM_TAPS] = {\n');
for i = 1:N
    if i < N
        fprintf(fid, '    %.8f,\n', b(i));
    else
        fprintf(fid, '    %.8f\n', b(i));
    end
end
fprintf(fid, '};\n');
fclose(fid);

fprintf('Coeficientes gravados em: %s\n', out_file);
fprintf('Pressione Enter para fechar os gráficos...\n');
pause;
