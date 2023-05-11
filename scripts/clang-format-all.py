import os
import sys
import subprocess

root_dir = os.path.abspath(os.path.join(sys.argv[0], os.pardir, os.pardir))

source_dir  = os.path.join(root_dir, "Source")
public_dir = os.path.join(source_dir, "Public")
private_dir = os.path.join(source_dir, "Private")

def clang_format(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.c') or file.endswith('.cpp') or file.endswith('.h') or file.endswith('.hpp'):
                file_path = os.path.join(root, file)
                print(f'Formatting {file_path}')
                try:
                    subprocess.run(['clang-format', '-i', '-style=file', file_path], check=True)
                except subprocess.CalledProcessError as e:
                    print(f'Error formatting {file_path}: {str(e)}')

if __name__ == '__main__':
    clang_format(public_dir)
    clang_format(private_dir)

