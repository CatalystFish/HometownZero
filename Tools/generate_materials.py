"""Headless material generation for Hometown Zero (run via -run=pythonscript).

Creates /Game/HZMaterials/M_HZ_<name> solid-color materials for every district
category plus road/ground/zombie. Deletes the folder first so every run is
deterministic. Safe to re-run.
"""
import unreal

CATEGORY_COLORS = {
    "medical": (0.90, 0.25, 0.25, 1.0),
    "emergency": (0.95, 0.45, 0.10, 1.0),
    "education": (0.95, 0.80, 0.30, 1.0),
    "food": (0.95, 0.60, 0.20, 1.0),
    "hardware": (0.55, 0.40, 0.25, 1.0),
    "weapons_outdoors": (0.20, 0.45, 0.20, 1.0),
    "fuel": (0.60, 0.30, 0.70, 1.0),
    "retail": (0.20, 0.65, 0.60, 1.0),
    "office": (0.35, 0.50, 0.75, 1.0),
    "industrial": (0.45, 0.48, 0.52, 1.0),
    "residential": (0.82, 0.72, 0.58, 1.0),
    "civic": (0.85, 0.50, 0.70, 1.0),
    "unknown": (0.60, 0.60, 0.60, 1.0),
    "road": (0.16, 0.16, 0.18, 1.0),
    "ground": (0.30, 0.32, 0.28, 1.0),
    "zombie": (0.30, 0.65, 0.25, 1.0),
}

PACKAGE_DIR = "/Game/HZMaterials"


def set_color(node, color):
    try:
        node.constant = unreal.LinearColor(*color)
        return "attr"
    except Exception:
        node.set_editor_property("constant", unreal.LinearColor(*color))
        return "set_editor_property"


def main():
    if unreal.EditorAssetLibrary.does_directory_exist(PACKAGE_DIR):
        unreal.EditorAssetLibrary.delete_directory(PACKAGE_DIR)
    unreal.EditorAssetLibrary.make_directory(PACKAGE_DIR)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    made = 0
    for name, color in CATEGORY_COLORS.items():
        mat = tools.create_asset(f"M_HZ_{name}", PACKAGE_DIR,
                                 unreal.Material, unreal.MaterialFactoryNew())
        if not mat:
            unreal.log_error(f"[HZMat] failed to create M_HZ_{name}")
            continue
        node = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant3Vector, 0, 0)
        via = set_color(node, color)
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        made += 1
        unreal.log(f"[HZMat] M_HZ_{name} color={color} via={via}")
    unreal.log(f"[HZMat] done: {made} materials")


main()
