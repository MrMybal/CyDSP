"""Generate ORIGINAL MIT test signals, not measured HRTF datasets.
Development-only dependency: h5py + numpy. Never run by CMake/production builds.
"""
from pathlib import Path
import hashlib,json,sys
import h5py,numpy as np
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'tests/data/sofa';OUT.mkdir(parents=True,exist_ok=True)
def make(name,libver='v108',chunk=False,dense=False,f32=False,rotated=False,
         convention='SimpleFreeFieldHRIR',vary_rate=False,bad_receiver=False,
         maxshape=None,bigendian=False,single=False,empty=False,shuffle=True):
    dtype=('>' if bigendian else '<')+('f4' if f32 else 'f8')
    with h5py.File(OUT/name,'w',libver=(libver,'v108' if libver=='earliest' else libver)) as f:
        attrs={'Conventions':'SOFA','SOFAConventions':convention,'DataType':'FIR',
               'Title':'CyDSP synthetic fixture','DatabaseName':'Original MIT test data',
               'ListenerShortName':'Synthetic','License':'MIT'}
        for k,v in attrs.items():f.attrs[k]=v if dense else np.bytes_(v)
        if empty:
            f.attrs["Title"]=h5py.Empty("S1")
            d=f.create_dataset("ListenerUp",data=np.asarray([[0,0,1]],dtype=dtype))
        if dense:
            for i in range(24):f.attrs['Extra'+str(i)]='attribute '+str(i)
            for i in range(24):f.create_dataset('Extra'+str(i),data=[0])
        for key,n in zip('ICRENM',(1,3,2,1,8,2)):
            d=f.create_dataset(key,data=np.zeros(n,dtype='f4'))
            d.attrs['CLASS']=np.bytes_('DIMENSION_SCALE')
            d.attrs['NAME']=np.bytes_('This is a netCDF dimension but not a netCDF variable. '+str(n))
        def dataset(key,data,type=None,units=None,**kwargs):
            d=f.create_dataset(key,data=np.asarray(data,dtype=dtype),**kwargs)
            if type:d.attrs['Type']=np.bytes_(type)
            if units:d.attrs['Units']=np.bytes_(units)
            return d
        ir=np.zeros((2,2,8));ir[0,0,0]=.25;ir[0,1,0]=.5+2**-40
        ir[1,0,0]=.125;ir[1,1,0]=.75;ir[:,:,2]=.0625
        opts={}
        if chunk:opts=dict(chunks=(2,2,8) if single else (1,2,4),compression='gzip',shuffle=shuffle)
        if maxshape:opts['maxshape']=maxshape
        dataset('Data.IR',ir,**opts)
        dataset('Data.SamplingRate',[48000,44100] if vary_rate else [48000],units='hertz')
        dataset('Data.Delay',[[1.25,2.5]])
        if rotated:
            dataset('SourcePosition',[[1,3,3],[0,2,3]],'cartesian','metre')
            dataset('ListenerPosition',[[1,2,3]],'cartesian','metre')
            dataset('ListenerView',[[0,1,0]],'cartesian','metre')
            dataset('ListenerUp',[[0,0,1]],'cartesian','metre')
        else:dataset('SourcePosition',[[0,0,1],[90,0,1]],'spherical','degree, degree, metre')
        receiver=np.zeros((2,3,1));receiver[:,1,0]=[.09,.1] if bad_receiver else [-.09,.09]
        dataset('ReceiverPosition',receiver,'cartesian','metre')
        dataset('EmitterPosition',np.zeros((1,3,1)),'cartesian','metre')
for name,options in {
    'legacy-v0.sofa':dict(libver='earliest'),
    'compact-v2.sofa':dict(),
    'compact-v1-chunks.sofa':dict(chunk=True),
    'compact-f32.sofa':dict(f32=True),
    'modern-fixed.sofa':dict(libver='v110',chunk=True),
    'modern-single.sofa':dict(libver='v110',chunk=True,single=True),
    'modern-extensible.sofa':dict(libver='v110',chunk=True,maxshape=(None,2,8)),
    'modern-dense.sofa':dict(libver='v110',chunk=True,dense=True),
    'bigendian.sofa':dict(bigendian=True),
    'rotated.sofa':dict(rotated=True),
    'general-fir.sofa':dict(convention='GeneralFIR'),
    'unsupported.sofa':dict(convention='GeneralTF'),
    'varying-rate.sofa':dict(vary_rate=True),
    'invalid-receiver.sofa':dict(bad_receiver=True),
    'empty-attributes.sofa':dict(empty=True),
    'deflate-only.sofa':dict(chunk=True,shuffle=False),
}.items():make(name,**options)
manifest={'license':'MIT','description':'Original synthetic CyDSP unit impulses. No measured dataset.',
          'generator':'scripts/generate_sofa_fixtures.py',
          'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(OUT.glob('*.sofa'))}}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Generated',len(manifest['files']),'original SOFA fixtures')
