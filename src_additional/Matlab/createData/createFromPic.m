% Create signals for source nodes from a bmp-picture.
% Each picture column corresponds to the input data of a node.

%% Settings
FILE_PATH = './out/data';       %output mat file
PIC_PATH = './.lena512.bmp';    %input picture
m = 128;                        % to check Y column sparsity
%% Defines
MAX_NODES = 256;


%% Input
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');

%% Load Picture
dir = fileparts(mfilename('fullpath'));
picByte = imread(fullfile(dir, PIC_PATH));
pic = double(picByte)/256;

dim = size(pic);

assert(dim(2) >= nNodes, 'not enough rows in picture');
assert(dim(1) >= nSamp, 'not enough columns in picture');

%% Crop picture
X = pic(1:nSamp, 1:nNodes);
% X = imresize(pic, [nNodes, nSamp]);

%% write to file
save(FILE_PATH, 'X', '-v6');

fid = fopen([FILE_PATH 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
fclose(fid);

%% Calculate Y to check
if(m > nSamp)
    m = floor(nSamp/2);
end

Y = zeros(nNodes, m);
for i=1:nNodes
%     A = randn(m,n);
%      Y(i,:) = A*X(:,i);
    idx = randperm(nSamp, m); % random subsampling
    idx = sort(idx);
    x =  X(:,i);
    Y(i,:) = x(idx);
end

%% informative plots

% X DCT
figure;
f = repmat((0:nSamp-1)'/nSamp,1,nNodes);
mesh(1:nNodes,f, abs(dct(X)));
title ('DCT of X')
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
