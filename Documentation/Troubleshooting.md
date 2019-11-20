Troubleshooting
===============

#### Errors when packaging

There is a known bug in UE4 where it requires all plugin Build.cs files to be present when packaging a game. This effectively means that in order to package your game you need full source to the FaceFX plugin installed.

If you are using the GitHub source, this is a non-issue. However, if you are using the Epic Launcher version of the engine, it means you need to get the full source to the FaceFX plugin, including the FaceFX Runtime library, and set it up in the Epic Launcher version of your engine. Place the source code in the same location you put the pre-built binaries of the FaceFX plugin (see the [README.md](README.md) file located in this repository).

#### Audio plays with no animation and there are no FaceFX errors in the log

Make sure the character's **Animation Blueprint** is referenced in the **Blueprint Class**, the **Blend FaceFX Animation** Blueprint node is added to the **Animation Blueprint**, and it is connected properly.

#### FaceFX Animations do not play from the Sequencer tab in OSX

On the Mac platform, there are known issues playing FaceFX animations from the Sequencer window.  Bone animations will work when scrubbing the time from the Sequencer tab, but morphs may not update during scrubbing.  Hitting play from the Sequencer tab will not update audio, bones or morph animations during playback.  Use the **Play in Editor** feature to see updates to both morph and bones animations.

#### All other issues

Make sure there are no FaceFX warnings or errors in the log (launch the Unreal Editor with the **-Log** option). Otherwise, let us know, so we can add the issue here!
