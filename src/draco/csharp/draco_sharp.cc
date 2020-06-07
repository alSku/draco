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
#include "../csharp/draco_sharp.h"

#ifdef BUILD_SHARP

namespace
{
    // Returns a DracoAttribute from a PointAttribute.
    dracosharp::DracoAttribute* CreateDracoAttribute(const draco::PointAttribute* attr)
    {
        auto* const attribute = new dracosharp::DracoAttribute();
        attribute->attribute_type = static_cast<draco::GeometryAttribute::Type>(attr->attribute_type());
        attribute->data_type = static_cast<draco::DataType>(attr->data_type());
        attribute->num_components = attr->num_components();
        attribute->unique_id = attr->unique_id();
        attribute->private_attribute = static_cast<const void*>(attr);

        return attribute;
    }

    // Returns the attribute data in |attr| as an array of type T.
    template<typename T>
    T* CopyAttributeData(int num_points, const draco::PointAttribute* attr)
    {
        const int num_components = attr->num_components();
        T* const data = new T[num_points * num_components];

        for (draco::PointIndex i(0); i < num_points; ++i)
        {
            const draco::AttributeValueIndex val_index = attr->mapped_index(i);
            bool got_data = false;

            switch (num_components)
            {
                case 1:
                    got_data = attr->ConvertValue<T, 1>(val_index, data + i.value() * num_components);
                    break;
                case 2:
                    got_data = attr->ConvertValue<T, 2>(val_index, data + i.value() * num_components);
                    break;
                case 3:
                    got_data = attr->ConvertValue<T, 3>(val_index, data + i.value() * num_components);
                    break;
                case 4:
                    got_data = attr->ConvertValue<T, 4>(val_index, data + i.value() * num_components);
                    break;
                default:
                    break;
            }

            if (!got_data)
            {
                delete[] data;
                return nullptr;
            }
        }

        return data;
    }

    // Returns the attribute data in |attr| as an array of void*.
    void* ConvertAttributeData(int num_points, const draco::PointAttribute* attr)
    {
        switch (attr->data_type())
        {
            case draco::DataType::DT_INT8:
                return static_cast<void*>(CopyAttributeData<int8_t>(num_points, attr));
            case draco::DataType::DT_UINT8:
                return static_cast<void*>(CopyAttributeData<uint8_t>(num_points, attr));
            case draco::DataType::DT_INT16:
                return static_cast<void*>(CopyAttributeData<int16_t>(num_points, attr));
            case draco::DataType::DT_UINT16:
                return static_cast<void*>(CopyAttributeData<uint16_t>(num_points, attr));
            case draco::DataType::DT_INT32:
                return static_cast<void*>(CopyAttributeData<int32_t>(num_points, attr));
            case draco::DataType::DT_UINT32:
                return static_cast<void*>(CopyAttributeData<uint32_t>(num_points, attr));
            case draco::DataType::DT_FLOAT32:
                return static_cast<void*>(CopyAttributeData<float>(num_points, attr));
            default:
                return nullptr;
        }
    }

    template<typename DataTypeT>
    int AddAttribute(draco::PointCloud* pc, draco::GeometryAttribute::Type type, long num_vertices, long num_components,
            const DataTypeT* att_values, draco::DataType draco_data_type)
    {
        if (!pc)
            return -1;

        std::unique_ptr<draco::PointAttribute> att(new draco::PointAttribute());
        att->Init(type, num_components, draco_data_type, /* normalized */ false, num_vertices);

        const int att_id = pc->AddAttribute(std::move(att));
        draco::PointAttribute* const att_ptr = pc->attribute(att_id);

        for (draco::PointIndex i(0); i < num_vertices; ++i)
            att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &att_values[i.value() * num_components]);

        if (pc->num_points() == 0)
            pc->set_num_points(num_vertices);
        else if (pc->num_points() != num_vertices)
            return -1;

