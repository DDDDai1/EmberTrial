"""Evaluate the documented M0-05B controls in memory; no UE assets are saved."""
import json
import math
from pathlib import Path
import unreal

workspace=Path(__file__).resolve().parents[2]
spec=json.loads((workspace/'Docs/Data/M0_05B_DefensivePoseValues.json').read_text(encoding='utf-8'))
out=workspace/'Docs/Reports/Raw/M0_05B_RigInspection.json'
out.parent.mkdir(parents=True,exist_ok=True)
bp=unreal.load_asset(spec['rig'])
bp.recompile_vm()
def key(name,t=unreal.RigElementType.CONTROL):
    return unreal.RigElementKey(name=name,type=t)
def xyz(v):
    return [float(v.x),float(v.y),float(v.z)]
def distance(a,b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))
result={'engine':unreal.SystemLibrary.get_engine_version(),'clips':{}}
for name,clip in spec['clips'].items():
    rig=bp.create_control_rig()
    h=rig.get_hierarchy()
    assert rig.execute('Construction')
    for switch,value in spec['fk_switches'].items():
        h.set_control_value(key(switch),h.make_control_value_from_bool(value))
    assert rig.execute('Forwards Solve')
    for control in clip['frames']['0']:
        t=h.get_local_transform(key(control))
        assert distance(xyz(t.translation),[0,0,0])<0.001
        assert distance(xyz(t.scale3d),[1,1,1])<0.001
    frames=sorted(int(f) for f in clip['frames'])
    samples={}
    for frame in range(frames[-1]+1):
        a=max(f for f in frames if f<=frame)
        b=min(f for f in frames if f>=frame)
        alpha=0 if a==b else (frame-a)/(b-a)
        for control,rotation in clip['frames'][str(a)].items():
            r=[x+(y-x)*alpha for x,y in zip(rotation,clip['frames'][str(b)][control])]
            t=unreal.EulerTransform(location=unreal.Vector(0,0,0),
                rotation=unreal.Rotator(pitch=r[1],yaw=r[2],roll=r[0]),scale=unreal.Vector(1,1,1))
            h.set_control_value(key(control),h.make_control_value_from_euler_transform(t))
        assert rig.execute('Forwards Solve')
        samples[str(frame)]={str(k.name):xyz(h.get_global_transform(k).translation) for k in h.get_bones()}
    first=samples['0']
    fixed=['root','pelvis','foot_l','foot_r']
    drift=max(distance(s[n],first[n]) for s in samples.values() for n in fixed)
    returned=max(distance(first[n],samples[str(frames[-1])][n]) for n in first)
    assert drift<0.001 and returned<0.001
    flex=[]
    for sample in samples.values():
        for side in ['l','r']:
            shoulder,elbow,wrist=[sample[n+'_'+side] for n in ['upperarm','lowerarm','hand']]
            u=[x-y for x,y in zip(elbow,shoulder)]
            v=[x-y for x,y in zip(wrist,elbow)]
            cosine=sum(x*y for x,y in zip(u,v))/(distance(u,[0,0,0])*distance(v,[0,0,0]))
            flex.append(math.degrees(math.acos(max(-1,min(1,cosine)))))
    assert min(flex)>10 and max(flex)<145
    peak='4' if name=='Dodge' else '3'
    head_delta=[x-y for x,y in zip(samples[peak]['head'],first['head'])]
    if name=='Dodge':
        assert head_delta[0]<-10, 'Dodge must lean toward character right (-X)'
    else:
        assert head_delta[1]<-8, 'Hit reaction must lean backward (-Y)'
    result['clips'][name]={'checks':{'samples':len(samples),'fixed_bone_drift_cm':drift,
        'end_to_start_bone_position_error_cm':returned,'elbow_flexion_range_deg':[min(flex),max(flex)],
        'peak_head_delta_xyz_cm':head_delta},'poses':{str(f):samples[str(f)] for f in frames}}
result['limits']='Transient rig geometry checks only; user Sequencer playback, skin deformation and baking remain untested.'
out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('M0_DEFENSIVE_POSES_COMPLETE: '+str(out))
