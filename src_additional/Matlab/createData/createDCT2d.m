%% SETTINGS
FILE_PATH = './out/data';       %output mat file base name
m = 64;                         %size of compressed measurement vector
%n = 512;                        %size of measurement vector per sequence
%% DEFINES
MAX_NODES = 256;
RAND_MIN = 0.5;
RAND_MAX = 1.5;
%% INPUT
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');
rho = input('Sparsity Ratio: ');
assert(rho>=0 && rho<=1, 'Sparsity Ratio must be 0...1');

nMeas = input('NOF measurement sequences [Default 1]:');
if isempty(nMeas)
    nMeas = 1;
end
assert(nMeas >=0, 'Must be greater/equal zero!');


var_c = input('Power of signal [Default 1]:');
if isempty(var_c)
    var_c = 1;
end
assert(var_c >=0, 'Must be greater/equal zero!');

% snrDb = input('SNR [db] (Default Inf for no noise):');
% if isempty(snrDb)
%     snrDb = Inf;
% end
%% random dct coefficients

X = zeros(nSamp*nMeas, nNodes);

for i = 0:nMeas-1
    k = round(rho * nSamp);

    Xf = zeros(nSamp, nNodes);
   
    fx = randperm(nSamp, k); %k sparse in temporal domain
    fy = randperm(nNodes,k); %k sparse in spatial domain
    idx =  fx.*fy;
    Xf(idx) =  rand(k, 1)*(RAND_MAX-RAND_MIN) + RAND_MIN;
    %normalize so we have the wanted signal power var_c       
    norm = var_c /var(Xf(:));
    Xf(idx) = Xf(idx) * sqrt(norm);
    %calculate Xi
    Xi = idct2(Xf);
    range = (i*nSamp)+1:(i+1)*nSamp;
    X(range,:) = Xi;
end
%% add noise


%% write to file
save(FILE_PATH, 'X', 'k', '-v6');

fid = fopen([FILE_PATH 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
fprintf(fid, 'Number of sequences: %d\n', nMeas);
fprintf(fid, 'Sparsity k: %d\n', k);
fprintf(fid, 'Power: %f\n', var_c);
fclose(fid);


%% Calculate Y to check

Y = zeros(nNodes, m);
A = randn(m,nSamp);
idx = randperm(nSamp, m); % random subsampling
for i=1:nNodes
  %  A = randn(m,nSamp);
  %   Y(i,:) = A*X(:,i);
  % idx = randperm(nSamp, m); % random subsampling
    idx = sort(idx);
    x =  Xi(:,i);
    Y(i,:) = x(idx);
end

%% informative plots
close all

% X DCT
figure;
f = repmat((0:nSamp-1)'/nSamp,1,nNodes);
mesh(1:nNodes,f, abs(dct(Xi)));
title ('DCT of X, last sequence')
xlabel('Node');
ylabel('F');
zlabel('|A|');

% Y DCT with random subsampling
figure;
f = repmat((0:nNodes-1)'/nNodes,1,m);
mesh(1:m,f, abs(dct(Y)));
title ('DCT of Y = A*X, last sequence')
xlabel('Column');
ylabel('F');
zlabel('|A|');