        return att_id;
    }

    template<typename T, typename U>
    void GetAttributeArray(std::vector<T> &out, const dracosharp::CsMesh mesh, const U dracosharp::Vertex::*ptr)
    {
        out.resize(mesh.vertexCount);

        for(size_t i = 0; i < mesh.vertexCount; i++)
        {
          T dest(std::begin(mesh.vertices[i].*ptr), std::end(mesh.vertices[i].*ptr));
          out[i] = dest;
        }
    }

    template<typename T>
    void WriteAttributeValue(std::vector<std::vector<T>>& vector, int dimensions, draco::PointAttribute* pointAttribute)
    {
        std::vector <uint8_t> buf(sizeof(float));

        for(uint32_t ii = 0; ii < vector.size(); ii++)
        {
            uint8_t* ptr = &buf[0];

            for (int iii = 0; iii < dimensions; iii++)
              ((T*) ptr)[iii] = vector[ii][iii];

            pointAttribute->SetAttributeValue(pointAttribute->mapped_index(draco::PointIndex(ii)), ptr);
        }
    }

}  // namespace

namespace dracosharp
{
    void EXPORT_API EncodeToBuffer(const char* outputData, GLTFDracoOptions gltfDracoOptions, CsMesh mesh)
      {
//          std::cout << "TriangleCount: " << mesh.triangleCount << std::endl;
//
//          std::cout
//              << "First Triangle: "
//              << mesh.triangles[0].vertices[0]
//              << mesh.triangles[0].vertices[1]
//              << mesh.triangles[0].vertices[2]
//              << std::endl;

          size_t triangleCount = mesh.triangleCount;
          size_t vertexCount = mesh.vertexCount;

          uint32_t v2fSize = sizeof(float) * 2;
          uint32_t v3fSize = sizeof(float) * 3;
          uint32_t v4fSize = sizeof(float) * 4;
          uint32_t v4uiSize = sizeof(uint16_t) * 4;

          auto dracoMesh(std::make_shared<draco::Mesh>());
          dracoMesh->set_num_points(vertexCount);
          dracoMesh->SetNumFaces(triangleCount);

          // Add PositionAttribute
          draco::GeometryAttribute positionAttribute;
          positionAttribute.Init(draco::GeometryAttribute::POSITION, nullptr, 3,
                  draco::DT_FLOAT32, false, v3fSize, 0);

          int posIdx = dracoMesh->AddAttribute(positionAttribute, true, vertexCount);
          auto posAttrib = dracoMesh->attribute(posIdx);

//          std::cout << "vertexCount: " << vertexCount << std::endl;
//          std::cout << "Pos Idx: " << posIdx << std::endl;
//          std::cout << "Pos Attr: " << draco::PointAttribute::TypeToString(posAttrib->attribute_type()) << std::endl;

          std::vector<std::vector<float>> posArr;
          GetAttributeArray(posArr, mesh, &Vertex::position);
          WriteAttributeValue(posArr, 3, posAttrib);
          // Eintrag in attributes POSITION : posIdx

          // Add NormalAttribute
          if(mesh.hasNormals)
          {
              draco::GeometryAttribute normalAttribute;
              normalAttribute.Init(draco::GeometryAttribute::NORMAL, nullptr, 3,
                  draco::DT_FLOAT32, false, v3fSize, 0);

              int norIdx = dracoMesh->AddAttribute(normalAttribute, false, vertexCount);
              auto norAttrib = dracoMesh->attribute(norIdx);

              std::vector<std::vector<float>> norArr;
              GetAttributeArray(norArr, mesh, &Vertex::normal);
              WriteAttributeValue(norArr, 3, norAttrib);
              //Eintrag in attributes NORMAL : norIdx
          }

          // Add TangentAttribute
          if(mesh.hasTangents)
          {
              draco::GeometryAttribute tangentAttribute;
              tangentAttribute.Init(draco::GeometryAttribute::GENERIC, nullptr, 4,
                  draco::DT_FLOAT32, false, v3fSize, 0);

              int tanIdx = dracoMesh->AddAttribute(tangentAttribute, false, vertexCount);
              auto tanAttrib = dracoMesh->attribute(tanIdx);

              std::vector<std::vector<float>> tanArr;
              GetAttributeArray(tanArr, mesh, &Vertex::tangent);
              WriteAttributeValue(tanArr, 3, tanAttrib);
              //Eintrag in attributes TANGENT : tanIdx
          }

          // Add ColorAttribute
          if(mesh.hasColor)
          {
              draco::GeometryAttribute colorAttribute;
              colorAttribute.Init(draco::GeometryAttribute::COLOR, nullptr, 4,
                  draco::DT_FLOAT32, false, v4fSize, 0);

              int colIdx = dracoMesh->AddAttribute(colorAttribute, false, vertexCount);
              auto colAttrib = dracoMesh->attribute(colIdx);

              std::vector<std::vector<float>> colArr;
              GetAttributeArray(colArr, mesh, &Vertex::color);
              WriteAttributeValue(colArr, 4, colAttrib);
              //Eintrag in attributes COLOR_0 : colIdx;
          }

          // Add Texcoord0Attribute
          if(mesh.hasUv0)
          {
              draco::GeometryAttribute texCoords0Attribute;
              texCoords0Attribute.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2,
                  draco::DT_FLOAT32, false, v2fSize, 0);

              int tex0Idx = dracoMesh->AddAttribute(texCoords0Attribute, false, vertexCount);
              auto tex0Attrib = dracoMesh->attribute(tex0Idx);

              std::vector<std::vector<float>> texCoord0Arr;
              GetAttributeArray(texCoord0Arr, mesh, &Vertex::uv0);
              WriteAttributeValue(texCoord0Arr, 2, tex0Attrib);
              //Eintrag in attributes TEXCOORD_0 : tex0Attrib
          }

