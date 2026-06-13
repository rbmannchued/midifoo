pkg load signal;
clear;
clc;

origem = "./samples/44100_sample_rate/";
destino = "./samples/4000_sample_rate/";

if ~exist(destino, "dir")
    mkdir(destino);
end

arquivos = dir(fullfile(origem, "*.wav"));

for k = 1:length(arquivos)

    nome = arquivos(k).name;

    fprintf("Converting %s...\n", nome);

    [x, fs] = audioread(fullfile(origem, nome));

    if columns(x) > 1
        x = mean(x,2);
    end

    y = resample(x, 4000, fs);

    audiowrite(
        fullfile(destino, nome),
        y,
        4000,
        "BitsPerSample", 16
    );

end

fprintf("\nDone.\n");
