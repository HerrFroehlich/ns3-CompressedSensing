%%
%
% Evaluates the output from a single cluster simulation for a single measurement sequence.
% The snr was not calculated during the simulation.
%
% Be sure to load the data first!
%
% Author: Tobias Waurick
%%

%% SETTINGS
ALGO_NAME = 'OMP';
MEAS_SEQ = 0;
%% get data
% load('dataOut.mat')
%load('data.mat')
data = eval(['Cluster0.RecSeq'  num2str(MEAS_SEQ)]);
recSpat = reshape(data, nNodesUsed, m); %written column by column

Y = zeros(nNodesUsed, m);
for i = 1:nNodesUsed

    st = eval(['Cluster0.Node'  num2str(i-1)]);
    Y(i,:) = st.Compressed(:, MEAS_SEQ+1);
end

%% spatial snr
errorSpat = abs(Y-recSpat);
snrSpat = snr(Y, errorSpat)
figure;
mesh(errorSpat, 'CDataMapping','scaled');
colorbar;
ylabel('nNodes'); xlabel('column'); zlabel('err');
title(['Absolute Error with  ' ALGO_NAME ' Reconstruction']);

%% temporal snr

snrTemp = zeros(nNodesUsed, 1);
Xrec = zeros(n, nNodesUsed);

for i = 1:nNodesUsed

    xRec = eval(['Cluster0.Node'  num2str(i-1) '.RecSeq' num2str(MEAS_SEQ)]);
    range = MEAS_SEQ*n+1:(MEAS_SEQ+1)*n;
    snrTemp(i) = snr(X(range,i), abs(xRec - X(range,i)));
    
    Xrec (:,i) = xRec;

end
figure;
plot(snrTemp);
xlabel('Node'); ylabel('Snr in dB');
title(['SNR with ' ALGO_NAME ' Reconstruction']);
mean(snrTemp)

%% additional plots
% x vs xrec
figure;
subplot(1,2,1);image(X(range,:), 'CDataMapping','scaled');title('X');
subplot(1,2,2); image(Xrec, 'CDataMapping','scaled');title('Xrec'); 

% dct y vs dct yrec
figure;h1 = subplot(1,2,1);mesh(abs(dct(Y)));
title ('DCT of Y')
xlabel('Column');
ylabel('F');
zlabel('|A|');
h2 = subplot(1,2,2); mesh(abs(dct(recSpat)));
title ('DCT of Yrec')
xlabel('Column');
ylabel('F');
zlabel('|A|');
linkprop([h1 h2], 'CameraPosition');
% y vs yrec

figure;h3 = subplot(1,2,1);mesh(Y);
title ('Y')
xlabel('Column');
ylabel('n');
zlabel('A');
h4 = subplot(1,2,2); mesh(recSpat);
title ('Yrec')
xlabel('Column');
ylabel('n');
zlabel('A');
linkprop([h3 h4], 'CameraPosition');