          // Add Texcoord1Attribute
          if(mesh.hasUv1)
          {
              draco::GeometryAttribute texCoords1Attribute;
              texCoords1Attribute.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2,
                  draco::DT_FLOAT32, false, v2fSize, 0);

              int tex1Idx = dracoMesh->AddAttribute(texCoords1Attribute, false, vertexCount);
              auto tex1Attrib = dracoMesh->attribute(tex1Idx);

              std::vector<std::vector<float>> texCoord1Arr;
              GetAttributeArray(texCoord1Arr, mesh, &Vertex::uv1);
              WriteAttributeValue(texCoord1Arr, 2, tex1Attrib);
              //Eintrag in attributes TEXCOORD_1 : tex1Attrib
          }

          // Add JointsAttribute
          if(mesh.hasJointIndices)
          {
              draco::GeometryAttribute joints0Attribute;
              joints0Attribute.Init(draco::GeometryAttribute::GENERIC, nullptr, 4,
                  draco::DT_UINT16, false, v4uiSize, 0);

              int joints0Idx = dracoMesh->AddAttribute(joints0Attribute, false, vertexCount);
              auto joints0Attrib = dracoMesh->attribute(joints0Idx);

              std::vector<std::vector<uint16_t>> joints0Arr;
              GetAttributeArray(joints0Arr, mesh, &Vertex::jointIndices);
              WriteAttributeValue(joints0Arr, 4, joints0Attrib);
              //Eintrag in attributes JOINTS_0 : joints0Idx
          }

          // Add WeightsAttribute
          if(mesh.hasJointWeights)
          {
              draco::GeometryAttribute weights0Attribute;
              weights0Attribute.Init(draco::GeometryAttribute::GENERIC, nullptr, 4,
                  draco::DT_FLOAT32, false, v4fSize, 0);

              int weights0Idx = dracoMesh->AddAttribute(weights0Attribute, false, vertexCount);
              auto weights0Attrib = dracoMesh->attribute(weights0Idx);

              std::vector<std::vector<float>> weights0Arr;
              GetAttributeArray(weights0Arr, mesh, &Vertex::jointWeights);
              WriteAttributeValue(weights0Arr, 4, weights0Attrib);
              //Eintrag in attributes WEIGHTS_0 : weights0Idx
          }

          // AddFacesToMesh
          for(uint32_t c = 0; c < triangleCount; c++)
          {
              draco::Mesh::Face face;
              face[0] = mesh.triangles[c].vertices[0];
              face[1] = mesh.triangles[c].vertices[1];
              face[2] = mesh.triangles[c].vertices[2];

//              std::cout << "Face" << c << ": " << face[0] << ", " << face[1] << ", " << face[2] << ", " << std::endl;
//              std::cout << "FaceIndex: " << c << std::endl;

              dracoMesh->SetFace(draco::FaceIndex(c), face);
          }

          //Set up the encoder.
          draco::Encoder encoder;

          if(gltfDracoOptions.compressionLevel != -1) // default = 7
          {
              int dracoSpeed = 10 - gltfDracoOptions.compressionLevel;
              encoder.SetSpeedOptions(dracoSpeed, dracoSpeed);
          }

          if(gltfDracoOptions.positionQuantizationBits != -1) // default = 11
              encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, gltfDracoOptions.positionQuantizationBits);

          if(gltfDracoOptions.texCoordsQuantizationBits != -1) // default = 10
              encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, gltfDracoOptions.texCoordsQuantizationBits);

          if(gltfDracoOptions.normalsQuantizationBits != -1) // default = 7
              encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, gltfDracoOptions.normalsQuantizationBits);

          if(gltfDracoOptions.colorQuantizationBits != -1) // default = 8
              encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, gltfDracoOptions.colorQuantizationBits);

          if(gltfDracoOptions.genericQuantizationBits != -1) // default = 8
              encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, gltfDracoOptions.genericQuantizationBits);

  #ifdef DRACO_ATTRIBUTE_DEDUPLICATION_SUPPORTED
          dracoMesh->DeduplicateAttributeValues();
          dracoMesh->DeduplicatePointIds();
  #endif

          draco::EncoderBuffer dracoBuffer;
          draco::Status status = encoder.EncodeMeshToBuffer(*dracoMesh, &dracoBuffer);

          assert(status.code() == draco::Status::OK);

          outputData = dracoBuffer.data();
      }

    int EXPORT_API DecodeDracoMesh(char* inputData, unsigned int length, DracoMesh* mesh)
    {
        if (mesh == nullptr)
          return -1;

        draco::DecoderBuffer buffer;
        buffer.Init(inputData, length);
        auto type_statusOr = draco::Decoder::GetEncodedGeometryType(&buffer);

        if (!type_statusOr.ok())
        {
            // TODO(draco-eng): Use enum instead.
            return -2;
        }

        const draco::EncodedGeometryType geom_type = type_statusOr.value();

        if (geom_type != draco::TRIANGULAR_MESH)
            return -3;

        draco::Decoder decoder;
        auto statusOr = decoder.DecodeMeshFromBuffer(&buffer);

        if (!statusOr.ok())
            return -4;

        std::unique_ptr<draco::Mesh> in_mesh = std::move(statusOr).value();

        mesh = new DracoMesh();
        DracoMesh* const dracoMesh = mesh;
        dracoMesh->num_faces = in_mesh->num_faces();
        dracoMesh->num_vertices = in_mesh->num_points();
        dracoMesh->num_attributes = in_mesh->num_attributes();
        dracoMesh->private_mesh = static_cast<void*>(in_mesh.release());

        return dracoMesh->num_faces;
    }

    bool EXPORT_API GetAttribute(const DracoMesh mesh, int index, DracoAttribute* attribute)
    {
        if (attribute == nullptr)
            return false;

        const auto m = static_cast<const draco::Mesh*>(mesh.private_mesh);
        const auto attr = m->attribute(index);

        if (attr == nullptr)
            return false;

        attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetAttributeByType(const DracoMesh mesh, draco::GeometryAttribute::Type type, int index, DracoAttribute* attribute)
    {
        if (attribute == nullptr)
            return false;

        const auto m = static_cast<const draco::Mesh*>(mesh.private_mesh);
        auto att_type = static_cast<draco::GeometryAttribute::Type>(type);
        const auto attr = m->GetNamedAttribute(att_type, index);

        if (attr == nullptr)
            return false;

        attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetAttributeByUniqueId(const DracoMesh mesh, int unique_id, DracoAttribute* attribute)
    {
        if (attribute == nullptr)
            return false;

        const auto* const m = static_cast<const draco::Mesh*>(mesh.private_mesh);
        const auto attr = m->GetAttributeByUniqueId(unique_id);

        if (attr == nullptr)
            return false;

        attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetMeshIndices(const DracoMesh mesh, DracoData* indices)
    {
        if (indices == nullptr)
            return false;

        const auto* const m = static_cast<const draco::Mesh*>(mesh.private_mesh);
        int* const temp_indices = new int[m->num_faces() * 3];

        for (draco::FaceIndex face_id(0); face_id < m->num_faces(); ++face_id)
        {
            const draco::Mesh::Face &face = m->face(draco::FaceIndex(face_id));
            memcpy(temp_indices + face_id.value() * 3, reinterpret_cast<const int*>(face.data()), sizeof(int) * 3);
        }

        auto* const draco_data = new DracoData();
        draco_data->data = temp_indices;
        draco_data->data_type = draco::DT_INT32;
        indices = draco_data;

        return true;
    }

    bool EXPORT_API GetAttributeData(const DracoMesh mesh, const DracoAttribute attribute, DracoData* data)
    {
        if (data == nullptr)
            return false;

        const auto m = static_cast<const draco::Mesh*>(mesh.private_mesh);
        const auto* const attr = static_cast<const draco::PointAttribute*>(attribute.private_attribute);

        void* temp_data = ConvertAttributeData(m->num_points(), attr);

        if (temp_data == nullptr)
            return false;

        auto* const draco_data = new DracoData();
        draco_data->data = temp_data;
        draco_data->data_type = static_cast<draco::DataType>(attr->data_type());
        data = draco_data;

        return true;
    }

    void GLTFDracoOptions::PrintGltfDracoOptions() const
    {
      std::cout << "GLTFDracoOptions:" << std::endl;
      std::cout << "PositionQuantizationBits: " << positionQuantizationBits << std::endl;
      std::cout << "TexCoordsQuantizationBits: " << texCoordsQuantizationBits << std::endl;
      std::cout << "NormalsQuantizationBits: " << normalsQuantizationBits << std::endl;
      std::cout << "ColorQuantizationBits: " << colorQuantizationBits << std::endl;
      std::cout << "GenericQuantizationBits: " << genericQuantizationBits << std::endl;
      std::cout << "CompressionLevel: " << compressionLevel << std::endl << std::endl;
    }

    void CsMesh::clearTriangles()
    {
      triangles.clear();
      triangleCount = 0;
    }

    void CsMesh::clearVertices()
    {
      vertices.clear();
      vertexCount = 0;
    }

    void CsMesh::pushBackVertex(Vertex vertex)
    {
      vertices.push_back(vertex);
      vertexCount++;
    }

    void CsMesh::pushBackTriangle(Triangle triangle)
    {
      triangles.push_back(triangle);
      triangleCount++;
    }

    void CsMesh::eraseVertexAt(int index)
    {
      if (vertices.size() <= index)
        return;

      vertices.erase(vertices.begin() + index);
      vertexCount--;
    }

    void CsMesh::eraseTriangleAt(int index)
    {
      if (triangles.size() <= index)
        return;

      triangles.erase(triangles.begin() + index);
      triangleCount--;
    }

    void Vertex::printPos()
    {
      std::cout << "Address of vertex position: " << &position << std::endl;
      std::cout << position[0] << " " << position[1] << " " << position[2] << std::endl << std::endl;
    }

}  // namespace dracosharp

#endif  // BUILD_SHARP
