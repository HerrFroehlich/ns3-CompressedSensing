%%
%
% Evaluates the output for a 3 simulation with tree topolgy and link errors. 
% The snr was calculated during the simulation (flag: --snr).
% This evaluates the tree topology when --onlyprecoding was used.
% The (mean) SNR is plotted over the reduction of the NOF transmissions (e) with
% increasing incoming packets at the sink. Also the distribution over e is
% plotted, when all packets have been received at the sink.
% The sink is expected to receive different NOF packets (<=> attempts) in each sequence.
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';  % Name of used algorithm
P0       = 8*85;    % NOF transmission without NC
%% INIT
nClusters = 3;      % NOF clusters used
%load('data.mat')
nTx =  zeros(1,nMeasSeq);

%% get max NOF tx to sink
nRxSink = l2+Cluster2.nPktRxCl(1:nMeasSeq);
nRxSinkMax = max(nRxSink);

%% Get NOF transmission
for c = 1:nClusters
    clName = ['Cluster' num2str(c-1)];
       
    nRxSrc = eval([clName '.nPktRxSrc']);
    nRxCl = eval([clName '.nPktRxCl']);
    nTx = nTx + nRxSrc +  nRxCl(1:nMeasSeq); %+1 since cluster head is also source
end
min_nTx = min(nTx);
max_nTx = max(nTx);
nTxS = nTx+nRxSink; % NOF tx with sink link

%% Get SNR ordered by NOF transmissions

snrSpat = nan(nMeasSeq, max_nTx-min_nTx+nRxSinkMax, nClusters);
snrSpatFin = zeros(nMeasSeq); %final SNR
for c = 1:nClusters
    for meas = 1:nMeasSeq
        %% spatial snr

        stField = eval([clName '.RecSeq'  num2str(meas-1)]);
        snrNow = stField;
        range = (nTx(meas) - min_nTx)+(1:nRxSink(meas));
        snrSpat(meas, range, c) = snrNow(:);
        snrSpatFin(meas) = snrNow(end);
    end
end
%% plot
%reordering to mean SNR with same number of transmissions
snrSpatMeanCl = squeeze(nanmean(snrSpat,3)); %mean over clusters
snrSpatMean =  squeeze(nanmean(snrSpatMeanCl));


%considering connection to sink
min_nTxS = min_nTx +1; % one transmission C2-> sink
max_nTxS = max_nTx + nRxSinkMax; % all transmissions C2->sink
eps = (min_nTxS:max_nTxS)/P0; 
% calculate distribution
nTxS_dist = histc(nTxS, min_nTx+1:max_nTxS)/nMeasSeq;
nTxS_distNan = nTxS_dist;
nTxS_distNan(nTxS_dist==0) = nan; %don't plot 0s

%plot
figure;
yyaxis left; plot(eps, snrSpatMean); ylabel('mean SNR in dB');
yyaxis right;stem(eps, nTxS_distNan); ylabel('% of SEQ');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('\epsilon_P'); 
figure;
snrFinal = snrSpatMean;
snrFinal(nTxS_dist==0) = nan;
yyaxis left;stem(eps, snrFinal);ylabel('mean SNR in dB');
yyaxis right;stem(eps, nTxS_distNan); ylabel('% of SEQ');
title(['Final SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('\epsilon_P'); 