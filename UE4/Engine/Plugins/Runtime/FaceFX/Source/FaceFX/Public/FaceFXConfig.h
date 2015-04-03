/** 
* Enable this by setting to 1 to support the use of bone name filters for FaceFX characters. This filter defines what bones will be blended in
* The filter is defined within the SkeletalMesh assets and will be evaluated by the character instance.
* Search for the string "TODO_FACEFX_WITHBONEFILTER" to find the places that needs to get touched when changing this setting
*/
#define FACEFX_WITHBONEFILTER 0

//The number of total FaceFX channels. One channel per animation we want to be able to play at once per FaceFXCharacter
#define FACEFX_CHANNELS 1