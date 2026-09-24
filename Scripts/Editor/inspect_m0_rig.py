"""Evaluate documented M0 poses in a separate UE editor commandlet.
Saves no Unreal assets. Output: Docs/Reports/Raw/M0_05A_RigInspection.json.
Use UE 5.5.4 -run=pythonscript -EnablePlugins=PythonScriptPlugin.
"""
import json
import math
from pathlib import Path
import unreal

workspace = Path(__file__).resolve().parents[2]
spec = json.loads((workspace / 'Docs/Data/M0_05A_AttackPoseValues.json').read_text(encoding='utf-8'))
out = workspace / 'Docs/Reports/Raw/M0_05A_RigInspection.json'
out.parent.mkdir(parents=True, exist_ok=True)
bp = unreal.load_object(None, spec['rig'] + '.CR_Mannequin_Body')
bp.recompile_vm()  # Compiles only the copy loaded in this separate process.
rig = bp.create_control_rig()
h = rig.get_hierarchy()
assert rig.execute('Construction'), 'Construction failed'
assert rig.execute('Forwards Solve'), 'Forwards Solve failed'

def key(name):
    return unreal.RigElementKey(name=name, type=unreal.RigElementType.CONTROL)

def vec(v):
    return [round(float(v.x),6), round(float(v.y),6), round(float(v.z),6)]

def transform(t):
    r = t.rotation.rotator()
    return {'location':vec(t.translation), 'rotation_xyz':[r.roll,r.pitch,r.yaw],
            'quaternion_xyzw':[t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w],
            'scale':vec(t.scale3d)}

def evaluate(controls):
    for name, xyz in controls.items():
        value = unreal.EulerTransform(location=unreal.Vector(0,0,0),
            rotation=unreal.Rotator(pitch=xyz[1],yaw=xyz[2],roll=xyz[0]),scale=unreal.Vector(1,1,1))
        h.set_control_value(key(name), h.make_control_value_from_euler_transform(value))
    assert rig.execute('Forwards Solve'), 'Pose evaluation failed'
    return {str(k.name):transform(h.get_global_transform(k)) for k in h.get_bones()}

def distance(a,b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))

data = {'engine':unreal.SystemLibrary.get_engine_version(), 'rig':spec['rig'],
        'controls':{}, 'reference_poses':{}, 'sampled_frames':{}}
for name in spec['frames']['0']:
    k = key(name)
    data['controls'][name] = {'local':transform(h.get_local_transform(k)),
        'global':transform(h.get_global_transform(k)),
        'offset_global':transform(h.get_global_control_offset_transform(k))}
    assert distance(data['controls'][name]['local']['location'],[0,0,0]) < 0.001
    assert distance(data['controls'][name]['local']['scale'],[1,1,1]) < 0.001
for frame,controls in spec['frames'].items():
    data['reference_poses'][frame] = evaluate(controls)
frames = sorted(int(f) for f in spec['frames'])
for frame in range(31):
    a = max(f for f in frames if f <= frame)
    b = min(f for f in frames if f >= frame)
    alpha = 0 if a == b else (frame-a)/(b-a)
    controls = {name:[x+(y-x)*alpha for x,y in zip(values,spec['frames'][str(b)][name])]
                for name,values in spec['frames'][str(a)].items()}
    data['sampled_frames'][str(frame)] = evaluate(controls)
baseline = data['reference_poses']['0']
fixed = ['root','pelvis','foot_l','foot_r','upperarm_l','lowerarm_l','hand_l']
drift = max(distance(p[name]['location'],baseline[name]['location'])
            for p in data['sampled_frames'].values() for name in fixed)
assert drift < 0.001, 'Root, feet or left arm moved unexpectedly'
return_error = max(distance(baseline[name]['location'],data['reference_poses']['30'][name]['location']) for name in baseline)
assert return_error < 0.001, 'End pose did not return to start'
flexion = []
for p in data['sampled_frames'].values():
    shoulder,elbow,wrist = [p[name]['location'] for name in ['upperarm_r','lowerarm_r','hand_r']]
    u = [x-y for x,y in zip(elbow,shoulder)]
    v = [x-y for x,y in zip(wrist,elbow)]
    cosine = sum(x*y for x,y in zip(u,v))/(distance(u,[0,0,0])*distance(v,[0,0,0]))
    flexion.append(math.degrees(math.acos(max(-1,min(1,cosine)))))
assert min(flexion)>10 and max(flexion)<140, 'Elbow overextension/folding'
data['checks'] = {'fixed_bone_max_location_drift_cm':drift,
    'frame_0_to_30_max_bone_location_difference_cm':return_error,
    'right_elbow_flexion_range_deg':[min(flexion),max(flexion)],
    'samples':31, 'interpolation':'Linear control Euler values',
    'limitations':'No skinned-mesh collision or visual playback test; no original sequence edited or baked.'}
out.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('M0_RIG_INSPECTION_COMPLETE: ' + str(out))
