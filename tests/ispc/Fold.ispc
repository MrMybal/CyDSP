// Original experimental kernel, outside the production library.
inline double triangle(double x) {
    double r=x-4.0d*floor(x*.25d+.5d); double a=abs(r);
    return a<=1.0d?r:(r<0.0d?-1.0d:1.0d)*(2.0d-a);
}
inline double primitive(double x) {
    double r=abs(x-4.0d*floor(x*.25d+.5d));
    return r<=1.0d?.5d*r*r:2.0d*r-.5d*r*r-1.0d;
}
export void fold(uniform const float input[],uniform float output[],uniform int n,
                 uniform double state[],uniform double drive,uniform double bias) {
    uniform double old=state[0];uniform bool active=state[1]!=0.0d;
    double dc=triangle(bias);
    foreach(i=0...n) {
        double x=clamp((double)input[i]*drive+bias,-1e6d,1e6d);
        double previous=i==0?old:clamp((double)input[max(i-1,0)]*drive+bias,-1e6d,1e6d);
        double delta=x-previous;
        double y;
        if(i==0&&!active)y=triangle(x);
        else if(abs(delta)>1e-5d*max(1.0d,max(abs(x),abs(previous))))y=(primitive(x)-primitive(previous))/delta;
        else y=triangle(.5d*(x+previous));
        output[i]=(float)(y-dc);
    }
    if(n>0){state[0]=clamp((double)input[n-1]*drive+bias,-1e6d,1e6d);state[1]=1.0d;}
}
