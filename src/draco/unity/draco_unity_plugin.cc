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
#include "draco/unity/draco_unity_plugin.h"
//#ifdef BUILD_UNITY_PLUGIN

namespace draco {

int TestUnityModule() { return 123456; }

    int DecodeMeshForUnity(char *data, unsigned int length,
                           DracoToUnityMesh **tmp_mesh) {
        draco::DecoderBuffer buffer;
        buffer.Init(data, length);
        auto type_statusor = draco::Decoder::GetEncodedGeometryType(&buffer);
        if (!type_statusor.ok()) {
            return -1;
        }
        const draco::EncodedGeometryType geom_type = type_statusor.value();
        if (geom_type != draco::TRIANGULAR_MESH) {
            return -2;
        }
        
        draco::Decoder decoder;
        auto statusor = decoder.DecodeMeshFromBuffer(&buffer);
        if (!statusor.ok()) {
            return -3;
        }
        std::unique_ptr<draco::Mesh> in_mesh = std::move(statusor).value();
        
        *tmp_mesh = new DracoToUnityMesh();
        DracoToUnityMesh *unity_mesh = *tmp_mesh;
        unity_mesh->num_faces = in_mesh->num_faces();
        unity_mesh->num_vertices = in_mesh->num_points();
        
        unity_mesh->indices = new int[in_mesh->num_faces() * 3];
        for (draco::FaceIndex face_id(0); face_id < in_mesh->num_faces(); ++face_id) {
            const Mesh::Face &face = in_mesh->face(draco::FaceIndex(face_id));
            memcpy(unity_mesh->indices + face_id.value() * 3, face.data(),
                   sizeof(int) * 3);
        }
        
        // std::unique_ptr<draco::PointCloud> pc;
        // pc = std::move(in_mesh);
        unity_mesh->position = new float[in_mesh->num_points() * 3];
        const auto pos_att =
        in_mesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
        std::array<float, 3> value;
        for (draco::PointIndex i(0); i < in_mesh->num_points(); ++i) {
            const draco::AttributeValueIndex val_index = pos_att->mapped_index(i);
            if (!pos_att->ConvertValue<float, 3>(val_index, &value[0])) return -8;
            memcpy(unity_mesh->position + i.value() * 3, value.data(),
                   sizeof(float) * 3);
        }
        
        return in_mesh->num_faces();
    }
    /*
    int DecodeMeshForUnityDebug(char *data, unsigned int length,
                                DracoToUnityMesh **tmp_mesh) {
        draco::DecoderBuffer buffer;
        buffer.Init(data, length);
        auto type_statusor = draco::Decoder::GetEncodedGeometryType(&buffer);
        if (!type_statusor.ok()) {
            return -1;
        }
        const draco::EncodedGeometryType geom_type = type_statusor.value();
        if (geom_type != draco::TRIANGULAR_MESH) {
            return -2;
        }
        
        DecoderBuffer temp_buffer(buffer);
        DracoHeader header;
        auto header_statusor = PointCloudDecoder::DecodeHeader(&temp_buffer, &header);
        if (!type_statusor.ok()) {
            return -4;
        }
        if (header.encoder_type != TRIANGULAR_MESH) {
            return -5;
        }
        if (header.encoder_method != MESH_EDGEBREAKER_ENCODING) {
            return -8;
        }
        auto decoder_statusor = CreateMeshDecoder(header.encoder_method);
        if (!decoder_statusor.ok()) {
            return -5;
        }
        std::unique_ptr<MeshDecoder> decoder_2 =
        std::move(std::move(decoder_statusor).value());
        
        DecoderOptions options;
        std::unique_ptr<draco::Mesh> in_mesh(new draco::Mesh());
        if (in_mesh == nullptr) return -9;
        
        auto decode_statusor = decoder_2->Decode(options, &buffer, in_mesh.get());
        if (!decode_statusor.ok()) return -7;
     
        
        *tmp_mesh = new DracoToUnityMesh();
        DracoToUnityMesh *unity_mesh = *tmp_mesh;
        unity_mesh->num_faces = in_mesh->num_faces();
        unity_mesh->num_vertices = in_mesh->num_points();
        
        unity_mesh->indices = new int[in_mesh->num_faces() * 3];
        for (draco::FaceIndex face_id(0); face_id < in_mesh->num_faces(); ++face_id) {
            const Mesh::Face &face = in_mesh->face(draco::FaceIndex(face_id));
            memcpy(unity_mesh->indices + face_id.value() * 3, face.data(),
                   sizeof(int) * 3);
        }
        
        // std::unique_ptr<draco::PointCloud> pc;
        // pc = std::move(in_mesh);
        unity_mesh->position = new float[in_mesh->num_points() * 3];
        const auto pos_att =
        in_mesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
        std::array<float, 3> value;
        for (draco::PointIndex i(0); i < in_mesh->num_points(); ++i) {
            const draco::AttributeValueIndex val_index = pos_att->mapped_index(i);
            if (!pos_att->ConvertValue<float, 3>(val_index, &value[0])) return -8;
            memcpy(unity_mesh->position + i.value() * 3, value.data(),
                   sizeof(float) * 3);
        }
        
        return in_mesh->num_faces();
    } */

}  // namespace draco

//#endif // BUILD_UNITY_PLUGIN