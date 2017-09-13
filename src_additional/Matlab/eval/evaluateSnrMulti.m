%%
%
% Evaluates the output for a multi cluster simulation. The snr was calculated during the simulation (flag: --snr).
% The sink is expected to receive the same NOF packets (<=> attempts) in each sequence.
% The SNR is averaged over all measurement sequences and clusters for the spatial reconstruction.
% The temporal SNR is averaged over the measurement sequences and the nodes.  
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';  % Name of used algorithm
nClusters = 3;      % NOF clusters used
attempts = 9;     % NOF reconstruction attempts per sequence
notemp   = true;    % Was temporal reconstruction disabled (--notemp)


%% INIT
%load('data.mat')
snrSpat = zeros(nMeasSeq, attempts, nClusters);
snrSpatMean = zeros(nMeasSeq, attempts);
spatVar = zeros(attempts, nClusters);

if(~notemp)
    snrTemp = zeros(nMeasSeq, attempts, nNodesUsed, nClusters);
    tempVar = zeros(attempts, nNodesUsed, nClusters);
    snrTempMean = zeros(attempts, nNodesUsed, nClusters);
    snrTempMedian = zeros(attempts, nNodesUsed, nClusters);
    snrTempMin = zeros(attempts, nNodesUsed, nClusters);
    snrTempMax = zeros(attempts, nNodesUsed, nClusters);
end
%% Iterate over clusters
for c = 1:nClusters
    
    clName = ['Cluster' num2str(c-1)];
    %% Iterate over all sequence and average
    for meas = 0:nMeasSeq-1
        %% spatial snr
        
        stField = eval([clName '.RecSeq'  num2str(meas)]);
        snrNow = stField;
        snrSpat(meas+1, :, c) = snrNow;
        
        
        %% temporal snr
        if(~notemp)
            for i = 1:nNodesUsed
                
                stField = eval([clName '.Node'  num2str(i-1) '.RecSeq'  num2str(meas)]);
                snrNow = stField;
                snrTemp(meas+1,:,i, c) = snrNow;
            end
        end
    end

    %% variance, mean... over attempts
    spatVar(:,c) = var(snrSpat(:,:,c));
    if(~notemp)
        tempVar(:,:,c) = squeeze(var(snrTemp(:,:,:,c),1));
        snrTempMean(:,:,c) = squeeze(mean(snrTemp(:,:,:,c))); 
        snrTempMedian(:,:,c) = squeeze(median(snrTemp(:,:,:,c)));   
        snrTempMin(:,:,c) = squeeze(min(snrTemp(:,:,:,c)));
        snrTempMax(:,:,c) = squeeze(max(snrTemp(:,:,:,c)));
    end
    
    %% plot for each cluster
    figure; 
    yyaxis left
    plot(squeeze(mean(snrSpat(:,:,c))));hold on;
    plot(squeeze(min(snrSpat(:,:,c))))
    plot(squeeze(max(snrSpat(:,:,c))))
    plot(squeeze(median(snrSpat(:,:,c))))
    ylabel('Snr in dB');
    yyaxis right
    plot(spatVar);
    ylabel('Variance in dB²');
    title(['SNR with ' ALGO_NAME ' Spatial Reconstruction ' clName]);xlabel('nPackets');
    legend({'mean','min','max','median','variance'})
   
    if(~notemp)
        figure;
        yyaxis left
        plot(mean(snrTempMean(:,:,c),2));ylabel('Snr in dB');
        plot(mean(snrTempMin(:,:,c),2));
        plot(mean(snrTempMax(:,:,c),2));
        plot(mean(snrTempMedian(:,:,c),2));
        xlabel('nPackets');
        title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over node) ' clName]);
        legend({'mean','min','max','median','variance'})
    end
end


%% plot

snrSpatMean(:,:) = squeeze(mean(snrSpat,3));
%snr spat over attempts
figure;
%yyaxis left
plot(squeeze(mean(snrSpatMean)));hold on;
% plot(squeeze(min(snrSpat,3)))
% plot(squeeze(max(snrSpat,3)))
% plot(squeeze(median(snrSpat)))
ylabel('mean SNR in dB');
%yyaxis right
% plot(spatVar);
% ylabel('Variance in dB²');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);xlabel('nPackets'); %legend({'mean','min','max','median','variance'})


% snr temp over attempts
if(~notemp)
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
end