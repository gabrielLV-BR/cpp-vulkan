# Made by Enzo Zardo -> https://github.com/EnzoZardo
# For now, GLSLC must be in path (will be fixed soon)
Get-ChildItem ".\src\resources\shaders\*.*" -Exclude "*.spv"  | ForEach-Object { glslc $_.FullName -o ($_.FullName + ".spv")}