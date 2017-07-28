%% SETTINGS
fileBase= './inputData/x';
fileBaseMat= './inputData/data';
writeMatFile = true; % write to a mat file instead of multiple bin files
m = 128;
%% DEFINES
MAX_NODES = 256;

%% INPUT
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');
rho = input('Sparsity Ratio: ');
assert(rho>=0 && rho<=1, 'Sparsity Ratio must be 0...1');

corrTemp = input('Temporal correlation of nodes: ');
assert(corrTemp>=0 && corrTemp<=1, 'correlation must be 0...1');

corrSpat = input('Spatial correlation of nodes: ');
assert(corrSpat>=0 && corrSpat<=1, 'correlation must be 0...1');

%% Create temporal correlated signals
k = round(rho * nSamp);
rng('shuffle'); %set seed based on current time

%X = randn(nSamp,nNodes);
X = zeros(nSamp,nNodes); % create nSamp gaussian samples

for i=1:nNodes
    
    xi = zeros(nSamp,1);
    
    xi(1) = randn();
    %create dependent samples
    for j=2:nSamp
        xi(j) = randn() + corrTemp * xi(j-1); %autoregressive model
    end
    
    X(:,i) = xi;
end


%% make sparse
% choose sparse indices randomly
% for i = 1:nNodes
%
%  idx = randperm(nSamp, nSamp-k);
%  X(idx,:) = 0;
%
% end;

%%choose indices randomly but same for every node
idx = randperm(nSamp, nSamp-k);
X(idx,:) = 0;
%% Create correlated signals

corrMat = eye(nNodes);
corrMat(corrMat==0) = corrSpat;

U = chol(corrMat);

Xc = X*U;
figure;
image(corr(Xc, Xc), 'CDataMapping','scaled');
title( 'Resulting Correlation matrix:')
colorbar;

Y = zeros(nNodes, m);
for i=1:nNodes
    %     A = randn(m,n);
    %      Y(i,:) = A*X(:,i);
    idx = randperm(nSamp, m); % random subsampling
    x =  X(:,i);
    Y(i,:) = x(idx);
end
%% write to file
mkdir('./inputData');
if(writeMatFile)
    x = Xc;
    save(fileBaseMat, 'x', '-v6');
else
    for i=1:nNodes
        fid = fopen([fileBase num2str(i-1)],'w');
        fwrite(fid, Xc(:,i),'double');
        fclose(fid);
    end
    
    fid = fopen([fileBase 'INFO'], 'w');
    fprintf(fid, 'Number of nodes: %d\n', nNodes);
    fprintf(fid, 'Number of samples: %d\n', nSamp);
    fprintf(fid, 'Sparsity ratio: %f\n', rho);
    fprintf(fid, 'Spatial Correlation: %f\n', corrSpat);
    fprintf(fid, 'Temporal Correlation: %f\n', corrTemp);
    fclose(fid);
end

