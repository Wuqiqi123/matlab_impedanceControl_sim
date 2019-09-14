%plot(time,Vel,time,theta,time,Torque,time,simtheta.signals.values,time,simVel.signals.values);%一张图三条曲线
% [AX,h1,h2]=plotyy(time,[Vel;Torque;simVel.signals.values],time,[theta;simtheta.signals.values]);%一张图三条曲线
%errvel=simVel.signals.values-Vel;
clc

time=xlsread('C:\Users\wqq\Desktop\Data\Robotdata0.1-0.2-kalman-0.0075-0.05-0.1.csv','V2:V1502');%读取横坐标
Vel=xlsread('C:\Users\wqq\Desktop\Data\Robotdata0.1-0.2-kalman-0.0075-0.05-0.1.csv','B2:B1502');%读取第一条曲线
theta=xlsread('C:\Users\wqq\Desktop\Data\Robotdata0.1-0.2-kalman-0.0075-0.05-0.1.csv','A2:A1502');%读取第二条曲线
Torque=xlsread('C:\Users\wqq\Desktop\Data\Robotdata0.1-0.2-kalman-0.0075-0.05-0.1.csv','W2:W1502');%读取第三条曲线
errVel=simVel.signals.values-Vel;
errPos=simtheta.signals.values-theta;
 [hAx,h1,h2]=plotyy(time,errVel,time,errPos);
% [hAx,h1,h2]=plotyy(time,[Vel,simVel.signals.values,Torque],time,[theta,simtheta.signals.values]);

h1(1).LineStyle = '-';
% h1(1).Marker='.';
% %h1(1).MarkerSize=7;
% h1(1).MarkerEdgeColor='b';
h1(1).LineWidth=0.7;
% 
% h1(2).Color='r';
% h1(2).LineWidth=1.5;
% 
% h1(3).Color=[0.2,0.3,0];
% h1(3).LineWidth=1.5;
% 
%h2(1).Color='g';
h2(1).LineStyle = '-';
h2(1).LineWidth = 2;
% % h2(1).Marker='*';
% % h2(1).MarkerSize=3;
% h2(1).MarkerEdgeColor='c';
% 
% h2(2).Color='m';
% h2(2).LineStyle = '--';
% h2(2).LineWidth=2;
% 
 ylabel(hAx(1),'速度偏差(deg/s)','fontsize',22) % left y-axis 
 ylabel(hAx(2),'角度偏差(deg)','fontsize',22) % right y-axis
 set(hAx(1),'ylim',[-2 2]) % handles可以指定具体坐标轴的句柄
 set(hAx(2),'ylim',[-0.5 0.5]) % handles可以指定具体坐标轴的句柄
 set(hAx(1),'YTick',-2:0.5:2) % handles可以指定具体坐标轴的句柄
 set(hAx(2),'YTick',-0.5:0.1:0.5) % handles可以指定具体坐标轴的句柄
% 
 set(hAx(1),'Fontsize',22) % handles可以指定具体坐标轴的句柄
 set(hAx(2),'Fontsize',22) % handles可以指定具体坐标轴的句柄

 k1=legend('速度偏差','角度偏差','fontsize',16);
 %k2=title('关节1阻抗控制曲线','fontsize',20);
 l1=xlabel('时间/s','fontsize',22);
% l2=ylabel('力矩、速度、角度值');
% set(l1,'fontsize',14);
% set(l2,'fontsize',14);
% set(k1,'fontsize',13); 
% set(k2,'fontsize',15);
% 
% errvel=simVel.signals.values-Vel;
% plot(time,errvel);%一张图三条曲线
% k1=legend('速度误差(deg/s)');
% k2=title('第一根轴速度误差');
% l1=xlabel('时间(t)');
% l2=ylabel('速度误差值');
% set(l1,'fontsize',14);
% set(l2,'fontsize',14);
% set(k1,'fontsize',13);
% set(k2,'fontsize',15);

% errtheta=simtheta.signals.values-theta;
% plot(time,errtheta);%一张图三条曲线
% k1=legend('角度误差(deg/s)');
% k2=title('第一根轴角度误差');
% l1=xlabel('时间(t)');
% l2=ylabel('角度误差值');
% set(l1,'fontsize',14);
% set(l2,'fontsize',14);
% set(k1,'fontsize',13);
% set(k2,'fontsize',15);