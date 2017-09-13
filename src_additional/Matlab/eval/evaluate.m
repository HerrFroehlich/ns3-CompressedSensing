%%
%
% Evaluates the output from a single cluster simulation. The snr was not calculated during the simulation.
% The SNR is averaged over all measurement sequences for the spatial reconstruction.
% The temporal SNR is averaged over the measurement sequences and the nodes.  
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';
%% INIT
%load('data.mat')
snrSpat = zeros(attempts,1);
snrTemp = zeros(nNodesUsed, attempts);

%% Iterate over all sequence and average
for meas = 0:nMeasSeq-1
    %% spatial snr
    
    %get reconstructed Y
    Yrec = zeros(nNodesUsed, m, attempts);
    for i = 1:attempts
        st = eval(['Cluster0.RecSeq'  num2str(meas)]);
        data = st(:,i);
        data = reshape(data, [], m); %written column by column
        Yrec(:,:,i) = data;
    end

    %get original Y
    Y = zeros(nNodesUsed, m);
    for i = 1:nNodesUsed

        st = eval(['Cluster0.Node'  num2str(i-1)]);
        Y(i,:) = st.Compressed(:,meas+1);
    end

 
    for i = 1:attempts
        errorSpat = abs(Y-Yrec(:,:,i));
        snrSpat(i) = (meas*snrSpat(i)+snr(Y, errorSpat))/(meas+1); % moving average
    end

    %% temporal snr

    Xrec = zeros(n, nNodesUsed);
    xrange = (meas*n)+1:n*(meas+1);
    Xnow = X(xrange,:);

    for a = 1:attempts
        for i = 1:nNodesUsed

            st = eval(['Cluster0.Node'  num2str(i-1) '.RecSeq'  num2str(meas)]);
            xRec = st(:,a);
            snrTemp(i,a) = (meas*snrTemp(i,a)+...
                snr(Xnow(:,i), abs(xRec - Xnow(:,i))))/(meas+1);% moving average

            Xrec (:,i) = xRec;
        end
    end
end
%% plot

% snr spat over attempts
figure;
plot(snrSpat);
xlabel('nPackets'); ylabel('Snr in dB');
title(['SNR with ' ALGO_NAME ' Spatial Reconstruction']);


% snr temp over attempts
figure;
plot(mean(snrTemp));
xlabel('nPackets'); ylabel('Mean Snr in dB');
title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over node)']);

%snr temp over sequence
figure;
plot(mean(snrTemp,2));
xlabel('Node'); ylabel('Mean Snr in dB');
title(['SNR with ' ALGO_NAME ' Temporal Reconstruction (mean over sequence)']);