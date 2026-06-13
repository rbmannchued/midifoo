clear;
clc;

pasta = "./samples/44100_sample_rate/";

arquivos = dir(fullfile(pasta, "*.wav"));

fprintf("\n%-35s %-12s\n", "File", "F0 (Hz)");
fprintf("--------------------------------------------------\n");

for k = 1:length(arquivos)

    nome = arquivos(k).name;
    caminho = fullfile(pasta, nome);

    try
        f0 = estimate_f0(caminho);

        fprintf("%-35s %10.3f\n", nome, f0);

    catch err
        fprintf("%-35s ERROR: %s\n", nome, err.message);
    end

end
