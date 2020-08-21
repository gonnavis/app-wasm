mkdir -p bin
if [ ! -f temp.o ]; then
  emcc -s WASM=1 -s USE_PTHREADS=1 -s NO_FILESYSTEM=1 -O3 \
		draco/mesh/mesh_misc_functions.cc \
		draco/mesh/mesh_attribute_corner_table.cc \
		draco/mesh/corner_table.cc \
		draco/mesh/mesh_cleanup.cc \
		draco/mesh/triangle_soup_mesh_builder.cc \
		draco/mesh/mesh_stripifier.cc \
		draco/mesh/mesh_are_equivalent.cc \
		draco/mesh/mesh.cc \
		draco/point_cloud/point_cloud_builder.cc \
		draco/point_cloud/point_cloud.cc \
		draco/core/bounding_box.cc \
		draco/core/bit_utils.cc \
		draco/core/data_buffer.cc \
		draco/core/hash_utils.cc \
		draco/core/cycle_timer.cc \
		draco/core/divide.cc \
		draco/core/quantization_utils.cc \
		draco/core/options.cc \
		draco/core/decoder_buffer.cc \
		draco/core/encoder_buffer.cc \
		draco/core/draco_types.cc \
		draco/attributes/point_attribute.cc \
		draco/attributes/attribute_octahedron_transform.cc \
		draco/attributes/attribute_quantization_transform.cc \
		draco/attributes/geometry_attribute.cc \
		draco/attributes/attribute_transform.cc \
		draco/compression/mesh/mesh_edgebreaker_decoder_impl.cc \
		draco/compression/mesh/mesh_edgebreaker_decoder.cc \
		draco/compression/mesh/mesh_decoder.cc \
		draco/compression/mesh/mesh_sequential_decoder.cc \
		draco/compression/point_cloud/point_cloud_kd_tree_decoder.cc \
		draco/compression/point_cloud/point_cloud_sequential_decoder.cc \
		draco/compression/point_cloud/point_cloud_decoder.cc \
		draco/compression/point_cloud/algorithms/float_points_tree_decoder.cc \
		draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_decoder.cc \
		draco/compression/entropy/symbol_encoding.cc \
		draco/compression/entropy/shannon_entropy.cc \
		draco/compression/entropy/symbol_decoding.cc \
		draco/compression/bit_coders/adaptive_rans_bit_encoder.cc \
		draco/compression/bit_coders/symbol_bit_encoder.cc \
		draco/compression/bit_coders/direct_bit_decoder.cc \
		draco/compression/bit_coders/rans_bit_decoder.cc \
		draco/compression/bit_coders/adaptive_rans_bit_decoder.cc \
		draco/compression/bit_coders/symbol_bit_decoder.cc \
		draco/compression/bit_coders/rans_bit_encoder.cc \
		draco/compression/bit_coders/direct_bit_encoder.cc \
		draco/compression/attributes/sequential_integer_attribute_decoder.cc \
		draco/compression/attributes/sequential_attribute_decoder.cc \
		draco/compression/attributes/sequential_attribute_decoders_controller.cc \
		draco/compression/attributes/sequential_quantization_attribute_decoder.cc \
		draco/compression/attributes/sequential_normal_attribute_decoder.cc \
		draco/compression/attributes/kd_tree_attributes_decoder.cc \
		draco/compression/attributes/attributes_decoder.cc \
		draco/compression/decode.cc \
		draco/metadata/metadata.cc \
		draco/metadata/geometry_metadata.cc \
		draco/metadata/metadata_decoder.cc \
		draco/compression/mesh/mesh_edgebreaker_encoder_impl.cc \
		draco/compression/mesh/mesh_encoder.cc \
		draco/compression/mesh/mesh_sequential_encoder.cc \
		draco/compression/mesh/mesh_edgebreaker_encoder.cc \
		draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_encoder.cc \
		draco/compression/point_cloud/algorithms/float_points_tree_encoder.cc \
		draco/compression/point_cloud/point_cloud_kd_tree_encoder.cc \
		draco/compression/point_cloud/point_cloud_encoder.cc \
		draco/compression/point_cloud/point_cloud_sequential_encoder.cc \
		draco/compression/attributes/sequential_normal_attribute_encoder.cc \
		draco/compression/attributes/sequential_quantization_attribute_encoder.cc \
		draco/compression/attributes/kd_tree_attributes_encoder.cc \
		draco/compression/attributes/attributes_encoder.cc \
		draco/compression/attributes/prediction_schemes/prediction_scheme_encoder_factory.cc \
		draco/compression/attributes/sequential_integer_attribute_encoder.cc \
		draco/compression/attributes/sequential_attribute_encoder.cc \
		draco/compression/attributes/sequential_attribute_encoders_controller.cc \
		draco/compression/encode.cc \
		draco/compression/expert_encode.cc \
		draco/metadata/metadata_encoder.cc \
		-I. \
		-o temp.o
fi
# m = 64*1024; s = 350000000; Math.floor(s/m)*m;
emcc -s WASM=1 -s USE_PTHREADS=1 -s NO_FILESYSTEM=1 -s TOTAL_MEMORY=349962240 -s MODULARIZE=1 -s 'EXPORT_NAME="GeometryModule"' -O3 \
  objectize.cc vector.cc \
  FastNoise.cpp noise.cc march.cc biomes.cc \
  temp.o \
  -I. \
  -o bin/geometry.js
cat prefix2.js bin/geometry.js >bin/geometry2.js
