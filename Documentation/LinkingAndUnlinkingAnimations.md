Linking and Unlinking Animations
================================

When you import a FaceFX asset by dragging a **.facefx** file onto UE4, the animations created are linked to the actor. This does the following:

+ When the actor is loaded into memory, all of the animations (and associated audio files) are loaded into memory as well.

+ The FaceFX **Play by ID** Blueprint node, which takes string parameters for the animation group and animation name, can be used to play any linked animation. This is convenient because the string can be dynamically generated from code.

For games with lots of audio files, some effort needs to be taken to manage memory appropriately since all audio files can't be loaded at once. This can be done in several ways:

+ **Creating different actors for each level (preferred method)** - Only the animations required for a particular level are loaded into memory. Animations remain linked to their actors, but there are fewer animations. This is a good approach because the FaceFX **Play by ID** Blueprint node can still be used, and keeping your animations and actors in sync is easy. Be sure to name each **.facefx** file uniquely (**Slade-level1.facefx** and **Slade-level2.facefx** are preferred over **Level1\Slade.facefx** and **Level2\Slade.facefx**). This avoids the following issues:

	1. The FaceFX Runtime plugin for FaceFX Studio uses a single compilation directory organized by **.facefx** name. Using multiple assets with the same name breaks the minimal rebuild feature.

	2. The following warning may be displayed in UE4 when multiple **.facefx** assets share the same filename: *Skipped **.ffxanim** file with no audiomap entry.*

+ **Unlinking animations from the actor (advanced)** - Using the right-click menu, animations can be unlinked from the actor. They can still be played on the actor (provided that they are compatible) by linking to them in a FaceFX **Play** Blueprint node. Beware that unlinked animations will become stale if you update your actor's **Face Graph** and never reimport the animation. Use this method if you have a lot of animations, but you lack discreet levels that you can break your content into. Memory will be managed by the Blueprint Classes that reference the FaceFX animations in FaceFX **Play** Blueprint nodes.
