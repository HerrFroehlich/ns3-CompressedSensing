%%
%
% Evaluates the output for a 3 simulation with tree topolgy. 
% The snr was calculated during the simulation (flag: --snr).
% The sink is expected to receive the same NOF packets (<=> attempts) in each sequence.
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';  % Name of used algorithm
nRxSink = 96;      % NOF reconstruction attempts per sequence
P0       = 8*85;    % NOF transmission without NC
NBINS    = 100;

%% INIT
nClusters = 3;      % NOF clusters used
%load('data.mat')
snrSpat = zeros(nMeasSeq, nRxSink, nClusters);
nTx =  zeros(1,nMeasSeq);

%% Iterate over clusters
for c = 1:nClusters
    
    clName = ['Cluster' num2str(c-1)];
    %% Iterate over all sequence
    for meas = 0:nMeasSeq-1
        %% spatial snr
        
        stField = eval([clName '.RecSeq'  num2str(meas)]);
        snrNow = stField;
        snrSpat(meas+1, :, c) = snrNow;
    end
       
    nRxSrc = eval([clName '.nPktRxSrc']);
    nRxCl = eval([clName '.nPktRxCl']);
    nTx = nTx + nRxSrc +  nRxCl(1:nMeasSeq);  %overall NOF tx wo link to sink
end

%% plot
%reordering to mean SNR with same number of transmissions
snrSpatMeanCl = squeeze(mean(snrSpat,3)); %mean over clusters
min_nTx = min(nTx);
max_nTx = max(nTx);
snrSpat_nRx = nan(nMeasSeq, (nRxSink + max_nTx-min_nTx));
for meas = 1:nMeasSeq
    idx = nTx(meas) - min_nTx;
    snrSpat_nRx(meas, (idx+(1:nRxSink))) = snrSpatMeanCl(meas, :);
end
snrSpatMean = nanmean(snrSpat_nRx);

%considering connection to sinl
min_nTx = min_nTx +1; % one transmission C2-> sink
max_nTx = max_nTx + nRxSink; % all transmissions C2->sink
eps = (min_nTx:max_nTx)/P0; 

% calculate distribution
nTx_dist = histc(nTx + nRxSink,min_nTx:max_nTx)/nMeasSeq; %also considering fixed tx to sink

figure;
yyaxis left; plot(eps, snrSpatMean); ylabel('mean SNR in dB');
yyaxis right;plot(eps, nTx_dist); ylabel('% of SEQ');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('\epsilon_P'); 