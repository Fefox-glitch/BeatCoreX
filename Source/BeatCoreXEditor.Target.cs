using UnrealBuildTool;
using System.Collections.Generic;

public class BeatCoreXEditorTarget : TargetRules
{
    public BeatCoreXEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("BeatCoreX");
    }
}
