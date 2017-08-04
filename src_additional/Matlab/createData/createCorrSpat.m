% Create signals sparse in time.
% The signals's coefficients are chosen with a auto regression model and a
% normal distribution and are set to zero at the same indices
% The signals are correlated spatially with a parameterized correlation
% coefficient.
% 
% Author : Tobias Waurick
% Date   : 31.07.17

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

snrDb = input('SNR [db] (Inf for no noise):');

%% Create temporal correlated signals
k = round(rho * nSamp);
rng('shuffle'); %set seed based on current time

X = zeros(nSamp,nNodes);

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

X = X*U;
figure;
image(corr(X, X), 'CDataMapping','scaled');
title( 'Resulting Correlation matrix:')
colorbar;
% 
% Y = zeros(nNodes, m);
% for i=1:nNodes
%     %     A = randn(m,n);
%     %      Y(i,:) = A*X(:,i);
%     idx = randperm(nSamp, m); % random subsampling
%     x =  X(:,i);
%     Y(i,:) = x(idx);
% end
%% Add gaussian noise
X0 = X; %without noise

sigmaN = db2mag(-snrDb);
X = X + sigmaN*randn(nSamp,nNodes);


%% write to file
mkdir('./inputData');
if(writeMatFile)
%     save(fileBaseMat, 'x', '-v6');
    save(fileBaseMat, 'X', 'X0','k', '-v6');
else
    for i=1:nNodes
        fid = fopen([fileBase num2str(i-1)],'w');
        fwrite(fid, X(:,i),'double');
        fclose(fid);d
    end
end  
fid = fopen([fileBase 'INFO'], 'w');
fprintf(fid, 'Number of nodes: %d\n', nNodes);
fprintf(fid, 'Number of samples: %d\n', nSamp);
fprintf(fid, 'Sparsity ratio: %f\n', rho);
fprintf(fid, 'Spatial Correlation: %f\n', corrSpat);
fprintf(fid, 'Temporal Correlation: %f\n', corrTemp);
fprintf(fid, 'SNR: %f\n', snrDb);
fclose(fid);

%% plot informative figures
figure;
x = k:nSamp;
y = k * log10(x);
plot(x,y);
title(['Minimum m with OMP for k=' num2str(k)]); xlabel('n'); ylabel('m');

% figure; % see Book Cs Sensing:Theory&Applications p.254
% x= 1:nSamp;
% y = k*log(x).^4;
% plot(x,y);
% title(['Minimum m for Random Subsampling for k=' num2str(k)]); xlabel('n'); ylabel('m');

figure;
y = zeros(1,nNodes);
for l = 1:nNodes;
    y(l) = binocdf(k,l,k/nSamp);
end
plot(y);
title(['Probability for precoded Y column sparsity : k<=' num2str(k)]); 
xlabel('l'); ylabel('P');

figure;
for x = 1:nSamp;
    y(x) = 1 - binopdf(0,x,k/nSamp);
end
plot(y);
title('Probability for precoded Y row sparsity : k!=0'); 
xlabel('m'); ylabel('P');

figure;
x = 1:nNodes;
y = k * log10(x);
plot(x,y);
title(['Minimum l with OMP for k=' num2str(k)]); xlabel('nNodes'); ylabel('l');


