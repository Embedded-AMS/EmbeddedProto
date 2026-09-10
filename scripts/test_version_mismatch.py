#!/usr/bin/env python3

"""
Test script to verify version mismatch detection in Embedded Proto.
This script tests three different version mismatch scenarios:
1. Major version mismatch - Should cause compilation error
2. Minor version mismatch - Should cause warning
3. Patch version mismatch - Should build successfully
"""

import os
import shutil
import subprocess
import sys
import re

class VersionTest:
    def __init__(self):
        self.script_dir = os.path.dirname(os.path.abspath(__file__))
        self.root_dir = os.path.dirname(self.script_dir)
        self.version_h_path = os.path.join(self.root_dir, 'src', 'Version.h')
        self.backup_path = self.version_h_path + '.bak'
        self.build_script = os.path.join(self.root_dir, 'build_test.sh')

    def backup_version_h(self):
        """Create backup of Version.h"""
        print("\nBacking up Version.h...")
        shutil.copy2(self.version_h_path, self.backup_path)

    def restore_version_h(self):
        """Restore Version.h from backup and cleanup"""
        print("\nRestoring original Version.h...")
        if os.path.exists(self.backup_path):
            shutil.copy2(self.backup_path, self.version_h_path)
            os.remove(self.backup_path)
            print("Original Version.h restored successfully")

    def get_current_versions(self):
        """Get current version numbers from Version.h"""
        with open(self.version_h_path, 'r') as f:
            content = f.read()
            
        self.original_major = re.search(r'#define EMBEDDEDPROTO_VERSION_MAJOR (\d+)', content).group(1)
        self.original_minor = re.search(r'#define EMBEDDEDPROTO_VERSION_MINOR (\d+)', content).group(1)
        self.original_patch = re.search(r'#define EMBEDDEDPROTO_VERSION_PATCH (\d+)', content).group(1)
        
        print("Original version: " + self.original_major + "." + self.original_minor + "." + self.original_patch)

        return int(self.original_major), int(self.original_minor), int(self.original_patch)

    def modify_major_version(self, new_major):
        """Modify only the major version number"""
        with open(self.version_h_path, 'r') as f:
            content = f.read()
            
        modified_content = re.sub(
            r'#define EMBEDDEDPROTO_VERSION_MAJOR \d+',
            f'#define EMBEDDEDPROTO_VERSION_MAJOR {new_major}',
            content
        )
        
        with open(self.version_h_path, 'w') as f:
            f.write(modified_content)

    def modify_minor_version(self, new_minor):
        """Modify only the minor version number"""
        with open(self.version_h_path, 'r') as f:
            content = f.read()
            
        modified_content = re.sub(
            r'#define EMBEDDEDPROTO_VERSION_MINOR \d+',
            f'#define EMBEDDEDPROTO_VERSION_MINOR {new_minor}',
            content
        )
        
        with open(self.version_h_path, 'w') as f:
            f.write(modified_content)

    def modify_patch_version(self, new_patch):
        """Modify only the patch version number"""
        with open(self.version_h_path, 'r') as f:
            content = f.read()
            
        modified_content = re.sub(
            r'#define EMBEDDEDPROTO_VERSION_PATCH \d+',
            f'#define EMBEDDEDPROTO_VERSION_PATCH {new_patch}',
            content
        )
        
        with open(self.version_h_path, 'w') as f:
            f.write(modified_content)

    def run_build(self):
        """Run build_test.sh and return result and output"""
        try:
            result = subprocess.run(
                [self.build_script], 
                check=True,
                capture_output=True,
                text=True
            )
            return True, result.stdout + result.stderr
        except subprocess.CalledProcessError as e:
            return False, e.stdout + e.stderr

    def test_major_version_mismatch(self):
        """Test major version mismatch (should fail compilation)"""

        dummy_version = 999

        print("\n=== Testing Major Version Mismatch ===")
        print("Changing major version from " + self.original_major + " to " + str(dummy_version) + " (should fail with static assertion)")
        
        try:
            self.backup_version_h()
            self.modify_major_version(dummy_version)
            success, output = self.run_build()
            
            if success:
                print("❌ Test FAILED: Build succeeded when it should have failed")
                return False
            elif "static assertion failed" in output.lower():
                print("✅ Test PASSED: Build failed with static assertion as expected")
                return True
            else:
                print("❌ Test FAILED: Build failed but not due to static assertion")
                return False
                
        finally:
            self.restore_version_h()

    def test_minor_version_mismatch(self):
        """Test minor version mismatch (should show warning)"""
        
        dummy_version = 999

        print("\n=== Testing Minor Version Mismatch ===")
        print("Changing minor version from " + self.original_minor + " to " + str(dummy_version) + " (should build with warning)")
        
        try:
            self.backup_version_h()
            self.modify_minor_version(dummy_version)
            success, output = self.run_build()
            
            if not success:
                print("❌ Test FAILED: Build failed when it should have succeeded with warning")
                return False
            elif "warning" in output.lower() and "version mismatch" in output.lower():
                print("✅ Test PASSED: Build succeeded with version mismatch warning")
                return True
            else:
                print("❌ Test FAILED: Build succeeded but without version mismatch warning")
                return False
                
        finally:
            self.restore_version_h()

    def test_patch_version_mismatch(self):
        """Test patch version mismatch (should build successfully)"""

        dummy_version = 999

        print("\n=== Testing Patch Version Mismatch ===")
        print("Changing patch version from" + self.original_patch + " to " + str(dummy_version) + " (should build successfully)")
        
        try:
            self.backup_version_h()
            self.modify_patch_version(dummy_version)
            success, output = self.run_build()
            
            if success and "version mismatch" not in output.lower():
                print("✅ Test PASSED: Build succeeded without warnings")
                return True
            else:
                print("❌ Test FAILED: Build failed or showed unexpected warnings")
                return False
                
        finally:
            self.restore_version_h()

def main():
    test = VersionTest()
    
    test.get_current_versions()

    # Run all tests
    results = []
    results.append(("Major Version Mismatch", test.test_major_version_mismatch()))
    results.append(("Minor Version Mismatch", test.test_minor_version_mismatch()))
    results.append(("Patch Version Mismatch", test.test_patch_version_mismatch()))
    
    # Print summary
    print("\n=== Test Summary ===")
    all_passed = True
    for name, passed in results:
        status = "✅ PASSED" if passed else "❌ FAILED"
        print(f"{name}: {status}")
        all_passed = all_passed and passed
    
    sys.exit(0 if all_passed else 1)

if __name__ == '__main__':
    main()
