Troubleshooting
===============

#### Errors when packaging

There is a known bug in UE4 where it requires all plugin Build.cs files to be present when packaging a game. This effectively means that in order to package your game you need full source to the FaceFX plugin installed.

If you are using the GitHub source, this is a non-issue. However, if you are using the Epic Launcher version of the engine, it means you need to get the full source to the FaceFX plugin, including the FaceFX Runtime library, and set it up in the Epic Launcher version of your engine. Place the source code in the same location you put the pre-built binaries of the FaceFX plugin (see the [README.md](README.md) file located in this repository).

#### Audio plays with no animation and there are no FaceFX errors in the log

Make sure the character's **Animation Blueprint** is referenced in the **Blueprint Class**, the **Blend FaceFX Animation** Blueprint node is added to the **Animation Blueprint**, and it is connected properly.

#### FaceFX Animations do not play from the Matinee tab

If the animations are played correctly when you use PIE, but not when you click play from the Matinee tab, check the following:

+ **Compile Blueprints** Make sure your Blueprint and Animation Blueprint have been compiled.  In some cases, you must explicitly compile these blueprints with the Compile button before they will function. 

+ **Morph bug** Due to a [potential issue with UE4](https://answers.unrealengine.com/questions/290685/morph-targets-not-updating-from-matinee-tab.html), morphs may not update correctly when played from the Matinee tab.

#### All other issues

Make sure there are no FaceFX warnings or errors in the log (launch the Unreal Editor with the **-Log** option). Otherwise, let us know, so we can add the issue here!
