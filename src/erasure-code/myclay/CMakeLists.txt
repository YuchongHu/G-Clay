# clay plugin

set(myclay_srcs
  ErasureCodePluginMyClay.cc
  ErasureCodeMyClay.cc
  $<TARGET_OBJECTS:erasure_code_objs>
  $<TARGET_OBJECTS:crush_objs>
  ${CMAKE_SOURCE_DIR}/src/common/str_map.cc
)

add_library(ec_myclay SHARED ${myclay_srcs})
set_target_properties(ec_myclay PROPERTIES
  INSTALL_RPATH "")
install(TARGETS ec_myclay DESTINATION ${erasure_plugin_dir})
