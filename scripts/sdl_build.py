import os
import subprocess
import sys
import shutil

VS_WHERE = os.path.join(os.getenv("ProgramFiles(x86)"),r"Microsoft Visual Studio\Installer\vswhere.exe")

SDL_RELATIVE_PATH = 'SDL\VisualC'

def Execute(cmd):
    popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True, shell=True)
    for stdout_line in iter(popen.stdout.readline, ""):
        yield stdout_line 
    popen.stdout.close()
    return_code = popen.wait()
    if return_code:
        raise subprocess.CalledProcessError(return_code, cmd)
        
def GetMSBuild():
    find_msbuild = subprocess.run([VS_WHERE, '-latest', '-prerelease','-products','*','-requires','Microsoft.Component.MSBuild','-find', 'MSBuild\**\Bin\MSBuild.exe'], shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='cp949')
    if find_msbuild.returncode == 0:
        return find_msbuild.stdout.strip()
    else:
        raise Exception("Unable to find MSBuild.exe")
        
def BuildSDL(MSBuild, SDL_solution, configuration):
    for output in Execute([MSBuild, SDL_solution, '-t:SDL2;SDL2Main', '-property:Configuration='+configuration+';Platform=x64', '-m']):
        print(output, end="")
        
def CopyWithExtension(in_path, out_path, extension):
    for filename in os.scandir(in_path):
        if (filename.is_file() and filename.name.endswith(extension)):
            destination = os.path.join(out_path, filename.name)
            print("Copying '" + filename.name + "' to: '" + destination + "'")
            shutil.copy2(filename.path, destination)
            
def MakeDir(path):
    if not os.path.isdir(path):
        os.mkdir(path)
      
try:
    if (not os.path.exists(VS_WHERE)):
        raise Exception("Unable to find vswhere.exe")        
    
    path = os.path.abspath(os.path.join(sys.argv[0], os.pardir, os.pardir))
    
    configuration = ""
    
    if(len(sys.argv) > 1):
        configuration = sys.argv[1]
        
    build_debug = configuration == "" or configuration == "Debug"
    build_release = configuration == "" or configuration == "Release" or configuration == "ReleaseWithTrace"
    
    if(build_debug == False and build_release == False):
        raise Exception("Invalid configuration argument, only Debug and Release are accepted")
    
    SDL_solution = os.path.join(path, SDL_RELATIVE_PATH, 'SDL.sln')
    
    if (not os.path.exists(SDL_solution)):
        raise Exception("Solution " + SDL_solution + " does not exists")
        
    MSBuild = GetMSBuild()
    
    if (build_debug):
        BuildSDL(MSBuild, SDL_solution, "Debug")        
        source_debug_input = os.path.join(path, SDL_RELATIVE_PATH, 'x64', 'Debug')
        executable_debug_output = os.path.join(path, 'Binaries', 'x64', 'Debug')
        lib_debug_output = os.path.join(path, 'Libs', 'Debug')
        
        MakeDir(executable_debug_output);
        MakeDir(lib_debug_output);
        
        CopyWithExtension(source_debug_input, executable_debug_output, '.dll')
        CopyWithExtension(source_debug_input, executable_debug_output, '.pdb')
        CopyWithExtension(source_debug_input, lib_debug_output, '.lib')
    
    if (build_release):
        source_release_input = os.path.join(path, SDL_RELATIVE_PATH, 'x64', 'Release')
        executable_release_output = os.path.join(path, 'Binaries', 'x64', 'Release')
        lib_release_output = os.path.join(path, 'Libs', 'Release')
        
        MakeDir(executable_release_output);
        MakeDir(lib_release_output);
        
        CopyWithExtension(source_release_input, executable_release_output, '.dll')
        CopyWithExtension(source_release_input, executable_release_output, '.pdb')
        CopyWithExtension(source_release_input, lib_release_output, '.lib')
        
except Exception as error:
    print('Caught error: ' + repr(error))