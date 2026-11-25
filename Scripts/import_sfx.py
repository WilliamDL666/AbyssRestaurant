# Imports the synthesised sound effects (Scripts/make_sfx.py) into /Game/Abyss/Sfx.
import os, glob
import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Sfx")
for f in sorted(glob.glob(os.path.join(SRC, "S_*.wav"))):
    name = os.path.splitext(os.path.basename(f))[0]
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(f))
    t.set_editor_property("destination_path", "/Game/Abyss/Sfx")
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tools.import_asset_tasks([t])
    unreal.log("[AbyssSetup] sfx %s ok=%s" % (name, eal.does_asset_exist("/Game/Abyss/Sfx/" + name)))
