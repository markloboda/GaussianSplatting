struct VertexInput {
    @builtin(vertex_index) index: u32,
    @builtin(instance_index) instanceIndex: u32
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) @interpolate(perspective) offset: vec2<f32>,
    @location(1) @interpolate(flat) uniformScale: f32,
    @location(2) color: u32
}

struct ShaderUniforms {
    model: mat4x4<f32>,      // Model matrix
    view: mat4x4<f32>,       // Camera view matrix
    projection: mat4x4<f32>, // Camera projection matrix
    splatScale: f32          // Scaling of splats
};

struct Splat {
    position: vec4<f32>,
    scale: vec4<f32>,
    color: u32,
    rotation: u32,
};

struct SortSplatsData {
   index: u32,
   z: f32
};

@group(0) @binding(0)
var<storage, read> splats: array<Splat>;

@group(1) @binding(0)
var<storage, read_write> sortedSplats: array<SortSplatsData>;

@group(1) @binding(1)
var<uniform> uUniforms: ShaderUniforms;

@group(1) @binding(2)
var<storage, read> sortSplatsParamsData: array<vec2<u32>>;

@group(1) @binding(3)
var<uniform> uSortSplatsParam: vec2<u32>;

// Define the 2 triangles
var<private> quadVertices: array<vec2<f32>, 4> = array<vec2<f32>, 4>(
    vec2<f32>(1.0, 1.0),   // bottom right
    vec2<f32>(-1.0, 1.0),  // bottom left
    vec2<f32>(1.0, -1.0),    // top right
    vec2<f32>(-1.0, -1.0),   // top left
);

@compute @workgroup_size(256)
fn cs_calculate_sort_splats(@builtin(global_invocation_id) global_id: vec3<u32>) {
   if (global_id.x >= arrayLength(&splats)) {
      return;
   }

   var viewPosition = uUniforms.view * uUniforms.model * splats[global_id.x].position;

   var splat: SortSplatsData;
   splat.index = global_id.x;
   splat.z = viewPosition.z;

   sortedSplats[global_id.x] = splat;
}

@compute @workgroup_size(256)
fn cs_sort_splats(@builtin(global_invocation_id) global_id: vec3<u32>) {
   let k: u32 = uSortSplatsParam.x;
   let j: u32 = uSortSplatsParam.y;
   let i: u32 = global_id.x;

   let l: u32 = i ^ j;

   var temp: SortSplatsData;
   if (i < l) {
      if (
         ((i & k) == 0u) && (sortedSplats[i].z > sortedSplats[l].z) ||
         ((i & k) != 0u) && (sortedSplats[i].z < sortedSplats[l].z)
      ) {
         temp = sortedSplats[i];
         sortedSplats[i] = sortedSplats[l];
         sortedSplats[l] = temp;
      }
   }
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
   var index: u32 = sortedSplats[in.instanceIndex].index;

   var output: VertexOutput;

   let splat = splats[sortedSplats[in.instanceIndex].index];

   let splatViewPosition =  uUniforms.view * uUniforms.model * splat.position;
   let uniformScale = (uUniforms.splatScale / -splatViewPosition.z);
   let vertexOffset = quadVertices[in.index] * uniformScale;

   output.position = uUniforms.projection * (splatViewPosition + vec4<f32>(vertexOffset, 0.0, 0.0));
   output.offset = quadVertices[in.index];
   output.uniformScale = uniformScale;
   output.color = splat.color;

   return output;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
   let color = unpackColor(in.color);
   let offset = sqrt(dot(in.offset, in.offset));

   // Gaussian fallof
   let sigma = in.uniformScale;
   let gauss = gaussianF32(offset, sigma);
   let alpha = color.a * gauss;

   return vec4<f32>(color.rgb, alpha);
}

// Calculates gaussian
fn gaussianF32(x: f32, sigma: f32) -> f32 {
   return exp(-0.5 * x * x * 1 / sigma);
}

fn unpackColor(packed: u32) -> vec4<f32> {
   return vec4<f32>(
      f32((packed >> 24) & 0xFF) / 255.0,
      f32((packed >> 16) & 0xFF) / 255.0,
      f32((packed >> 8) & 0xFF) / 255.0,
      f32(packed & 0xFF) / 255.0,
   );
}