# GPUMark
GPUMark is a performance test suite.

<table>
  <tr align=center>
    <td><strong>Feature</td>
    <td><strong>Benchmark</td>
    <td><strong>Description</td>
  </tr>
  <tr align=left>
    <td>MultiSample Anti-Aliasing (MSAA)</td>
    <td>aquarium</td>
    <td></td>
  </tr>
  <tr align=left>
    <td>Render Pass</td>
    <td>aquarium</td>
    <td></td>
  </tr>
  <tr align=left>
    <td>Variable Rate Shading (VRS)</td>
    <td>asteroid</td>
    <td>https://gitlab.devtools.intel.com/web/asteroids_vrs_vi</td>
  </tr>
  <tr align=left>
    <td>View Instancing (VI)</td>
    <td>asteroid</td>
    <td>https://gitlab.devtools.intel.com/web/asteroids_vrs_vi</td>
  </tr>
  <tr align=left>
    <td>Compute Shader</td>
    <td>d3d11_compute, d3d12_compute</td>
    <td></td>
  </tr>
  <tr align=left>
    <td>Overlay</td>
    <td>vp_overlay</td>
    <td></td>
  </tr>
  <tr align=left>
    <td>Async Compute</td>
    <td>nbody</td>
    <td>https://github.com/GPUOpen-LibrariesAndSDKs/nBodyD3D12/tree/master/Samples/D3D12nBodyGravity</td>
  </tr>
</table>

Provision
* Ensure you have depot_tools and it's in your path
* gclient sync
* python gpumark.py --release
