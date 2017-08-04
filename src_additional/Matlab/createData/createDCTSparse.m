% Create signals sparse in a DCT.
% The DCT coefficients are chosen from a normal distribution.
% A mean is added by setting the 0th DCT coefficient to the abs. sum of the
% coefficients. This ensures sparsity whe compressing spatially after 
% temporally compression via  random subsampling.
% The signals are correlated spatially with a parameterized correlation
% coefficient.
% 
% Author : Tobias Waurick
% Date   : 31.07.17

%% SETTINGS
fileBase= './out/x';
fileBaseMat= './out/data';
writeMatFile = true; % write to a mat file instead of multiple bin files
m = 64;
%% DEFINES
MAX_NODES = 256;

%% INPUT
nNodes = input('Number of nodes: ');
assert(nNodes > 0, 'Not enough nodes');
assert(nNodes <= MAX_NODES, 'too many nodes');

nSamp = input('NOF Samples: ');
rho = input('Sparsity Ratio: ');
assert(rho>=0 && rho<=1, 'Sparsity Ratio must be 0...1');

corrSpat = input('Spatial correlation of nodes: ');
assert(corrSpat>=0 && corrSpat<=1, 'correlation must be 0...1');

%% Create DCT sparse signals

rng('shuffle'); %set seed based on current time
k = round(rho * nSamp);

Xf = randn(nSamp,nNodes);
idx = randperm(nSamp, nSamp-k+1);
Xf(idx,:) = 0;
Xf(1,:) = abs(sum(Xf));

figure;stem(Xf);title('DCT of independent signals');

X = idct(Xf);

corrMat = eye(nNodes);
corrMat(corrMat==0) = corrSpat;
U = chol(corrMat);
Xc = X * U;

figure;plot(Xc);title('Spatially correlated signals');
figure; image(corr(Xc, Xc), 'CDataMapping','scaled');
title( 'Resulting Correlation matrix:'); colorbar;

%% write to file
mkdir('./inputData');
if(writeMatFile)
    x = Xc;
%     save(fileBaseMat, 'x', '-v6');
    save(fileBaseMat, 'x','k', '-v6');
else
    for i=1:nNodes
        fid = fopen([fileBase num2str(i-1)],'w');
        fwrite(fid, Xc(:,i),'double');
        fclose(fid);
    end
end  
fid = fopen([fileBase 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
 fprintf(fid, 'Sparsity ratio: %f\n', rho);
fprintf(fid, 'Spatial Correlation: %f\n', corrSpat);
fclose(fid);

%% Create Y  for checking spatial sparsity
Y = zeros(nNodes, m);
for i=1:nNodes
%     A = randn(m,nSamp);
%      Y(i,:) = A*X(:,i);
    idx = randperm(nSamp, m); % random subsampling
    %sort(idx);
    x =  Xc(:,i);
    Y(i,:) = x(idx);
end

%% plot informative figures

figure;
plot(abs(dct(Y(:,1)))); title('DCT Amplitude of Ys first column');

figure;
x = k:nSamp;
y = k * log10(x);
plot(x,y);
title(['Minimum m with OMP for k=' num2str(k)]); xlabel('nSamp'); ylabel('m');
figure;
x = 1:nNodes;
y = k * log10(x);
plot(x,y);
title(['Minimum l with OMP for k=' num2str(k)]); xlabel('nNodes'); ylabel('l');

