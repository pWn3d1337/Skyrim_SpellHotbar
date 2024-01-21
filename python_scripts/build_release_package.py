from pathlib import Path
from zipfile import ZipFile, ZIP_STORED
from glob import glob

# this scripts builds a releasable zip file


dev_mod_root = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar")
dev_mod_root_nordic_ui = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI")

project_root = Path(__file__).parent.parent

released_files_main_plugin = [
    (project_root / "skse_plugin/build/release/SpellHotbar.dll", "SKSE/Plugins"),
    (dev_mod_root / "SpellHotbar.esp", ""),
    (dev_mod_root / "Scripts/*.pex", dev_mod_root),  # if Path, add relative path to root in zip
    (dev_mod_root / "Scripts/Source/*.psc", dev_mod_root),
    (dev_mod_root / "Interface/SpellHotbar/spell_icons.swf", dev_mod_root),
    (dev_mod_root / "meshes/SpellHotbar/*.nif", dev_mod_root),
    (dev_mod_root / "meshes/**/*.txt", dev_mod_root),
    (dev_mod_root / "meshes/**/*.hkx", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/InventoryInjector/SpellHotbar.json", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/effectdata/vanilla_cast_effects.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/*.ttf", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/presets", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/transformdata", dev_mod_root),
]

released_files_nordic_ui_plugin = [
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/fonts/9_$ConsoleFont_Futura Condensed.ttf",
     dev_mod_root_nordic_ui),
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/*.csv", dev_mod_root_nordic_ui),
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/*.png", dev_mod_root_nordic_ui),
]

def get_spell_pack_list(modname: str):
    return [
        (dev_mod_root / f"Interface/SpellHotbar/{modname}_icons.swf", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.csv", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.png", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_{modname}.csv", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/InventoryInjector/{modname.capitalize()}.json", dev_mod_root),
    ]

released_files_triumvirate_spellpack = [
    (dev_mod_root / f"Interface/SpellHotbar/triumvirate_icons.swf", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/InventoryInjector/Triumvirate - Mage Archetypes.json", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_druid.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_druid.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_druid.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_cleric.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_cleric.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_cleric.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shadow_mage.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shadow_mage.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_shadow_mage.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_warlock.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_warlock.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_warlock.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shaman.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shaman.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_shaman.csv", dev_mod_root),
]



def build_release_zip(outfile: Path, files: list[tuple[Path, str | Path]], main_folder: str = "data"):
    print(f"Building Release zip: {outfile}...")
    main_path = Path(main_folder)
    with ZipFile(outfile, mode="w", compression=ZIP_STORED) as zfile:
        for entry in files:
            file_list = glob(str(entry[0]))
            for file_name in file_list:
                if isinstance(entry[1], Path):
                    arcname = str(main_path / Path(file_name).relative_to(entry[1]))
                else:
                    fname = Path(file_name).name
                    zip_path = Path(entry[1]) / fname
                    arcname = str(main_path / zip_path)
                print(f"Adding: {arcname}")
                zfile.write(str(file_name), arcname=arcname)
    print("Done\n")

def test_build_release_zip(outfile: Path, files: list[tuple[Path, str]], main_folder: str = "Spell Hotbar"):
    main_path = Path(main_folder)
    for entry in files:
        file_list = glob(str(entry[0]), recursive=True)
        for file_name in file_list:
            if isinstance(entry[1], Path):
                arcname = str(main_path / Path(file_name).relative_to(entry[1]))
            else:
                fname = Path(file_name).name
                zip_path = Path(entry[1]) / fname
                arcname = str(main_path / zip_path)
            print(arcname)


if __name__ == "__main__":
    version = "0.1"

    release_zip_path = project_root / f"build/Spell Hotbar_{version}.zip"
    #build_release_zip(release_zip_path, released_files_main_plugin)

    #build_release_zip(project_root / f"build/Spell Hotbar Nordic UI_1.0.zip", released_files_nordic_ui_plugin)

    #for modname in ["vulcano", "arclight", "desecration"]:
    #    build_release_zip(project_root / f"build/Spell Hotbar - {modname.capitalize()}_1.0.zip", get_spell_pack_list(modname))

    #build_release_zip(project_root / f"build/Spell Hotbar - Triumvirate_1.0.zip", released_files_triumvirate_spellpack)