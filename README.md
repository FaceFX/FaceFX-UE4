The FaceFX Unreal Engine 4 plugin
=================================

TODO: intro

License
-------

Supported Unreal Engine 4 versions
----------------------------------

The FaceFX UE4 plugin currently supports UE4 version 4.8.0 and up. It will not
work unmodified on earlier versions of UE4.

Installation
------------

The following steps describe how to install the FaceFX UE4 plugin.

TODO: describe what the pre-built binaries version is and where to obtain it.
TODO: describe what the source code is, its dependency (FaceFX Runtime), where
      to obtain it, and the license.

#### Pre-built binaries

1. Download the pre-built binaries distribution from TODO

2. Create a directory named FaceFX in your `UnrealEngine/Engine/Plugins/Runtime`
   directory. You should now have this directory:
   `UnrealEngine/Engine/Plugins/Runtime/FaceFX`

3. Unzip the contents of the binary distribution's .zip file into the newly
   created FaceFX directory.

4. Run Unreal

#### Source

1. Clone this git repository onto your computer.

2. Download the FaceFX Runtime distribution from TODO

3. Create a directory named FaceFX in your `UnrealEngine/Engine/Plugins/Runtime`
   directory. You should now have this directory:
   `UnrealEngine/Engine/Plugins/Runtime/FaceFX`

4. Copy the contents of this git repository into the newly created FaceFX
   directory.

5. Create a directory named facefx-1.0.0 in your
   `UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib` directory. You
   should now have this directory:
   `UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib/facefx-1.0.0`

6. Unzip the contents of the FaceFX Runtime distribution .zip file ino the newly
   created facefx-1.0.0 directory.

7. Make sure the UE4 solution is **not** loaded in Visual Studio.

8. Run the GenerateProjectFiles.bat files located in your `UnrealEngine`
   directory.

9. Load the UE4 solution in Visual Studio and do Build -> Clean Solution
   followed by Build -> Rebuild Solution.

Usage
-----
