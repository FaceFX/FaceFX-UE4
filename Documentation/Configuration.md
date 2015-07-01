Configuration
=============

The FaceFX UE4 Plugin can be configured with several options contained in the **Engine/Plugins/Runtime/FaceFX/Config/FaceFX.ini** file.

FaceFX.ini
----------

##### StudioPathAbsolute

The absolute path to the FaceFX Studio installation.

##### IsImportAudio

Indicates if the audio data (.wav files only) should be automatically imported during the FaceFX Animation import process.

##### IsImportLookupAudio

Indicates if the import should search through all existing **USoundWave** assets and look for an asset that was generated with the linked sound source file (per FaceFX Animation). If such one is found it will be used instead of creating a new **USoundWave** asset for the .wav file.

**Note:** This might affect performance heavily when there are a lot of **USoundWave** assets.

##### IsImportAnimationOnActorImport

Indicates if animations should be imported during FaceFX actor import. If set to false only the FaceFX Actor asset will be imported or updated.

##### IsImportLookupAnimation

Indicates if the import should search through all existing **UFaceFXAnimation** assets and look for an asset that was generated with the linked **.ffxanim** source file. If such one is found it will be used instead of creating a new **UFaceFXAnimation** asset for the **.ffxanim** file.

**Note:** This might affect performance heavily when there are a lot of **UFaceFXAnimation** assets.

##### ShowToasterMessageOnIncompatibleAnim

Indicates if the editor should show a warning toaster message when a **UFaceFXAnimation** is attempted to be played on a **UFaceFXCharacter** with an incompatible FaceFX actor handle.
