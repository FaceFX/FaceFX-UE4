Context Menus
=============

The FaceFX UE4 Plugin adds several new options to the Unreal Editor asset right-click context menus in the Unreal Editor.

FaceFXActor Asset
-----------------

![](Images/FaceFXActorContextMenu.png =205x154)

##### Open in FaceFX Studio

Opens the **.facefx** file in FaceFX Studio (if the path to your FaceFX Studio installation is correct in the **FaceFX.ini** file).

**Note:** Only available on Windows.

##### Open FaceFX Studio Asset Folder

Opens the folder that contains your **.facefx** file.

##### Reimport FaceFX Studio Assets

Batch imports the actor and any modified animation via the **.ffxc** folder that is parallel to the **.facefx** file and links the animations to the actor.

##### Set new FaceFX Studio Assets

Assigns this actor to a different **.facefx** file. Use this to update the source data for the UE4 asset while still preserving all of the Blueprint asset references in UE4.

##### Show FaceFX Asset Details

Lists the animations linked to this actor, the bone names in the actor, and the size of the actor.

##### Link FaceFX Animset

Links animations to the actor.

##### Unlink FaceFX Animset

Unlinks animations from the actor.

FaceFXAnim Asset
----------------

![](Images/FaceFXAnimationContextMenu.png =206x150)

##### Open FaceFX Studio Asset Folder

Opens the folder that contains your **.facefx** file.

##### Reimport FaceFX Studio Assets

Reimports the selected animation asset only. Does not link or unlink the animation.

##### Set new FaceFX Studio Assets

Assign the animation to a different **.facefx** file.

##### Show FaceFX Asset Details

Displays the source FaceFX filename, the animation group, and the animation name.

##### Link with FaceFX Actor

Links the animation with an actor.

##### Unlink From FaceFX Actor

Unlinks the animation from an actor.
