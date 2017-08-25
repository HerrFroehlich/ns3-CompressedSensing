% Create plots to determine the tx probability modifier of the source
% nodes, so that the cluster head receives l measurement vectors with high
% probability according to a binomial distrbution
% 
% Author : Tobias Waurick
% Date   : 16.08.17

%% Settings
nNodes = 256; %nodes in cluster
lRange = 2.^(3:7); % l sweep range 
modRange = 1:0.01:3; % mu sweep range

%% calculate for modifier

nMod = numel(modRange);
nL = numel(lRange);
p = zeros(nMod, nL);

for j = 1:nL
    l = lRange(j);
    for i = 1:nMod
        mu = modRange(i);
        p(i, j) = 1 - binocdf((l-1),nNodes,mu* (l-1)/(nNodes-1)); %binomial distribution
    end
end

%% plot
legendCell = cellstr(num2str(lRange','l=%d'));


figure;
plot(repmat(modRange',1,nL), p);
title('Probability that at least l-1 meas. vectors where received');
xlabel('\mu');
ylabel('P(R \geq l-1)');
legend(legendCell);

%% calculate for tx probability in general
txProb = 0:0.01:1;
nTxProb = numel(txProb);
p2 = zeros(nTxProb, nL);

for j = 1:nL
    l = lRange(j);
    for i = 1:nTxProb
        p2(i, j) = 1 - binocdf((l-1),nNodes,txProb(i)); %binomial distribution
    end
end

%% plot
figure;
plot(repmat(txProb',1,nL), p2);
title('Probability that at least l-1 meas. vectors where received');
xlabel('p_{tx}');
legend(legendCell);   
ylabel('P(R \geq l-1)');