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
    draco::DracoAttribute* CreateDracoAttribute(const draco::PointAttribute* attr)
    {
        auto* const attribute = new draco::DracoAttribute();
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

    template<typename T>
    void GetAttributeArray(std::vector <T> &out, const draco::CsMesh mesh, const T draco::Vertex::*ptr)
    {
        out.resize(mesh.vertexCount);

        for(size_t i = 0; i < mesh.vertexCount; i++)
            out[i] = mesh.vertices[i].*ptr;
    }

}  // namespace

namespace draco
{
    void EXPORT_API EncodeToBuffer(const char* bytes, GLTFDracoOptions gltfDracoOptions, CsMesh mesh)
    {
        size_t triangleCount = mesh.triangleCount;
        size_t vertexCount = mesh.vertexCount;

        uint32_t v2fSize = sizeof(float) * 2;
        uint32_t v3fSize = sizeof(float) * 3;
        uint32_t v4fSize = sizeof(float) * 4;

        auto dracoMesh(std::make_shared<draco::Mesh>());
        dracoMesh->set_num_points(vertexCount);
        dracoMesh->SetNumFaces(triangleCount);

        // Add PositionAttribute
        GeometryAttribute positionAttribute;
        positionAttribute.Init(GeometryAttribute::POSITION, nullptr, 3,
                draco::DT_FLOAT32, false, v3fSize, 0);

        int posIdx = dracoMesh->AddAttribute(positionAttribute, false, vertexCount);
        auto posAttrib = dracoMesh->attribute(posIdx);

        std::vector<std::vector<float>> posArr;
        GetAttributeArray(posArr, mesh, &Vertex::position);
        posAttrib->buffer()->Write(0, &posArr, v3fSize * vertexCount);
        // Eintrag in attributes POSITION : posIdx

        // Add NormalAttribute
        GeometryAttribute normalAttribute;
        normalAttribute.Init(GeometryAttribute::NORMAL, nullptr, 3,
                draco::DT_FLOAT32, false, v3fSize, 0);

        int norIdx = dracoMesh->AddAttribute(normalAttribute, false, vertexCount);
        auto norAttrib = dracoMesh->attribute(norIdx);

        std::vector<std::vector<float>> norArr;
        GetAttributeArray(norArr, mesh, &Vertex::normal);
        norAttrib->buffer()->Write(0, &norArr, v3fSize * vertexCount);
        //Eintrag in attributes NORMAL : norIdx

        // Add TangentAttribute
        GeometryAttribute tangentAttribute;
        tangentAttribute.Init(GeometryAttribute::GENERIC, nullptr, 4,
                draco::DT_FLOAT32, false, v4fSize, 0);

        int tanIdx = dracoMesh->AddAttribute(tangentAttribute, false, vertexCount);
        auto tanAttrib = dracoMesh->attribute(tanIdx);

        std::vector<std::vector<float>> tanArr;
        GetAttributeArray(tanArr, mesh, &Vertex::tangent);
        tanAttrib->buffer()->Write(0, &tanArr, v4fSize * vertexCount);
        //Eintrag in attributes TANGENT : tanIdx

        // Add ColorAttribute
        GeometryAttribute colorAttribute;
        colorAttribute.Init(GeometryAttribute::COLOR, nullptr, 4,
                draco::DT_FLOAT32, false, v4fSize, 0);

        int colIdx = dracoMesh->AddAttribute(colorAttribute, false, vertexCount);
        auto colAttrib = dracoMesh->attribute(colIdx);

        std::vector<std::vector<float>> colArr;
        GetAttributeArray(colArr, mesh, &Vertex::color);
        colAttrib->buffer()->Write(0, &colArr, v4fSize * vertexCount);
        //Eintrag in attributes COLOR_0 : colIdx;

        // Add TexcoordAttribute
        GeometryAttribute texCoords0Attribute;
        texCoords0Attribute.Init(GeometryAttribute::TEX_COORD, nullptr, 2,
                draco::DT_FLOAT32, false, v2fSize, 0);

        int tex0Idx = dracoMesh->AddAttribute(texCoords0Attribute, false, vertexCount);
        auto tex0Attrib = dracoMesh->attribute(tex0Idx);

        std::vector<std::vector<float>> texCoord0Arr;
        GetAttributeArray(texCoord0Arr, mesh, &Vertex::uv0);
        tex0Attrib->buffer()->Write(0, &texCoord0Arr, v2fSize * vertexCount);
        //Eintrag in attributes TEXCOORD_0 : tex0Attrib

        // Add TexcoordAttribute
        GeometryAttribute texCoords1Attribute;
        texCoords1Attribute.Init(GeometryAttribute::TEX_COORD, nullptr, 2,
                draco::DT_FLOAT32, false, v2fSize, 0);

        int tex1Idx = dracoMesh->AddAttribute(texCoords1Attribute, false, vertexCount);
        auto tex1Attrib = dracoMesh->attribute(tex1Idx);

        std::vector<std::vector<float>> texCoord1Arr;
        GetAttributeArray(texCoord1Arr, mesh, &Vertex::uv1);
        tex1Attrib->buffer()->Write(0, &texCoord1Arr, v2fSize * vertexCount);
        //Eintrag in attributes TEXCOORD_1 : tex1Attrib

        // Add JointsAttribute
        GeometryAttribute joints0Attribute;
        joints0Attribute.Init(GeometryAttribute::GENERIC, nullptr, 4,
                draco::DT_UINT16, false, v4fSize, 0);

        int joints0Idx = dracoMesh->AddAttribute(joints0Attribute, false, vertexCount);
        auto joints0Attrib = dracoMesh->attribute(joints0Idx);

        std::vector<std::vector<uint16_t>> joints0Arr;
        GetAttributeArray(joints0Arr, mesh, &Vertex::jointIndices);
         joints0Attrib->buffer()->Write(0, &joints0Arr, vertexCount);
        //Eintrag in attributes JOINTS_0 : joints0Idx

        // Add WeightsAttribute
        GeometryAttribute weights0Attribute;
        weights0Attribute.Init(GeometryAttribute::GENERIC, nullptr, 4,
                draco::DT_FLOAT32, false, v4fSize, 0);

        int weights0Idx = dracoMesh->AddAttribute(weights0Attribute, false, vertexCount);
        auto weights0Attrib = dracoMesh->attribute(weights0Idx);

        std::vector<std::vector<float>> weights0Arr;
        GetAttributeArray(weights0Arr, mesh, &Vertex::jointWeights);
        weights0Attrib->buffer()->Write(0, &weights0Arr, vertexCount);
        //Eintrag in attributes WEIGHTS_0 : weights0Idx

        // AddFacesToMesh
        for(uint32_t i = 0; i < triangleCount; i++)
        {
            draco::Mesh::Face face;
            face[0] = mesh.triangles[i].verts[0];
            face[1] = mesh.triangles[i].verts[1];
            face[2] = mesh.triangles[i].verts[2];
            dracoMesh->SetFace(draco::FaceIndex(i), face);
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

        bytes = dracoBuffer.data();
    };

    void EXPORT_API ReleaseDracoMesh(DracoMesh** mesh_ptr)
    {
        if (!mesh_ptr)
            return;

        const DracoMesh* const mesh = *mesh_ptr;

        if (!mesh)
            return;

        const Mesh* const m = static_cast<const Mesh*>(mesh->private_mesh);
        delete m;
        delete mesh;
        *mesh_ptr = nullptr;
    }

    void EXPORT_API ReleaseDracoAttribute(DracoAttribute** attr_ptr)
    {
        if (!attr_ptr)
            return;

        const DracoAttribute* const attr = *attr_ptr;

        if (!attr)
            return;

        delete attr;
        *attr_ptr = nullptr;
    }

    void EXPORT_API ReleaseDracoData(DracoData** data_ptr)
    {
        if (!data_ptr)
            return;

        const DracoData* const data = *data_ptr;

        switch (data->data_type)
        {
            case draco::DataType::DT_INT8:
                delete[] static_cast<int8_t*>(data->data);
                break;
            case draco::DataType::DT_UINT8:
                delete[] static_cast<uint8_t*>(data->data);
                break;
            case draco::DataType::DT_INT16:
                delete[] static_cast<int16_t*>(data->data);
                break;
            case draco::DataType::DT_UINT16:
                delete[] static_cast<uint16_t*>(data->data);
                break;
            case draco::DataType::DT_INT32:
                delete[] static_cast<int32_t*>(data->data);
                break;
            case draco::DataType::DT_UINT32:
                delete[] static_cast<uint32_t*>(data->data);
                break;
            case draco::DataType::DT_FLOAT32:
                delete[] static_cast<float*>(data->data);
                break;
            default:
                break;
        }

        delete data;
        *data_ptr = nullptr;
    }

    int EXPORT_API DecodeDracoMesh(char* data, unsigned int length, DracoMesh** mesh)
    {
        if (mesh == nullptr || *mesh != nullptr)
            return -1;

        draco::DecoderBuffer buffer;
        buffer.Init(data, length);
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

        *mesh = new DracoMesh();
        DracoMesh* const unity_mesh = *mesh;
        unity_mesh->num_faces = in_mesh->num_faces();
        unity_mesh->num_vertices = in_mesh->num_points();
        unity_mesh->num_attributes = in_mesh->num_attributes();
        unity_mesh->private_mesh = static_cast<void*>(in_mesh.release());

        return unity_mesh->num_faces;
    }

    bool EXPORT_API GetAttribute(const DracoMesh* mesh, int index, DracoAttribute** attribute)
    {
        if (mesh == nullptr || attribute == nullptr || *attribute != nullptr)
            return false;

        const auto m = static_cast<const Mesh*>(mesh->private_mesh);
        const auto attr = m->attribute(index);

        if (attr == nullptr)
            return false;

        *attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetAttributeByType(const DracoMesh* mesh, GeometryAttribute::Type type, int index,
            DracoAttribute** attribute)
    {
        if (mesh == nullptr || attribute == nullptr || *attribute != nullptr)
            return false;

        const auto m = static_cast<const Mesh*>(mesh->private_mesh);
        auto att_type = static_cast<GeometryAttribute::Type>(type);
        const auto attr = m->GetNamedAttribute(att_type, index);

        if (attr == nullptr)
            return false;

        *attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetAttributeByUniqueId(const DracoMesh* mesh, int unique_id, DracoAttribute** attribute)
    {
        if (mesh == nullptr || attribute == nullptr || *attribute != nullptr)
            return false;

        const Mesh* const m = static_cast<const Mesh*>(mesh->private_mesh);
        const PointAttribute* const attr = m->GetAttributeByUniqueId(unique_id);

        if (attr == nullptr)
            return false;

        *attribute = CreateDracoAttribute(attr);
        return true;
    }

    bool EXPORT_API GetMeshIndices(const DracoMesh* mesh, DracoData** indices)
    {
        if (mesh == nullptr || indices == nullptr || *indices != nullptr)
            return false;

        const Mesh* const m = static_cast<const Mesh*>(mesh->private_mesh);
        int* const temp_indices = new int[m->num_faces() * 3];

        for (draco::FaceIndex face_id(0); face_id < m->num_faces(); ++face_id)
        {
            const Mesh::Face &face = m->face(draco::FaceIndex(face_id));
            memcpy(temp_indices + face_id.value() * 3, reinterpret_cast<const int*>(face.data()), sizeof(int) * 3);
        }

        auto* const draco_data = new DracoData();
        draco_data->data = temp_indices;
        draco_data->data_type = DT_INT32;
        *indices = draco_data;

        return true;
    }

    bool EXPORT_API GetAttributeData(const DracoMesh* mesh, const DracoAttribute* attribute, DracoData** data)
    {
        if (mesh == nullptr || data == nullptr || *data != nullptr)
            return false;

        const auto m = static_cast<const Mesh*>(mesh->private_mesh);
        const auto* const attr = static_cast<const PointAttribute*>(attribute->private_attribute);

        void* temp_data = ConvertAttributeData(m->num_points(), attr);

        if (temp_data == nullptr)
            return false;

        auto* const draco_data = new DracoData();
        draco_data->data = temp_data;
        draco_data->data_type = static_cast<DataType>(attr->data_type());
        *data = draco_data;

        return true;
    }

}  // namespace draco

#endif  // BUILD_SHARP
