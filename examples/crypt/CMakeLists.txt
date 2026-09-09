set(shader ${CMAKE_CURRENT_SOURCE_DIR}/crypt.slang)
set(shader_dir ${CMAKE_CURRENT_BINARY_DIR}/shaders)
set(vertex_spv ${shader_dir}/crypt.vert.spv)
set(fragment_spv ${shader_dir}/crypt.frag.spv)

NoGraphicsAPI_compile_slang(${vertex_spv} ${shader} vertexMain vertex
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crypt_shared.h)
NoGraphicsAPI_compile_slang(${fragment_spv} ${shader} fragmentMain fragment
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crypt_shared.h)

NoGraphicsAPI_add_example(example_crypt
    crypt.cpp
    crypt_shared.h
    ${shader}
    ${vertex_spv}
    ${fragment_spv}
)
target_link_libraries(example_crypt PRIVATE NoGraphicsAPIUtility::types)
target_compile_definitions(example_crypt PRIVATE
    NOGRAPHICSAPI_VERTEX_SPV_PATH="${vertex_spv}"
    NOGRAPHICSAPI_FRAGMENT_SPV_PATH="${fragment_spv}"
)
