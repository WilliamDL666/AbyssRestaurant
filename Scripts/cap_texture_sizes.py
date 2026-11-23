# Caps the big Tripo / model textures under /Game/Abyss at 2048 (4K x 4 maps per model filled VRAM). UI textures untouched.
import unreal
eal = unreal.EditorAssetLibrary
n = 0
for p in eal.list_assets("/Game/Abyss", recursive=True, include_folder=False):
    if "/UI/" in p:
        continue
    a = eal.load_asset(p)
    if isinstance(a, unreal.Texture2D) and max(a.blueprint_get_size_x(), a.blueprint_get_size_y()) > 2048:
        a.set_editor_property("max_texture_size", 2048)
        eal.save_loaded_asset(a, False)
        n += 1
unreal.log("[AbyssSetup] capped %d textures at 2048" % n)
