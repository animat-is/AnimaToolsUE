# AnimaToolsUE
AnimaTools is UE plugin with custom tools for editor, focused on animation.

This repository is to support animators to extend their pipeline and be able to do more directly inside the engine.
There is no roadmap yet, but I hope to add more tools in the future.

# Notes

## Adjustment blend
For the script to run properly you should have IK bones in the world-like hierarchy. Default UE rigs always bake body_offset_ctrl movement to the ctrl. From my experience it's better when used only for offseting character (forward solve), but when baking animation it should stay zeroed, without affecting poses every frame. If it's zeroed all the IK will blend as expected with the adjustment blend.

# Special credits

Adjustment blend algorithm in this toolkit is just my UE implementation of a popular algorithm shared by Dan Lowe here https://github.com/Danlowlows/MobuCore
