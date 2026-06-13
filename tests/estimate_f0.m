function f0 = estimate_f0(arquivo)
    pkg load signal
    [x, fs] = audioread(arquivo);

    if columns(x) > 1
        x = mean(x, 2);
    end

    x = x - mean(x);

    N = length(x);
    ini = floor(0.25 * N);
    fim = floor(0.75 * N);
    x = x(ini:fim);

    % Window
    x = x .* hann(length(x));

    % Big FFT
    Nfft = 131072;

    X = abs(fft(x, Nfft));
    X = X(1:Nfft/2);

    freq = (0:Nfft/2-1) * fs / Nfft;

    % Ignore DC
    X(1:10) = 0;

    % order peak by amplitude
    [~, idx_sorted] = sort(X, "descend");

    fprintf("\n%s\n", arquivo);
    fprintf("Biggest peaks found:\n");

    for i = 1:10
        idx = idx_sorted(i);
        fprintf("%2d: %8.3f Hz\n", i, freq(idx));
    end

    % 
    idx = idx_sorted(1);

    % Parabolic interpol
    alpha = X(idx-1);
    beta  = X(idx);
    gamma = X(idx+1);

    p = 0.5 * (alpha - gamma) / (alpha - 2*beta + gamma);

    idx_ref = idx + p;

    f0 = (idx_ref - 1) * fs / Nfft;

end
