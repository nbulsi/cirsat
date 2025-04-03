#!/usr/bin/env python3

"""
File Generator for cirsat Project

This script automatically generates C++ source/header files with copyright information
and updates CMakeLists.txt accordingly.

Usage:
    python3 /tools/create_file.py <filename> <brief description>

Examples:
    # Create a header file
    python3 /tools/create_file.py include/new_module.hpp "New module description"
    
    # Create a source file
    python3 /tools/create_file.py src/new_module.cpp "Implementation of new module"
"""

import os
import sys
from datetime import datetime
import re

TEMPLATE_HPP = '''/**
 * @file {filename}
 * @brief {brief}
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef {guard}
#define {guard}

namespace cirsat {{

{class_content}

}} // namespace cirsat

#endif // {guard}
'''

TEMPLATE_CPP = '''/**
 * @file {filename}
 * @brief {brief}
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#include "{header}"

namespace cirsat {{

{content}

}} // namespace cirsat
'''

def update_cmake(filename):
    """
    Update CMakeLists.txt in the corresponding directory.
    Args:
        filename: Path to the new source file
    """
    dir_path = os.path.dirname(filename)
    cmake_path = os.path.join(dir_path, 'CMakeLists.txt')
    
    # Skip if it's a header file
    if filename.endswith('.hpp'):
        return
    
    # If CMakeLists.txt doesn't exist, we don't need to update it
    if not os.path.exists(cmake_path):
        return
    
    with open(cmake_path, 'r') as f:
        content = f.read()
    
    # Check if we're already using file(GLOB...)
    if 'file(GLOB' in content:
        print(f"CMakeLists.txt already uses GLOB, no update needed")
        return
    
    # Add the new source file to the add_library command
    base_name = os.path.basename(filename)
    if 'add_library' in content:
        new_content = re.sub(
            r'add_library\((.*?)\)',
            f'add_library(\\1 {base_name})',
            content,
            flags=re.DOTALL
        )
        
        with open(cmake_path, 'w') as f:
            f.write(new_content)
        print(f"Updated {cmake_path}")

def create_file(filename, brief):
    """
    Create a new C++ source or header file with template content.
    Args:
        filename: Path to the new file
        brief: Brief description of the file
    """
    # Create directory if it doesn't exist
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    base_name = os.path.basename(filename)
    is_header = filename.endswith('.hpp')
    
    if is_header:
        guard = f"CIRSAT_{os.path.splitext(base_name)[0].upper()}_HPP"
        content = TEMPLATE_HPP.format(
            filename=base_name,
            brief=brief,
            guard=guard,
            class_content="// Add your class/function declarations here"
        )
    else:
        header = os.path.splitext(base_name)[0] + '.hpp'
        content = TEMPLATE_CPP.format(
            filename=base_name,
            brief=brief,
            header=header,
            content="// Add your implementation here"
        )
    
    with open(filename, 'w') as f:
        f.write(content)
    print(f"Created {filename}")
    
    # Update CMakeLists.txt if necessary
    update_cmake(filename)

def main():
    """
    Main entry point of the script.
    Validates command line arguments and creates the requested file.
    """
    if len(sys.argv) < 3:
        print("Usage: python create_file.py <filename> <brief description>")
        print("\nExamples:")
        print("  python create_file.py include/new_module.hpp 'New module description'")
        print("  python create_file.py src/new_module.cpp 'Implementation of new module'")
        sys.exit(1)
    
    filename = sys.argv[1]
    brief = sys.argv[2]
    
    if not (filename.endswith('.cpp') or filename.endswith('.hpp')):
        print("Error: File must have .cpp or .hpp extension")
        sys.exit(1)
    
    create_file(filename, brief)

if __name__ == "__main__":
    main()
