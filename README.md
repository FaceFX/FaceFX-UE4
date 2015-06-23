FaceFX Unreal Engine 4 Plugin
=============================

Welcome to the FaceFX Unreal Engine 4 plugin source code!

This repository gives you all the information you need to successfully compile and install the FaceFX UE4 plugin.

Documentation and tutorials can be found on the plugin's [official website](http://unreal.facefx.com).

License
-------

Complete licensing information can be found in the **LICENSE.md** file located in this repository. The short version is:

- The FaceFX UE4 plugin source code is licensed under the **MIT license** (this is the code that interfaces with UE4; in other words, the code contained in this repository).

- The FaceFX Runtime is licensed under the **FaceFX Runtime End User License**. Once the FaceFX UE4 plugin source code is compiled and linked with the FaceFX Runtime, the resulting binary falls under the **FaceFX Runtime End User License**.

Supported Unreal Engine 4 versions
----------------------------------

The FaceFX UE4 plugin currently supports UE4 version 4.8.0 and up. It will not work unmodified on earlier versions of UE4.

Installation
------------

First, make sure that you have obtained a binary build of UE4; or, have obtained, and successfully built, the source code distribution of UE4. More information can be found on [the Unreal Engine website](https://www.unrealengine.com).

There are two ways to obtain the FaceFX UE4 plugin. You can download a pre-built binary version and simply drop it into your UE4 install, or you can build the plugin from source. The quickest way to get up and running is simply to download and install the pre-built version.

The following steps describe how to install the FaceFX UE4 plugin:

#### Pre-built binaries

**Note**: The pre-built binaries distribution will only work with the version of UE4 4.8 that is installed from inside the Epic Games Launcher application. If you are using the UE4 GitHub source code you need to follow the directions for building the plugin from source.

1. [Download](http://unreal.facefx.com) the pre-built binaries distribution.

2. Unzip the pre-built binaries distribution into your **C:\Program Files\Unreal Engine\4.8\Engine\Plugins\Runtime** directory. You should now have this directory: **C:\Program Files\Unreal Engine\4.8\Engine\Plugins\Runtime\FaceFX**.

3. Run UE4 from the Epic Games Launcher.

#### Source

1. [Fork and clone this repository](https://guides.github.com/activities/forking/).

2. In order to build the source code you need to [download](http://www.facefx.com) the FaceFX Runtime distribution.

3. Create a directory named **FaceFX** in your **UnrealEngine/Engine/Plugins/Runtime** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX**.

4. Copy the contents of your cloned repository from step 1 into the newly created **FaceFX** directory.

5. Unzip the FaceFX Runtime distribution .zip file you downloaded in step 2.

6. Inside the extracted folder from step 5 you should find a **facefx-runtime-1.0.0** directory.

7. Copy the **facefx-runtime-1.0.0** directory into your **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib** directory. You should now have this directory: **UnrealEngine/Engine/Plugins/Runtime/FaceFX/Source/FaceFXLib/facefx-1.0.0**.

8. Run the **GenerateProjectFiles.bat** files located in your **UnrealEngine** directory.

9. Load the UE4 solution in Visual Studio and do Build -> Clean Solution followed by Build -> Rebuild Solution.

Contributing
------------

If you would like to contribute to the development of the FaceFX UE4 plugin, we accept contributions through [pull requests](https://help.github.com/articles/using-pull-requests/) on GitHub. Pull requests should be based on the **master** branch and should be associated with a GitHub [issue](https://help.github.com/articles/about-issues/). We use GitHub issues to track bugs, suggestions, questions, and feature requests.

All contributions must be under the **MIT license**, just like the FaceFX UE4 plugin source code itself.
