# winui-drover-island

In order to build this, you need latest Visual Studio 2019 and 10.0.18362
Open `winui-drover-island.sln`, pick Debug or Release "x64", build and run the Package project.

Requirements:
Windows 10, version 1803 or newer
Latest Visual Studio 2019 (16.8.1) - it may work with older too, but I didn’t try.
When installing VS, you need to add .NET Desktop Development (this also installs .NET 5), Universal Windows Platform development, Desktop development with C++ and The C++ (v142) Universal Windows Platform tools optional component for the Universal Windows Platform workload (see “Installation Details” under the “Universal Windows Platform development” section, on the right pane)
Make sure your system has a NuGet package source enabled for nuget.org
The VSIX Package: https://marketplace.visualstudio.com/items?itemName=Microsoft-WinUI.WinUIProjectTemplates - this is useful to try create new projects with WinUI to see if they work (if mine does not work).

