# Kenos

Kenos realtime rendering engine, this time in OpenGL.

## Dependencies
All packages are from NuGet, if you are contributing please do your best to keep it that way. Note: VS will often complain about missing packages/files installed from NuGet, just restart VS and it should go away.

Packages:
- GLM (OpenGL Mathematics)
- GLFW (OpenGL Framework)
- AssImp (Asset Importer)
- nlohmann.json (JSON for Modern C++)

## Building

This project expects you to use Visual Studio 2022, and uses the VS build system. If you cannot use Visual Studio too bad.

## Contributing

If you wish to contribute to this project create a pull request with your changes. 

### Some miscellaneous notes:

When creating a new file remember to put it into the `kenos\src` directory. This can be done easily by adding `src/` to the start of the filename when creating it with the VS add new item dialog.

If you add a wavefront .obj file to the project, make sure to go to it's properties and exclude it from the build. The compiler will interpret is an assembly object file and throw an error.