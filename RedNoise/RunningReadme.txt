How to run this project
(I put the makefile in the build folder, because I use relative path to include the header files and load materials, so please run the makefile in the build folder)
1. cd build
2. make
3. ./RedNoise

(if there is no makefile in the build folder, please create a build folder and cd in it and use "cmake .." and it will create the makefile)

Mode description
keypress 1:     random triangle
keypress 2:     draw filled triangle
keypress 3:     draw texture triangle
keypress 4:     Wireframe 3D scene rendering
keypress 5:     Rasterising
keypress 6:     Ray Tracing + Reflection
keypress 7:     Ray Tracing + Refraction
keypress 8:     Ray Tracing + Reflection + Refraction
keypress 9:     Ray Tracing, rendering sphere by using flat shading, gouraud shading or phong shading
keypress x:     Ray Tracing + Environment Mapping
keypress c:     Ray Tracing + Normal mapping
keypress z:     Ray Tracing + soft shadow

How to show these modes in different camera position:
1. press any camera movement key
2. press that mode again


camera movement:
keypress i: Pitch up
keypress k: Pitch down
keypress j: Yaw left
keypress l: Yaw right
keypress w: move camera towards
keypress s: move camera backwards
keypress a: move camera left
keypress d: move camera right
keypress q: move camera up
keypress e: move camera down


save image:
keypress g: save image


the default mode when running this project is keypress 8
