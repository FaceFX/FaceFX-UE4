FaceFX Unreal Engine 4 Plugin
=============================

Welcome to the FaceFX Unreal Engine 4 plugin source code!

This repository gives you all the information you need to successfully compile and install the FaceFX UE4 plugin.

The FaceFX UE4 plugin currently supports the Windows, Mac, Xbox One, PS4, Nintendo Switch, iOS, and Android platforms.

Important Information
---------------------

With the UE4 version 4.24 update, the FaceFX UE4 Plugin has been updated to the FaceFX Runtime v1.5. It is important that you use the FaceFX Runtime v1.5 (or newer) data compiler and FaceFX Studio plugin. If you are upgrading from a previous version of the FaceFX UE4 Plugin, please make sure that you fully update your tool chain to the FaceFX Runtime v1.5 (or newer) as well.

License
-------

Complete licensing information can be found in the [LICENSE.md](LICENSE.md) file located in this repository. The short version is:

- The FaceFX UE4 plugin source code is licensed under the **MIT license** (this is the code that interfaces with UE4; in other words, the code contained in this repository).

- The FaceFX Runtime is licensed under the **FaceFX Runtime End User License**. Once the FaceFX UE4 plugin source code is compiled and linked with the FaceFX Runtime, the resulting binary falls under the **FaceFX Runtime End User License**.

Supported Unreal Engine 4 versions
----------------------------------

The FaceFX UE4 plugin supports UE4 version 4.24. It will not work unmodified on earlier versions of UE4.

Documentation
-------------

The [FaceFX UE4 Plugin documentation](Documentation/Index.md) is located in the **Documentation** directory of this GitHub repository. Be sure to check out the [Troubleshooting](Documentation/Troubleshooting.md) section if you run into any problems.


Installation
------------

First, make sure that you have obtained a binary build of UE4; or, have obtained, and successfully built, the source code distribution of UE4. More information can be found on [the Unreal Engine website](https://www.unrealengine.com).

There are two ways to obtain the FaceFX UE4 plugin. You can download a pre-built binary version and simply drop it into your UE4 install, or you can build the plugin from source. The quickest way to get up and running is simply to download and install the pre-built version.

The following steps describe how to install the FaceFX UE4 plugin:

#### Pre-built binaries

**Note**: The pre-built binaries distribution will only work with the version of UE4 that is installed from inside the Epic Games Launcher application (currently 4.24). If you are using the UE4 GitHub source code you need to follow the directions for building the plugin from source.

##### Windows

1. [Download](https://unreal.facefx.com) the pre-built binaries distribution.

2. Unzip the pre-built binaries distribution into your **C:\Program Files\Epic Games\UE_4.24\Engine\Plugins\Runtime** directory. You should now have this directory: **C:\Program Files\Epic Games\UE_4.24\Engine\Plugins\Runtime\FaceFX**.

3. Run UE4 from the Epic Games Launcher.

##### Mac

1. [Download](https://unreal.facefx.com) the pre-built binaries distribution.

2. Unzip the pre-built binaries distribution into your **/Users/Shared/Epic Games/UE_4.24/Engine/Plugins/Runtime** directory. You should now have this directory: **/Users/Shared/Epic Games/UE_4.24/Engine/Plugins/Runtime/FaceFX**.

3. Run UE4 from the Epic Games Launcher.


#### Source

First, make sure you are familiar with the process of cloning Unreal Engine from GitHub and have performed the necessary UE4 build steps for your target platform. The following instructions assume you know how to successfully build the GitHub version of Unreal Engine 4.

##### Windows

1. [Fork and clone this repository](https://guides.github.com/activities/forking/).

2. In order to build the source code you need to [download](https://unreal.facefx.com) the FaceFX Runtime distribution.

3. Create a directory named **FaceFX** in your **UnrealEngine/Engine/Plugins/Runtime** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX**.

4. Copy the contents of your cloned repository from step 1 into the newly created **FaceFX** directory.

5. Unzip the FaceFX Runtime distribution .zip file you downloaded in step 2.

6. Inside the extracted folder from step 5 you should find a **facefx-runtime-1.5.1** directory.

7. Copy the **facefx-runtime-1.5.1** directory into your **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib/facefx-runtime-1.5.1**.

8. Run the **GenerateProjectFiles.bat** file located in your **UnrealEngine** directory.

9. Load the UE4 solution in Visual Studio. Set your solution configuration to **Development Editor** and your solution platform to **Win64**, then right click on the **UE4** target and select **Build**.

10. Run UnrealEd according to Epic's instructions.

##### Mac

1. [Fork and clone this repository](https://guides.github.com/activities/forking/).

2. In order to build the source code you need to [download](https://unreal.facefx.com) the FaceFX Runtime distribution.

3. Create a directory named **FaceFX** in your **UnrealEngine/Engine/Plugins/Runtime** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX**.

4. Copy the contents of your cloned repository from step 1 into the newly created **FaceFX** directory.

5. Unzip the FaceFX Runtime distribution .zip file you downloaded in step 2.

6. Inside the extracted folder from step 5 you should find a **facefx-runtime-1.5.1** directory.

7. Copy the **facefx-runtime-1.5.1** directory into your **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib/facefx-runtime-1.5.1**.

8. Run the **GenerateProjectFiles.sh** or double-click the **GenerateProjectFiles.command** file located in your **UnrealEngine** directory.

9. Load the UE4 project in Xcode. Select the **UE4Editor - Mac** for **My Mac** target in the title bar, then select the 'Product > Build' menu item.

10. Run UnrealEd according to Epic's instructions.

Contributing
------------

If you would like to contribute to the development of the FaceFX UE4 plugin, we accept contributions through [pull requests](https://help.github.com/articles/using-pull-requests/) on GitHub. Pull requests should be based on the **master** branch and should be associated with a GitHub [issue](https://help.github.com/articles/about-issues/). We use GitHub issues to track bugs, suggestions, questions, and feature requests.

All contributions must be under the **MIT license**, just like the FaceFX UE4 plugin source code itself.
