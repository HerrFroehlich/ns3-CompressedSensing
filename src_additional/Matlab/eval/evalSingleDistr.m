%%
%
% Evaluates the output for a single cluster showing the distribution of the
% SNR when only precoding is used (--onlyprecode).
% The snr was calculated during the simulation (flag: --snr).
% The sink is expected to receive different NOF packets (<=> attempts) in each sequence.
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';  % Name of used algorithm
edges = -5:2:155;
%% INI
%load('data.mat')

%% get  NOF tx 
nRxSink = Cluster0.nPktRxSrc + 1; %+1 since cluster head is also source
nRxSinkMax = max(nRxSink);

nTx = Cluster0.nPktRxSrc;
min_nTx = min(nTx);
max_nTx = max(nTx);
%% Get SNR ordered by NOF transmissions

snrSpatFin = zeros(nMeasSeq,1);
snrSpatMax = zeros(nMeasSeq,1);
for meas = 1:nMeasSeq
    %% spatial snr
    snrSpat = eval(['Cluster0.RecSeq'  num2str(meas-1)]);
    snrSpatFin(meas) = snrSpat(end);
    snrSpatMax(meas) = max(snrSpat);
end
%% plot
figure;
histogram(snrSpatFin, edges); xlabel('mean SNR in dB');
%histogram(snrSpatMax, edges); xlabel('mean SNR in dB');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('\epsilon_P'); 