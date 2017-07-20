%% SETTINGS
fileBase= './inputData/x';

%% DEFINES
MAX_NODES = 256;

%% INPUT
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');
rho = input('Sparsity Ratio: ');
assert(rho>=0 && rho<=1, 'Sparsity Ratio must be 0...1');

correlation = input('Spatial correlation of nodes: ');
assert(correlation>=0 && correlation<=1, 'correlation must be 0...1');

%% Create signals
k = round(rho * nSamp);
rng('shuffle'); %set seed based on current time

X = randn(nSamp,nNodes); % create nSamp gaussian samples

%% make sparse
% choose sparse indices randomly
% for i = 1:nNodes
%
% idx = randperm(nSamp, nSamp-k);
% X(idx,i) = 0;
%
% end;

%%choose indices randomly but same for every node
idx = randperm(nSamp, nSamp-k);
X(idx,:) = 0;
%% Create correlated signals

corrMat = eye(nNodes);
corrMat(corrMat==0) = correlation;

U = chol(corrMat);

Xc = X*U;
figure;
title( 'Resulting Correlation matrix:')
image(corr(Xc, Xc), 'CDataMapping','scaled');
colorbar;

%% write to file
mkdir('./inputData');
for i=1:nNodes
    fid = fopen([fileBase num2str(i-1)],'w');
    fwrite(fid, Xc(:,i),'double');
    fclose(fid);
end

fid = fopen([fileBase 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
fprintf(fid, 'Sparsity ratio: %f\n', rho);
fprintf(fid, 'Spatial Correlation: %f\n', correlation);
fclose(fid);

