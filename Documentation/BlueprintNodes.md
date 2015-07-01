Blueprint Nodes
===============

The FaceFX UE4 Plugin provides serveral Blueprint nodes, which are described here.

All FaceFX Blueprint nodes require a **FaceFX Component** to be wired to their respective **Target** slots. The FaceFX Setup Blueprint node requires the **Skel Mesh Comp** slot to be wired with the **Skeletal Mesh Component** to animate. The **Skel Mesh Comp** is optional for the other FaceFX Blueprint nodes (this is so a single Blueprint can drive multiple FaceFX characters). If it is not supplied, the first one found is used.

Setup
-----

<img src="Images/FaceFXSetupBlueprintNode.png" width="508">

The FaceFX Setup Blueprint node intitializes the linked **FaceFX Component** with the linked **Skeletal Mesh Component** and specified **FaceFXActor** asset. It must be called in the **Construction Script** before any other FaceFX Blueprint node.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Skel Mesh Comp** slot is required and should be wired to the **Sekeltal Mesh Component** to animate.

+ The **Asset** property is required and should be set to the **FaceFXActor** asset to be used for animation.

+ The **Is Auto Play Sound** property is optional. When **checked**, the audio associated with the **FaceFXAnim** asset being played will automatically be started at the appropriate time.

Play
----

<img src="Images/FaceFXPlayBlueprintNode.png" width="508">

The FaceFX Play Blueprint node plays the linked **FaceFXAnim** asset.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Animation** property is required and should be set to the **FaceFXAnim** asset to be played.

+ The **Skel Mesh Comp** slot is optional. It should be wired to the **Skeletal Mesh Component** to animate. If it is not set, the first **Skeletal Mesh Component** found is used.

+ The **Loop** property is optional. When set it causes the **FaceFXAnim** asset being played to loop continuously until manually stopped.

Play by Id
----------

<img src="Images/FaceFXPlayByIdBlueprintNode.png" width="508">

The FaceFX Play by Id Blueprint node plays a **FaceFXAnim** asset using a string ID created in the form **Group/AnimName**. The **FaceFXAnim** asset *must* be linked to the **FaceFXActor** asset in order for it to be found and played.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Group** property is optional. If set, it specifies the FaceFX animation group that contains the animation to be played. If not set, the first animation linked to the **FaceFXActor** asset found with the same name specified by the **Anim Name** property is played.

+ The **Anim Name** property is required. It specifies the name of the FaceFX animation to play.

+ The **Skel Mesh Comp** slot is optional. It should be wired to the **Skeletal Mesh Component** to animate. If it is not set, the first **Skeletal Mesh Component** found is used.

+ The **Loop** property is optional. When set it causes the **FaceFXAnim** asset being played to loop continuously until manually stopped.

Pause
-----

<img src="Images/FaceFXPauseBlueprintNode.png" width="508">

The FaceFX Pause Blueprint node pauses all FaceFX animations and audio files playing on an actor.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Skel Mesh Comp** slot is optional. It should be wired to the **Skeletal Mesh Component** to animate. If it is not set, the first **Skeletal Mesh Component** found is used.

Resume
------

<img src="Images/FaceFXResumeBlueprintNode.png" width="508">

The FaceFX Resume Blueprint node resumes all paused FaceFX animations and audio files.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Skel Mesh Comp** slot is optional. It should be wired to the **Skeletal Mesh Component** to animate. If it is not set, the first **Skeletal Mesh Component** found is used.

Stop
----

<img src="Images/FaceFXStopBlueprintNode.png" width="508">

The FaceFX Stop Blueprint node stops all FaceFX animations and audio files playing on an actor.

+ The **Target** slot is required and should be wired to the **FaceFX Component**.

+ The **Skel Mesh Comp** slot is optional. It should be wired to the **Skeletal Mesh Component** to animate. If it is not set, the first **Skeletal Mesh Component** found is used.
