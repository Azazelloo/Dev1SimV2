clc;
clear;

file = fopen('disp.txt','r');
A = fscanf(file,'%d %f',[2 Inf]);
A=A';

% file=fopen('myBuf.bin','r');
% A=fread(file,10000,'uint32');



figure();
subplot(1,1,1);
stairs(A(:,2));
ylim([-0.5 1.5]);


fclose(file);