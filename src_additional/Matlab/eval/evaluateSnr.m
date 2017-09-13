%%
%
% Evaluates the output from a single cluster simulation. The snr was calculated during the simulation (flag: --snr).
% The SNR is averaged over all measurement sequences for the spatial reconstruction.
% The temporal SNR is averaged over the measurement sequences and the nodes.  
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';
CLUSTER ='Cluster0';
%% INIT
%load('data.mat')
snrSpat = zeros(nMeasSeq, attempts);
snrTemp = zeros(nMeasSeq, attempts, nNodesUsed);

%% Iterate over all sequence and average
for meas = 0:nMeasSeq-1
    %% spatial snr

    stField = eval([CLUSTER '.RecSeq'  num2str(meas)]);
    snrNow = stField;
    snrSpat(meas+1, :) = snrNow;


    % temporal snr
    for i = 1:nNodesUsed

        stField = eval([CLUSTER '.Node'  num2str(i-1) '.RecSeq'  num2str(meas)]);
        snrNow = stField;
        snrTemp(meas+1,:,i) = snrNow;
    end
end
spatVar = var(snrSpat);
tempVar = squeeze(var(snrTemp));
snrTempMean = squeeze(mean(snrTemp));
snrTempMedian = squeeze(median(snrTemp));
snrTempMin = squeeze(min(snrTemp));
snrTempMax = squeeze(max(snrTemp));


%% plot

% snr spat over attempts
figure;
yyaxis left
plot(mean(snrSpat));hold on;
plot(min(snrSpat))
plot(max(snrSpat))
plot(median(snrSpat))
ylabel('Snr in dB');
yyaxis right
plot(spatVar);
xlabel('nPackets'); ylabel('Variance in dB²');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);
legend({'mean','min','max','median','variance'})

% snr temp over attempts
figure;
yyaxis left
plot(mean(snrTempMean,2)); hold on;ylabel('Snr in dB');
plot(mean(snrTempMin,2));
plot(mean(snrTempMax,2));
plot(mean(snrTempMedian,2));
yyaxis right
plot(mean(tempVar,2)); ylabel('Variance in dB²');
xlabel('nPackets');
title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over node)']);
legend({'mean','min','max','median','variance'})
% snr temp over attempts wo node 0
% figure;
% yyaxis left
% plot(mean(snrTempMean(:,2:end),2)); ylabel('Mean Snr in dB');
% yyaxis right
% plot(mean(tempVar(:,2:end),2));
% xlabel('nPackets'); ylabel('Variance in dB²');
% title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over node, wo node 0)']);

%snr temp over sequence
% figure;
% yyaxis left
% plot(mean(snrTempMean));ylabel('Mean Snr in dB');
% yyaxis right
% plot(mean(tempVar)); ylabel('Variance in dB²');
% xlabel('nPackets');
% xlabel('Node'); 
% title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over sequence)']);