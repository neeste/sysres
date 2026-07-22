% dpenv.m - compute DP envelope using hetrodyne method
% 1-13-02
% corresponds to Sysres V178, that includes fit for phase and level of DP
%
function dpenv(in_file, ef, SR, dbrange, dpon, out_file)
load (in_file);   % SYSRES data file
dpen=4096;          % number of samples in DP envelope
dpbw=320;            % low-pass filter bandwidth (Hz)
os=5904;             % offset = 123ms x 48kHz
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% heterodyne analysis
fdp=f1+dpon*(f2-f1);
spc=fft(resp);
nsm=length(resp);
ndp=1+round((nsm/2)*(fdp/fmax));
nwd=dpen/2-1;
new=[spc(ndp:ndp+nwd) 0 spc(ndp-nwd:ndp-1)];
nsb=3*dpbw*nsm/(2*fmax);
lpf=zeros(1,dpen);lpf(1)=1;
for n=1:min(nwd,nsb),
    p=n*pi/(nsb+1);
    w=0.42+0.5*cos(p)+0.08*cos(p*2);    % Blackman window
    lpf(1+n)=w; lpf(dpen+1-n)=w;
end
new=new.*lpf;
scl=(2.828e-5*sens_mp)*(nsm/dpen)/2;
rsp=ifft(new)/scl;
dplv=20*log10(abs(rsp));
dpph=imag(log(rsp))/(2*pi);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% exponential fit
sr=2*fmax;
t=[0:dpen-1]/sr*(nsm/dpen);
to=(os/sr)*ones(1,dpen);
ftlv=ef(1)*exp((to-t)/ef(2)) + ef(3)*exp((to-t)/ef(4)) + ef(5);
if (length(ef) < 8)
   ef=[ef, 0 0 0]; 
end;
ftph=ef(6)*exp((to-t)/ef(2)) + ef(7)*exp((to-t)/ef(4)) + ef(8);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% plot DP envelope
eo=1+floor(os*dpen/nsm);
tmax=nsm/sr;
ax=[0 tmax SR-dbrange SR];
axis(ax);
plot(t,dplv,t(eo:dpen),ftlv(eo:dpen));
axis(ax);
pause(1);
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% save envelope data in ASCII file
ofp = fopen(out_file, 'wt');
fprintf(ofp, '; %s - from SYSRES file "%s"\n', out_file, in_file);
fprintf(ofp, ';\n');
fprintf(ofp, '; exponential fit = %.3f  %.3f  %.3f  %.3f  %.3f\n', ef(1:5));
fprintf(ofp, ';                   %.4f  %.4f  %.4f\n', ef(6:8));
fprintf(ofp, '; time offset = %.3f  (msec)\n', to(1)*1000);
fprintf(ofp, '; DP order number = %d\n', dpon);
fprintf(ofp, '  m = %.0f ; (dB SPL)\n', SR);
fprintf(ofp, '  r = %.0f ; (dB SPL)\n', dbrange);
fprintf(ofp, ';\n');
fprintf(ofp, ';    time     DP_lev   fit_lev   DP_phase fit_phase\n');
s=[t ; dplv ; ftlv ; dpph ; ftph];
fprintf(ofp, ' %9.6f %9.3f %9.3f %9.4f %9.4f\n', s);
fclose(ofp);
