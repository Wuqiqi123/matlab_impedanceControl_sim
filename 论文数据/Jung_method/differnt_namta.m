clear;
F1timeseries=load('compare_force_contact_Fe_namta0.1.mat');
F2timeseries=load('compare_force_contact_Fe_namta0.0005.mat');
F3timeseries=load('compare_force_contact_Fe_namta0.01.mat');
F4timeseries=load('compare_force_contact_Fe_namta0.001.mat');
F5timeseries=load('compare_force_contact_Fe_namta0.167.mat');
DesiredFetimeseries=load('compare_force_contact_DesiredFe_namta0.1.mat');
t=F1timeseries.Fe.time;
Fz1=F1timeseries.Fe.Data(:,6);
Fz2=F2timeseries.Fe.Data(:,6);
Fz3=F3timeseries.Fe.Data(:,6);
Fz4=F4timeseries.Fe.Data(:,6);
Fz5=F5timeseries.Fe.Data(:,6);

DesiredFe=DesiredFetimeseries.DesiredFe.Data;
DesiredFe=DesiredFe(:,6);





plot(t,DesiredFe,t,Fz2,'r-*',t,Fz4',t,Fz3,t,Fz1,'g-+',t,Fz5,'LineWidth',1.2);
yticks(0:200:2000);
ylabel('平面对球的力/N','Fontsize',22,'Color','black');
xlabel('时间/s','fontsize',22);
legend('期望力fd','\lambda=0.0005','\lambda=0.001','\lambda=0.01','\lambda=0.1','\lambda=0.167','fontsize',16);


l1=xlabel('时间/s','fontsize',22);


