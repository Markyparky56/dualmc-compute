# dualmc-compute

Testbed for evaluating and refining a compute shader version of the Dual Marching Cubes algo

Based on the [implementation from dominikwodniok](https://github.com/dominikwodniok/dualmc) (BSD 3-clause) 

# Features
- Optimised packing of volume data and LUTs in shorts and bytes
- 6-bit Morton encoding to improve neighbourhood lookups
- Per-face normals

Does *not* currently support per-vertex normals (could be generated off GPU) \
Does *not* currently support vertex indexing (could be generated off GPU with meshoptimizer?)

# Status
Functional! 🎉

Program will execute for a single 36x36x36 volume, outputting the generated triangles, normals, and faces to a .obj for inspection

Triangle output buffer array is a bit oversized, have yet to investigate realistic upper-bound. Theoretical worst case is 34\*34\*34\*18 (707472) triangles if it were somehow to generate the maximum number of triangles for _every_ cell, which I'm certain is impossible.

Could maybe trim off some bytes from the Volume Data buffer array as well. Morton Codes can lead to some gaps in the array so even though we're generating a 36x36x36 volume, we need to work with the 6-bit encoding in mind. 6-bit means indices [0..64) could pop out. I'll get around to generating the *actual* highest-used index for our volume space next, since I'm certain we don't need 1MiB per volume. 

# Cloning
Repo contains submodules. Remember to clone recursively!

```
git clone --recursive https://github.com/Markyparky56/dualmc-compute.git
```
or
```
git clone https://github.com/Markyparky56/dualmc-compute.git
cd dualmc-compute
git submodule update --init --recursive
```