- [x] Destruct all at the end
- [x] ~~Make ImGuIZMO.quat work~~ No longer required
- [ ] Finish modularization
- [ ] Make SOIL2 stop giving that smelly error message
    Wtf is this warning?
    CMake Warning (dev) at include/SOIL2/CMakeLists.txt:4 (option):
  Policy CMP0077 is not set: option() honors normal variables.  Run "cmake
  --help-policy CMP0077" for policy details.  Use the cmake_policy command to
  set the policy and suppress this warning.

  For compatibility with older versions of CMake, option is clearing the
  normal variable 'SOIL2_BUILD_TESTS'.
This warning is for project developers.  Use -Wno-dev to suppress it.

CMake Warning (dev) at include/SOIL2/CMakeLists.txt:67 (install):
  Target soil2 has PUBLIC_HEADER files but no PUBLIC_HEADER DESTINATION.
This warning is for project developers.  Use -Wno-dev to suppress it.
- [x] Overhaul asset debug screen
- [ ] Simple Lighting
- [ ] Lightmapped Lighting
- [ ] Get ziggy with it
- [x] Implement Cameras

- [ ] Physics
- [ ] Sound
- [ ] Mesh deformation/animation
