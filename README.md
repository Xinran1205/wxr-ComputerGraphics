# Computer Graphics Project

This project is an experimental application showcasing the rendering of a Cornell Box using computer graphics techniques. We have employed a variety of rendering methods, such as rasterization and ray tracing, to achieve different visual effects. Moreover, the project supports a diverse range of materials and objects, along with flexible camera movement controls, providing users with a comprehensive platform to explore the principles and technologies of computer graphics.

## Getting Started
This document provides instructions on how to compile and run the project, details on interacting with the application via keyboard inputs to explore different computer graphics modes, and information on camera controls.

### Compiling the Project

The project uses a Makefile located in the `build` directory, utilizing relative paths for header files and material loading. Follow these steps to compile:

1. Navigate to the build directory:
`cd build`
2. Compile the project with Make:
`make`
3. Execute the compiled program:
`./RedNoise`

**Note:** If the `build` directory does not contain a Makefile, create one by navigating into the `build` directory, then use `cmake ..` to generate the Makefile.

### Default Mode

Upon running the project, the default graphics mode is activated with keypress `8`.

## Graphics Modes

Activate different graphics modes by pressing the corresponding keys:

- `1`: Random Triangle
- `2`: Draw Filled Triangle
- `3`: Draw Texture Triangle
- `4`: Wireframe 3D Scene Rendering
- `5`: Rasterizing
- `6`: Ray Tracing + Reflection
- `7`: Ray Tracing + Refraction
- `8`: Ray Tracing + Reflection + Refraction
- `9`: Ray Tracing with Flat, Gouraud, or Phong Shading for Spheres
- `x`: Ray Tracing + Environment Mapping
- `c`: Ray Tracing + Normal Mapping
- `z`: Ray Tracing + Soft Shadow

To display these modes from different camera positions:

1. Press any camera movement key.
2. Press the same mode key again.

## Camera Controls

Control the camera movement with the following keys:

- `i`: Pitch Up
- `k`: Pitch Down
- `j`: Yaw Left
- `l`: Yaw Right
- `w`: Move Forward
- `s`: Move Backward
- `a`: Move Left
- `d`: Move Right
- `q`: Move Up
- `e`: Move Down

## Saving Images

- `g`: Save the current view to an image.

Use these instructions to explore the various graphics modes and camera controls provided in the project.