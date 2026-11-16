# L02 Lunar Modular Kit

42 modular part types (including 3 rock variants), 7 editable prefab recipes, 4 LODs each.

Open `source/L02_LunarKit_Layout.blend` for the exact 513×513 R16 lunar terrain, existing S07 reference crate,
layout instances, Blender Asset Browser collections, part catalog and LOD comparison scenes.
The saved layout contains 24 prefab instances and 120 props across four bases, connected by five
abandoned service roads. There are 46 rock/rock-cluster instances and 1,451 shared Box colliders.
See `previews/L03_PlacementMap.png` and `previews/L03_WorldOverview.png`.
The terrain, crater, settlement, road and player-start specification is in
[`Docs/lunar-base-layout.md`](../../Docs/lunar-base-layout.md).

Edit `L03_WORLD_Authoring` in Blender: Bezier roads, level base pads, mountain/crater guides and
dust/gravel/rock regions. Run the embedded `export_lunar_world.py` to bake R16 height data,
linear surface-mask DDS, placements, shared collision, traversal routes and save the `.blend`.
The C++ runtime reads these exported files; it does not parse `.blend` directly.
Run `author_lunar_base.py --apply-plan` only to explicitly reset the map to the supplied sketch plan.

See [`Docs/lunar-modular-kit.md`](../../Docs/lunar-modular-kit.md) for Korean assembly, Blender placement,
custom prefab, export, LOD, texture and destruction integration instructions and validation results.

Engine: C++20 / DirectX12, existing `Physics::Box` collision. These are collection/recipe prefabs, not Unity `.prefab` files.
OBJ is RH Y-up with UV V preflipped for the existing loader. Blender source uses standard UVs.

Runtime rendering uses static prefab meshes and distance LOD. Destruction meshes/IDs/support graph are supplied;
runtime damage, fracture simulation and replication remain future work. Blender uses full supplied PBR maps;
the current engine building material samples BaseColor with constant metallic/roughness.
