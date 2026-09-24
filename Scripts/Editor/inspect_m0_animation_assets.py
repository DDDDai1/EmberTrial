"""Read saved M0 animation metadata and probe a transient rig; saves no UE assets."""
import json
from pathlib import Path
import unreal

workspace = Path(__file__).resolve().parents[2]
out = workspace / 'Docs/Reports/Raw/M0_05A_AssetInspection.json'
out.parent.mkdir(parents=True, exist_ok=True)
def path(obj):
    return obj.get_path_name() if obj else None
def vector(v):
    return [round(float(v.x),6), round(float(v.y),6), round(float(v.z),6)]
def transform(t):
    r=t.rotation.rotator()
    return {'location':vector(t.translation),'rotation_xyz':[r.roll,r.pitch,r.yaw],'scale':vector(t.scale3d)}
def load(asset):
    obj=unreal.load_asset(asset)
    assert obj is not None, asset
    return obj

clip=load('/Game/EmberTrial/Animations/Practice/Clips/AS_M0_Attack_01')
bp=load('/Game/Characters/Mannequins/Animations/ABP_Quinn')
player_bp=load('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
player=unreal.get_default_object(player_bp.generated_class())
mesh=player.get_editor_property('mesh')
mesh_asset=mesh.get_skeletal_mesh_asset()
data={'engine':unreal.SystemLibrary.get_engine_version(),
      'player':{'mesh':path(mesh_asset),'skeleton':path(mesh_asset.get_editor_property('skeleton')),
                'anim_class':path(mesh.get_editor_property('anim_class')),
                'animation_mode':str(mesh.get_editor_property('animation_mode'))},
      'anim_bp':{'asset':path(bp),'parent_class_tag':unreal.EditorAssetLibrary.find_asset_data('/Game/Characters/Mannequins/Animations/ABP_Quinn').get_tag_value('ParentClass')},
      'attack':{'asset':path(clip),'skeleton':path(clip.get_editor_property('skeleton')),
                'num_frames':unreal.AnimationLibrary.get_num_frames(clip),
                'num_keys':unreal.AnimationLibrary.get_num_keys(clip),
                'length_seconds':unreal.AnimationLibrary.get_sequence_length(clip),
                'enable_root_motion':clip.get_editor_property('enable_root_motion')}}
out.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
seq=load('/Game/EmberTrial/Animations/Practice/Sequence/LS_M0_Attack_01')
rate=seq.get_display_rate()
data['level_sequence']={'display_rate':[rate.numerator,rate.denominator],
    'playback_start':seq.get_playback_start(),'playback_end':seq.get_playback_end()}
data['attack']['root_samples']=[transform(unreal.AnimationLibrary.get_bone_pose_for_time(clip,'root',
    data['attack']['length_seconds']*i/30,False)) for i in range(31)]
rig_bp=load('/Game/Characters/Mannequins/Rigs/CR_Mannequin_Body')
rig_bp.recompile_vm()
rig=rig_bp.create_control_rig()
h=rig.get_hierarchy()
assert rig.execute('Construction')
assert rig.execute('Forwards Solve')
controls={str(k.name):k for k in h.get_controls()}
data['control_names']=list(controls)
data['probes']=[]
data['defaults']={}
for name in ['chest_ctrl','spine_01_ctrl','spine_02_ctrl','spine_03_ctrl','body_ctrl']:
    k=controls[name]
    data['defaults'][name]={'local':transform(h.get_local_transform(k)),
                          'global':transform(h.get_global_transform(k))}
    original=h.get_control_value(k)
    for axis in range(3):
        xyz=[0,0,0]
        xyz[axis]=15
        value=unreal.EulerTransform(location=unreal.Vector(0,0,0),
            rotation=unreal.Rotator(pitch=xyz[1],yaw=xyz[2],roll=xyz[0]),scale=unreal.Vector(1,1,1))
        h.set_control_value(k,h.make_control_value_from_euler_transform(value))
        assert rig.execute('Forwards Solve')
        points={n:transform(h.get_global_transform(unreal.RigElementKey(name=n,type=unreal.RigElementType.BONE)))
                for n in ['root','pelvis','spine_01','spine_05','head','foot_l','foot_r','hand_l','hand_r']}
        data['probes'].append({'control':name,'rotation_xyz':xyz,'bones':points})
        h.set_control_value(k,original)
        assert rig.execute('Forwards Solve')
out.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('M0_ASSET_INSPECTION_COMPLETE: '+str(out))
