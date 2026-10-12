# L02 Lunar Modular Kit

39 modular part types, 7 editable prefab recipes, 4 LODs each. Original authored geometry and atlas.

Open `source/L02_LunarKit_Layout.blend` for the exact 513×513 R16 lunar terrain, existing S07 reference crate,
layout instances, Blender Asset Browser collections, part catalog and LOD comparison scenes.
The saved layout contains 27 prefab instances and 73 individual props across eight sites,
with 1,240 existing Box colliders. See `previews/L02_PlacementMap.png` for the terrain overview.
The original terrain RAW is unchanged; sites lie on the walkable terrain component connected to the central base.

See [`Docs/lunar-modular-kit.md`](../../Docs/lunar-modular-kit.md) for Korean assembly, Blender placement,
custom prefab, export, LOD, texture and destruction integration instructions and validation results.

Engine: C++20 / DirectX12, existing `Physics::Box` collision. These are collection/recipe prefabs, not Unity `.prefab` files.
OBJ is RH Y-up with UV V preflipped for the existing loader. Blender source uses standard UVs.

Runtime rendering uses static prefab meshes and distance LOD. Destruction meshes/IDs/support graph are supplied;
runtime damage, fracture simulation and replication remain future work. Blender uses full supplied PBR maps;
the current engine building material samples BaseColor with constant metallic/roughness.
