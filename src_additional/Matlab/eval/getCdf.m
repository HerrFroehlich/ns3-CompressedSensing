% Gets the cdf for the attempts made at the sink. The No.
% attempts in each sequence must be the same and that the SNR
% was directly calculated in the simulation by running with --snr
%% SETTINGS
ALGO_NAME = 'SP';  % Name of used algorithm
nonc = true;      %NC of clusters disabled (--nonc)
SNRmin = [100];       %minimum SNR in dB
scen = 0;  %Scenario: 0-Single cluster 1-tree 2-diamond 3-four clusters
minP = 0; %minimum No. so that sink starts decoding (--minP)

%% Init
N =  nNodesUsed;
switch scen
    case 0 %single
        nClusters = 1;
        P0 = (N-1)+N;    % NOF transmission without NC
    case 1 %tree
        nClusters = 3;
        P0 = 3*(N-1)+5*N;    % NOF transmission without NC
    case 2 %diamond
        nClusters = 3;
        P0 = 3*(N-1)+5*N;    % NOF transmission without NC
    case 3 %4 clusters
        nClusters = 4;
        P0 = 4*(N-1)+10*N;    % NOF transmission without NC
    otherwise
        error('Invalid scenario!')
end
attempts = numel(Cluster0.RecSeq0);
%% Iterate over clusters
nTx = zeros(1, nMeasSeq);
cdf = zeros(attempts, numel(SNRmin));
for c = 1:nClusters
    clName = ['Cluster' num2str(c-1)];
    %% Iterate over Nodes
    for j = 1:N
        %% Iterate over all sequences
        for meas = 0:nMeasSeq-1
            stField = eval([clName '.Node' num2str(j-1) '.RecSeq'  num2str(meas)]);
            for s = 1:numel(SNRmin)
                cdf(:,s) = cdf(:,s) + (stField>SNRmin(s))';
            end
        end
    end
    nRxSrc = eval([clName '.nPktRxSrc']);
    nTx = nTx + nRxSrc;  %overall NOF tx wo link to sink
    if(~nonc)
        nTx = nTx + eval(['nc' num2str(c-1)]);
    else
        nRl = eval([clName '.nPktRxCl']);
        if(scen>0)
            nTx = nTx + eval(['l' num2str(c-1)]) + nRl(1:nMeasSeq);
        else
            nTx = nTx + l;
        end
    end
end
cdf = cdf/(nMeasSeq*N*nClusters);
g = nTx/P0 *m/n;   %gain in each measurement sequence
gMean = mean(nTx)/P0 *m/n;
%% gain with increasin transmissions to sink

minP = minP+1;
if(~nonc)
    switch scen
        case 0 %single
            gIncr = (mean(Cluster0.nPktRxSrc)+(minP:l))/P0;
        case 1 %tree
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                Cluster2.nPktRxSrc+Cluster2.nPktRxCl(1:nMeasSeq))+(minP:l2))/P0;
        case 2 %diamond
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                Cluster1.nPktRxCl(1:nMeasSeq)+Cluster2.nPktRxSrc+...
                Cluster2.nPktRxCl(1:nMeasSeq))...
                +(minP:(l1+l2)))/P0;
        case 3 %4 clusters
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                Cluster1.nPktRxCl(1:nMeasSeq)+Cluster2.nPktRxSrc+...
                Cluster2.nPktRxCl(1:nMeasSeq)+...
                Cluster3.nPktRxSrc+Cluster3.nPktRxCl(1:nMeasSeq))...
                +(minP:(l3+l2)))/P0;
        otherwise
            error('Invalid scenario!')
    end
else
    switch scen
        case 0 %single
            gIncr = (mean(Cluster0.nPktRxSrc)+(minP:l))/P0;
        case 1 %tree
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                Cluster2.nPktRxSrc)+nc1+nc0+(minP:nc2))/P0;
        case 2 %diamond
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                +Cluster2.nPktRxSrc)+nc0+(minP:(nc1+nc2)))/P0;
        case 3 %4 clusters
            gIncr = (mean(Cluster0.nPktRxSrc+Cluster1.nPktRxSrc+...
                Cluster2.nPktRxSrc+Cluster3.nPktRxSrc)+nc0+nc1...
                +(minP:(nc3+nc2)))/P0;
        otherwise
            error('Invalid scenario!')
    end
end
gIncr = gIncr* m/n;
%%
figure;
if(attempts > 1)
plot(repmat(gIncr',1,numel(SNRmin)), cdf','-x');
else
    lstyles = {'x','o','+','*'};
    for p = 1:numel(SNRmin)
        hold on;
    plot(gIncr, cdf(p),lstyles{mod(p,numel(lstyles))});
    end
end
title(['Decoding probability  with ' ALGO_NAME]);
xlabel('$g_D \bar g_S$','Interpreter', 'latex');
ylabel('$p(SNR_i > SNR_{min})$','Interpreter', 'latex')
for s = 1:numel(SNRmin)
lcell{s} = ['$SNR_{min}=' num2str(SNRmin(s)) '$'];
end
legend(lcell,'Interpreter', 'latex')