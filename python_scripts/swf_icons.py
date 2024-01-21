import subprocess

executable_path = r"F:\Skyrim Dev\TOOLS\SWFTools\png2swf.exe"

output = r"F:\Skyrim Dev\Papyrus_Include\test\test.swf"

images = [
    r"F:\Skyrim Dev\Python_projects\SpellHotbar\icons\school_restoration.png",
    r"F:\Skyrim Dev\Python_projects\SpellHotbar\icons\school_alteration.png",
    r"F:\Skyrim Dev\Python_projects\SpellHotbar\icons\school_conjuration.png",
    r"F:\Skyrim Dev\Python_projects\SpellHotbar\icons\school_destruction.png",
    r"F:\Skyrim Dev\Python_projects\SpellHotbar\icons\school_illusion.png"
]

proc = subprocess.run([executable_path, "-o", f"{output}", "-X", "32", "-Y", "32"] + images)
print(proc)