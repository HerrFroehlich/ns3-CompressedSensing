
%% SETTINGS
FILE_PATH = './out/data';       %output mat file base name
m = 64;                         %size of compressed measurement vector
%n = 512;                        %size of measurement vector per sequence

                      %size of measurement vector per sequence
%% DEFINES
MAX_NODES = 256;

%% INPUT
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');
rho_c = input('Common Sparsity Ratio: ');
assert(rho_c>=0 && rho_c<=1, 'Sparsity Ratio must be 0...1');

fc = input('Maximum discrete frequency of common part [Default 1]:');
if isempty(fc)
    fc = 1;
end
assert(fc>=0 && fc<=1, 'Frequency must be 0...1');
rng('shuffle'); %set seed based on current time

%%

k_c = round(rho_c * nSamp);
fIdx = round(fc*nSamp);
if fIdx < k_c
    k_c = fIdx;
end

idx = randperm(fIdx, k_c);

Xf = zeros(nSamp, nNodes);
Xf(idx,:) = randn(k_c, nNodes);
X = idct(Xf);

%% write to file
save(FILE_PATH, 'X', 'k', '-v6');

fid = fopen([FILE_PATH 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
fprintf(fid, 'Common sparsity kc: %d\n', k_c);
fprintf(fid, 'Innovative sparsity ki: %d\n', k);
fprintf(fid, 'Power ratio common/innovative: %d\n', var_c);
fprintf(fid, 'Maximum discrete frequency of common part: %f\n', fc);
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
    x =  X(:,i);
    Y(i,:) = x(idx);
end

%% informative plots
close all
% Xc DCT
figure;
f = repmat((0:nSamp-1)'/nSamp,1,nNodes);
mesh(1:nNodes,f, abs(dct(X)));
title ('DCT of common(X)')
xlabel('Node');
ylabel('F');

% X DCT
figure;
f = repmat((0:nSamp-1)'/nSamp,1,nNodes);
mesh(1:nNodes,f, abs(dct(X)));
title ('DCT of X, all Samples')
xlabel('Node');
ylabel('F');
zlabel('|A|');

% Y DCT with random subsampling
figure;
f = repmat((0:nNodes-1)'/nNodes,1,m);
mesh(1:m,f, abs(dct(Y)));
title ('DCT of Y = A*X')
xlabel('Column');
ylabel('F');
zlabel('|A|');