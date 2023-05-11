import os
import subprocess
import sys
import shutil
import filecmp

from pathlib import Path

SHADERS_RELATIVE_PATH = 'shaders'

VULKAN_GLSLC = "Bin\glslangValidator.exe"

# Main Function
try:
    vulkan = os.getenv("VULKAN_SDK");

    if(not os.path.exists(vulkan)):
        raise Exception("Unable to find Vulkan directory")

    compiler = os.path.join(vulkan, VULKAN_GLSLC)

    if(not os.path.exists(compiler)):
        raise Exception("Unable to find glslangValidator")

    print("Using Vulkan GLSL compiler at: " + compiler)

    root_dir = os.path.abspath(os.path.join(sys.argv[0], os.pardir, os.pardir))

    shaders_dir = os.path.join(root_dir, SHADERS_RELATIVE_PATH);

    if(not os.path.exists(shaders_dir)):
        raise Exception("Unable to find shaders directory " + shaders_dir)
    
    for filename in os.scandir(shaders_dir):
        if(filename.is_file() and not filename.name.endswith(".spv")):
            name = filename.path
            cmd = [compiler, "-V", name, "-o", os.path.join(shaders_dir, name + ".spv")]
            subprocess.run(cmd, shell=True)



except Exception as error:
    print('Caught error: ' + repr(error))