%%
%
% Evaluates the output for a 3 simulation with 4cluster topolgy. 
% The snr was calculated during the simulation (flag: --snr).
% There were no errors between the cluster heads (flag:  --errMax=0)
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'SP';  % Name of used algorithm
minP = 20;         %minimum Packets at sink to start decoding (--minP) 
nonc = true;      %NC of clusters disabled (--nonc)
%% INIT11
N = nNodesUsed;
attempts = numel(Cluster0.RecSeq0);
P0       = 4*(N-1)+10*N;    % NOF transmission without NC
nClusters = 4;      % NOF clusters used
%load('data.mat')
snrSpat = zeros(nMeasSeq, attempts, nClusters);
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
    nTx = nTx + nRxSrc;  %overall NOF tx wo link to sink
end

if(~nonc)
    nTx = nTx + nc0 + nc1 + nc2 + nc3; 
    nTxSink = nc2 + nc3;
else
    nTx = nTx + l0; %C0 transmissions
    nTx = nTx + l1 + Cluster1.nPktRxCl(1:nMeasSeq); %C1 transmissions
    nTx = nTx + l2 + Cluster2.nPktRxCl(1:nMeasSeq); %C2 transmissions
    nTx = nTx + l3 + Cluster3.nPktRxCl(1:nMeasSeq); %C3 transmissions
    nTxSink = l2 + Cluster2.nPktRxCl(1:nMeasSeq) + l3 + Cluster3.nPktRxCl(1:nMeasSeq);
end

%% reordering to mean SNR with same number of transmissions
snrSpatMeanCl = squeeze(mean(snrSpat,3)); %mean over clusters
min_nTx = min(nTx);
max_nTx = max(nTx);
max_nTxS = max(nTxSink);
snrSpat_nRx = nan(nMeasSeq, (max_nTxS + max_nTx-min_nTx));

nTxS = max_nTxS; 
for meas = 1:nMeasSeq
    
    if numel(nTxSink) > 1 %with nonc we have a different NOF tx to sink in case of errors
        nTxS = nTxSink(meas);
    end
    idx = nTx(meas) - min_nTx;
    snrSpat_nRx(meas, (idx+(minP:nTxS))) = snrSpatMeanCl(meas, :);
end
snrSpatMean = nanmean(snrSpat_nRx);

%considering connection to sinl
min_nTx = min_nTx +1; % one transmission C2-> sink
max_nTx = max_nTx + nTxSink; % all transmissions C2->sink
eps = (min_nTx:max_nTx)/P0; 

% calculate distribution
nTx_dist = histc(nTx + nTxSink,min_nTx:max_nTx)/nMeasSeq; %also considering fixed tx to sink

epsFinal_b = (eps(nTx_dist~=0));
snrSFinal_b = (snrSpatMean(nTx_dist~=0));

%% plot
figure;
yyaxis left; plot(eps, snrSpatMean); ylabel('mean SNR in dB');
yyaxis right;plot(eps, nTx_dist); ylabel('% of SEQ');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('\epsilon_P'); 