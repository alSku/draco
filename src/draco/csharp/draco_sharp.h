// Copyright 2017 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef DRACO_SHARP_H_
#define DRACO_SHARP_H_

#include <fstream>

#include "draco/attributes/geometry_attribute.h"
#include "draco/core/draco_types.h"

#include "draco/compression/decode.h"
#include "draco/compression/encode.h"

#ifdef BUILD_SHARP

// If compiling with Visual Studio.
#if defined(_MSC_VER)
#define EXPORT_API __declspec(dllexport)
#else
// Other platforms don't need this.
#define EXPORT_API
#endif  // defined(_MSC_VER)

namespace dracosharp {

extern "C" {

// Struct representing Draco attribute data within Unity.
struct EXPORT_API DracoData {
  DracoData() : data_type(draco::DT_INVALID), data(nullptr) {}

  draco::DataType data_type;
  void *data;
};

// Struct representing a Draco attribute within Unity.
struct EXPORT_API DracoAttribute {
  DracoAttribute()
      : attribute_type(draco::GeometryAttribute::INVALID),
        data_type(draco::DT_INVALID),
        num_components(0),
        unique_id(0),
        private_attribute(nullptr) {}

  draco::GeometryAttribute::Type attribute_type;
  draco::DataType data_type;
  int num_components;
  int unique_id;
  const void *private_attribute;
};

// Struct representing a Draco mesh.
struct EXPORT_API DracoMesh {
  DracoMesh()
      : num_faces(0),
        num_vertices(0),
        num_attributes(0),
        private_mesh(nullptr) {}

  int num_faces;
  int num_vertices;
  int num_attributes;
  void *private_mesh;
};

struct EXPORT_API Vertex
{
  float position[3] = {}; // 3f
  float normal[3] = {}; // 3f
  float binormal[3] = {}; // 3f
  float tangent[3] = {}; // 3f
  float color[4] = {}; // 4f
  float uv0[2] = {}; // 2f
  float uv1[2] = {}; // 2f
  uint16_t jointIndices[4] = {}; // 4i
  float jointWeights[4] = {}; // 4f
  // end of members that directly correspond to vertex attributes

  Vertex() = default;

  void printPos();
};

struct EXPORT_API Triangle
{
  int vertices[3];
  int surfaceIndex;

  Triangle() = default;
};

struct EXPORT_API CsMesh
{
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;

  size_t triangleCount{ 0 };
  size_t vertexCount{ 0 };

  bool hasNormals = false;
  bool hasBinormal = false;
  bool hasTangents = false;
  bool hasColor = false;
  bool hasUv0 = false;
  bool hasUv1 = false;
  bool hasJointIndices = false;
  bool hasJointWeights = false;

  CsMesh() = default;

  void clearVertices();
  void clearTriangles();

  void pushBackVertex(Vertex vertex);
  void pushBackTriangle(Triangle triangle);

  void eraseVertexAt(int index);
  void eraseTriangleAt(int index);
};

struct EXPORT_API GLTFDracoOptions{
    int positionQuantizationBits;
    int texCoordsQuantizationBits;
    int normalsQuantizationBits;
    int colorQuantizationBits;
    int genericQuantizationBits;
    int compressionLevel;

    GLTFDracoOptions() :
        positionQuantizationBits(11),
        texCoordsQuantizationBits(10),
        normalsQuantizationBits(7),
        colorQuantizationBits(8),
        genericQuantizationBits(8),
        compressionLevel(7)
    {
    }

    void PrintGltfDracoOptions() const;
};

// Decodes compressed Draco mesh in |data| and returns |mesh|. On input, |mesh|
// must be null. The returned |mesh| must be released with ReleaseDracoMesh.
int EXPORT_API DecodeDracoMesh(char *data, unsigned int length, DracoMesh *mesh);

// Encodes given CsMesh to DracoMesh
void EXPORT_API EncodeToBuffer(const char* outputData, GLTFDracoOptions gltfDracoOptions, CsMesh mesh);

// Returns |attribute| at |index| in |mesh|.  On input, |attribute| must be
// null. The returned |attribute| must be released with ReleaseDracoAttribute.
bool EXPORT_API GetAttribute(const DracoMesh mesh, int index, DracoAttribute *attribute);

// Returns |attribute| of |type| at |index| in |mesh|. E.g. If the mesh has
// two texture coordinates then GetAttributeByType(mesh,
// AttributeType.TEX_COORD, 1, &attr); will return the second TEX_COORD
// attribute. On input, |attribute| must be null. The returned |attribute| must
// be released with ReleaseDracoAttribute.
bool EXPORT_API GetAttributeByType(const DracoMesh mesh, draco::GeometryAttribute::Type type, int index, DracoAttribute *attribute);

// Returns |attribute| with |unique_id| in |mesh|. On input, |attribute| must be
// null. The returned |attribute| must be released with ReleaseDracoAttribute.
bool EXPORT_API GetAttributeByUniqueId(const DracoMesh mesh, int unique_id, DracoAttribute *attribute);

// Returns the indices as well as the type of data in |indices|. On input,
// |indices| must be null. The returned |indices| must be released with
// ReleaseDracoData.
bool EXPORT_API GetMeshIndices(const DracoMesh mesh, DracoData *indices);

// Returns the attribute data from attribute as well as the type of data in
// |data|. On input, |data| must be null. The returned |data| must be released
// with ReleaseDracoData.
bool EXPORT_API GetAttributeData(const DracoMesh mesh, const DracoAttribute attribute, DracoData *data);

}  // extern "C"

}  // namespace dracosharp

#endif  // BUILD_SHARP

#endif  // DRACO_SHARP_H_
