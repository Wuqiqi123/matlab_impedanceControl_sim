clear;
Ftimeseries=load('compare_force_contact_Fe.mat');
Xtimeseries=load('compare_force_contact_X.mat');
DesiredXtimeseries=load('compare_force_contact_DesiredX.mat');
DesiredFetimeseries=load('compare_force_contact_DesiredFe.mat');
t=Ftimeseries.Fe.time;
Fz=Ftimeseries.Fe.Data(:,6);
DesiredX=DesiredXtimeseries.DesiredX.Data;
X=Xtimeseries.X.Data;
DesiredFe=DesiredFetimeseries.DesiredFe.Data;
DesiredFe=DesiredFe(:,6);
X4=X(1,4,:);
X6=X(1,6,:);
X5=X(1,5,:);

DesiredX4=DesiredX(:,4);
DesiredX5=DesiredX(:,5);
DesiredX6=DesiredX(:,6);

XX5=zeros(2001,1);
XX4=zeros(2001,1);
XX6=zeros(2001,1);


for i=1:2001
    XX5(i,1)=X5(1,1,i);
end  
for i=1:2001
    XX4(i,1)=X4(1,1,i);
end 
for i=1:2001
    XX6(i,1)=X6(1,1,i);
end 


[Ax,h1,h2]=plotyy(t,[DesiredX6,XX6],t,[DesiredFe,Fz]);

set(get(Ax(1),'Ylabel'),'String','球心沿Z轴的位置/m','Fontsize',23);
set(get(Ax(2),'Ylabel'),'String','平面对球的力/N','Fontsize',23,'Color','magenta');


set(Ax(1),'Fontsize',22) % handles可以指定具体坐标轴的句柄
set(Ax(2),'Fontsize',22) % handles可以指定具体坐标轴的句柄

set(Ax(1),'ylim',[-0.05 0.5]) % handles可以指定具体坐标轴的句柄
set(Ax(2),'YLim',[-100 1000],'YColor','magenta') % handles可以指定具体坐标轴的句柄
%set(h1,'XTick',0:1:10) % handles可以指定具体坐标轴的句柄
set(Ax(1),'YTick',-0.05:0.05:0.5) % handles可以指定具体坐标轴的句柄
set(Ax(2),'YTick',-100:100:1000) % handles可以指定具体坐标轴的句柄

l1=xlabel('时间/s','fontsize',22);

h1(1).Color='b';
h1(1).LineStyle = '-';
h1(1).Marker='+';
h1(1).MarkerSize=1;
h1(1).LineWidth = 0.6;
h1(1).MarkerEdgeColor='b';

h1(2).Color='r';
h1(2).LineStyle = '-';
h1(2).Marker='diamond';
h1(2).MarkerSize=1;
h1(2).LineWidth = 0.6;
h1(2).MarkerEdgeColor='r';

% h1(3).Color='b';
% h1(3).LineStyle = '--';
% h1(3).Marker='+';
% h1(3).MarkerSize=1;
% h1(3).LineWidth = 0.6;
% h1(3).MarkerEdgeColor='b';

% h1(4).Color='r';
% h1(4).LineStyle = '-';
% h1(4).Marker='diamond';
% h1(4).MarkerSize=1;
% h1(4).LineWidth = 0.6;
% h1(4).MarkerEdgeColor='r';
% 
% h1(5).Color='r';
% h1(5).LineStyle = '-';
% h1(5).Marker='diamond';
% h1(5).MarkerSize=1;
% h1(5).LineWidth = 0.6;
% h1(5).MarkerEdgeColor='r';

% h1(6).Color='r';
% h1(6).LineStyle = '-';
% h1(6).Marker='diamond';
% h1(6).MarkerSize=1;
% h1(6).LineWidth = 0.6;
% h1(6).MarkerEdgeColor='r';

h2(1).Color='b';
h2(1).LineStyle = '-';
h2(1).Marker='.';
h2(1).MarkerSize=0.9;
h2(1).LineWidth = 0.8;
h2(1).MarkerEdgeColor='b';

h2(2).Color='magenta';
h2(2).LineStyle = '-';
h2(2).Marker='.';
h2(2).MarkerSize=0.9;
h2(2).LineWidth = 0.8;
h2(2).MarkerEdgeColor='magenta';


k1=legend('期望位置Xdz','实际位置Xz','期望接触力Fe','实际接触力Fz','fontsize',16);
%k2=title('轴1阻抗控制曲线','fontsize',15);

% h=figure(1);
% set(h,'color','w');
% set(gca,'fontsize',20);
% set(Ax(2),'Fontsize',20);
set(Ax(1),'XTick',0:1:5);
% hold on