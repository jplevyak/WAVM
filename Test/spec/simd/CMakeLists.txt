set(SIMDSpecTestSources
    simd_address.wast
    simd_align.wast
    simd_load.wast
    simd_store.wast
    simd_const.wast
    simd_splat.wast
    simd_lane.wast
    simd_i8x16_cmp.wast
    simd_i16x8_cmp.wast
    simd_i32x4_cmp.wast
    simd_f32x4_cmp.wast)
add_custom_target(SIMDSpecTests SOURCES ${SIMDSpecTestSources})
set_target_properties(SIMDSpecTests PROPERTIES FOLDER Testing)

if(WAVM_ENABLE_RUNTIME)
    ADD_WAST_TESTS("${SIMDSpecTestSources}")
endif()
