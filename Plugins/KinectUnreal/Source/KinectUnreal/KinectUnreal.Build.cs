// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;
using UnrealBuildTool;

public class KinectUnreal : ModuleRules
{
     
    public string GetUProjectPath()
    {
        //Change this according to your module's relative location to your project file. If there is any better way to do this I'm interested!
        //Assuming Source/ThirdParty/YourLib/
        return Directory.GetParent(ModuleDirectory).Parent.FullName;
    }

    private void CopyToBinaries(string Filepath, ReadOnlyTargetRules Target)
    {
        string binariesDir = Path.Combine(GetUProjectPath(), "Binaries", Target.Platform.ToString());
        string filename = Path.GetFileName(Filepath);

        System.Console.WriteLine( "Writing file " + Filepath + " to " + binariesDir );

        if (!Directory.Exists(binariesDir))
            Directory.CreateDirectory(binariesDir);

        if (!File.Exists(Path.Combine(binariesDir, filename)))
            File.Copy(Filepath, Path.Combine(binariesDir, filename), true);
    }

    public KinectUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
	    //OptimizeCode = CodeOptimization.Never;

        string KinectPathEnvVar = "%KINECTSDK20_DIR%";
        string ExpandedKinectEnvVar = Environment.ExpandEnvironmentVariables(KinectPathEnvVar);

        //NOTE (OS): Safety check for comptuers that don't have the kinect plugin
        if (KinectPathEnvVar == ExpandedKinectEnvVar)
        {
            var err = "ERROR : Environment variable {0} does not exist in this Windows environment. Check if the Kinect for Windows 2.0 plugin is installed.";
            Console.WriteLine(err, KinectPathEnvVar);
            throw new Exception(err);
        }

        string ThirdPartyKinectIncludePath = Path.Combine(ExpandedKinectEnvVar, "inc");

        string PlatformSpec = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
        string ThirdPartyKinectLibPath = Path.Combine(ExpandedKinectEnvVar, "Lib", PlatformSpec);

        


        PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyKinectLibPath, "Kinect20.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyKinectLibPath, "Kinect20.Face.lib"));

	    string kinectFaceDll = Path.Combine(ExpandedKinectEnvVar, "bin", "Kinect20.Face.dll");

	    System.Console.WriteLine("Module Directory " + GetUProjectPath());
        
        PublicDelayLoadDLLs.Add(kinectFaceDll);
	    //RuntimeDependencies.Add(kinectFaceDll, StagedFileType.NonUFS);
        CopyToBinaries(kinectFaceDll, Target);

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"KinectUnreal/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"KinectUnreal/Private",
			    ThirdPartyKinectIncludePath
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "RHI",
                "RenderCore",
                "ShaderCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
